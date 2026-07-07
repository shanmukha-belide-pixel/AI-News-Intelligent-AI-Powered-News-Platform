#pragma once
// =============================================================================
// auth_controller.hpp  —  Auth endpoints (register, login, OAuth, refresh)
// =============================================================================
#include <drogon/HttpController.h>
#include <nlohmann/json.hpp>
#include <bcrypt/BCrypt.hpp>
#include <spdlog/spdlog.h>
#include "jwt_service.hpp"
#include "oauth_service.hpp"
#include "../../infrastructure/redis_client.hpp"

namespace news::auth {

class AuthController
    : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::register_email,  "/api/v1/auth/register",         drogon::Post);
        ADD_METHOD_TO(AuthController::login_email,     "/api/v1/auth/login",            drogon::Post);
        ADD_METHOD_TO(AuthController::login_google,    "/api/v1/auth/google",           drogon::Post);
        ADD_METHOD_TO(AuthController::login_apple,     "/api/v1/auth/apple",            drogon::Post);
        ADD_METHOD_TO(AuthController::refresh_token,   "/api/v1/auth/refresh",          drogon::Post);
        ADD_METHOD_TO(AuthController::logout,          "/api/v1/auth/logout",           drogon::Post,  "news::AuthMiddleware");
        ADD_METHOD_TO(AuthController::me,              "/api/v1/auth/me",               drogon::Get,   "news::AuthMiddleware");
        ADD_METHOD_TO(AuthController::guest_login,     "/api/v1/auth/guest",            drogon::Post);
    METHOD_LIST_END

    // ── POST /api/v1/auth/register ───────────────────────────────────────────
    void register_email(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto j = parse_body(req);
        if (!j) { cb(err("Invalid JSON")); return; }

        std::string email    = j->value("email", "");
        std::string password = j->value("password", "");
        std::string username = j->value("username", "");

        if (email.empty() || password.size() < 8 || username.empty()) {
            cb(err("email, username and password (≥8 chars) are required")); return;
        }

        // Hash password with bcrypt cost 12
        std::string hash = BCrypt::generateHash(password, 12);

        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            R"(INSERT INTO users (email, username, password_hash, auth_provider, is_verified)
               VALUES ($1,$2,$3,'email',false) RETURNING id, username, role)",
            [=, cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                auto user_id  = r[0]["id"].as<std::string>();
                auto uname    = r[0]["username"].as<std::string>();
                auto role     = r[0]["role"].as<std::string>();
                auto [access, refresh] = issue_tokens(user_id, uname, role);
                store_session(user_id, refresh, req);
                cb(ok({{"access_token",  access},
                       {"refresh_token", refresh},
                       {"user_id",       user_id},
                       {"username",      uname}}));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                std::string msg = e.base().what();
                if (msg.find("duplicate") != std::string::npos)
                    cb(err("Email or username already exists", 409));
                else
                    cb(err(msg, 500));
            });
    }

    // ── POST /api/v1/auth/login ──────────────────────────────────────────────
    void login_email(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto j = parse_body(req);
        if (!j) { cb(err("Invalid JSON")); return; }
        std::string email    = j->value("email", "");
        std::string password = j->value("password", "");
        if (email.empty() || password.empty()) { cb(err("email and password required")); return; }

        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            "SELECT id, username, role, password_hash, is_active, locked_until FROM users WHERE email=$1 AND is_deleted=false",
            [=, cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                if (r.empty()) { cb(err("Invalid credentials", 401)); return; }
                auto& row = r[0];
                if (!row["is_active"].as<bool>()) { cb(err("Account suspended", 403)); return; }
                if (!BCrypt::validatePassword(password, row["password_hash"].as<std::string>())) {
                    cb(err("Invalid credentials", 401)); return;
                }
                auto user_id = row["id"].as<std::string>();
                auto uname   = row["username"].as<std::string>();
                auto role    = row["role"].as<std::string>();
                auto [access, refresh] = issue_tokens(user_id, uname, role);
                store_session(user_id, refresh, req);
                // Update last_login_at
                drogon::app().getDbClient()->execSqlAsync(
                    "UPDATE users SET last_login_at=NOW(), failed_login_count=0 WHERE id=$1",
                    [](const drogon::orm::Result&){}, [](const drogon::orm::DrogonDbException&){},
                    user_id);
                cb(ok({{"access_token",  access},
                       {"refresh_token", refresh},
                       {"user_id",       user_id},
                       {"username",      uname},
                       {"role",          role}}));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(err(e.base().what(), 500));
            });
    }

    // ── POST /api/v1/auth/google ─────────────────────────────────────────────
    void login_google(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto j = parse_body(req);
        if (!j) { cb(err("Invalid JSON")); return; }
        std::string id_token = j->value("id_token", "");
        if (id_token.empty()) { cb(err("id_token required")); return; }

        OAuthService::verify_google_token(id_token,
            [=, cb=std::move(cb)](std::optional<OAuthUser> user) mutable {
                if (!user) { cb(err("Invalid Google token", 401)); return; }
                upsert_oauth_user(*user, "google", cb, req);
            });
    }

    // ── POST /api/v1/auth/apple ──────────────────────────────────────────────
    void login_apple(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto j = parse_body(req);
        if (!j) { cb(err("Invalid JSON")); return; }
        std::string id_token = j->value("identity_token", "");
        if (id_token.empty()) { cb(err("identity_token required")); return; }

        OAuthService::verify_apple_token(id_token,
            [=, cb=std::move(cb)](std::optional<OAuthUser> user) mutable {
                if (!user) { cb(err("Invalid Apple token", 401)); return; }
                upsert_oauth_user(*user, "apple", cb, req);
            });
    }

    // ── POST /api/v1/auth/guest ──────────────────────────────────────────────
    void guest_login(const drogon::HttpRequestPtr&,
                     std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        // Generate ephemeral guest user
        std::string guest_id = "guest_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
        JwtService jwt;
        TokenClaims claims{guest_id, guest_id, "guest", guest_id, "access"};
        auto access = jwt.sign_access(claims);
        cb(ok({{"access_token", access},
               {"user_id",      guest_id},
               {"role",         "guest"},
               {"expires_in",   3600}}));
    }

    // ── POST /api/v1/auth/refresh ─────────────────────────────────────────────
    void refresh_token(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto j = parse_body(req);
        if (!j) { cb(err("Invalid JSON")); return; }
        std::string token = j->value("refresh_token", "");
        if (token.empty()) { cb(err("refresh_token required")); return; }

        JwtService jwt;
        auto claims = jwt.verify(token);
        if (!claims || claims->token_type != "refresh") {
            cb(err("Invalid or expired refresh token", 401)); return;
        }

        // Check token in DB sessions
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            R"(SELECT s.user_id, u.username, u.role FROM user_sessions s
               JOIN users u ON u.id = s.user_id
               WHERE s.refresh_token=$1 AND s.is_active=true AND s.expires_at > NOW())",
            [=, cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                if (r.empty()) { cb(err("Session expired", 401)); return; }
                auto user_id = r[0]["user_id"].as<std::string>();
                auto uname   = r[0]["username"].as<std::string>();
                auto role    = r[0]["role"].as<std::string>();
                JwtService jwt2;
                TokenClaims c{user_id, uname, role, user_id, "access"};
                auto new_access = jwt2.sign_access(c);
                cb(ok({{"access_token", new_access},
                       {"expires_in",   3600}}));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(err(e.base().what(), 500));
            }, token);
    }

    // ── POST /api/v1/auth/logout ──────────────────────────────────────────────
    void logout(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        std::string user_id = req->getAttributes()->get<std::string>("user_id");
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            "UPDATE user_sessions SET is_active=false WHERE user_id=$1",
            [cb=std::move(cb)](const drogon::orm::Result&) mutable {
                cb(ok({{"message","Logged out successfully"}}));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(err(e.base().what(), 500));
            }, user_id);
    }

    // ── GET /api/v1/auth/me ───────────────────────────────────────────────────
    void me(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        std::string user_id = req->getAttributes()->get<std::string>("user_id");
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            R"(SELECT u.id, u.email, u.username, u.role, u.created_at,
                      p.display_name, p.avatar_url, p.reading_streak, p.total_read,
                      p.listen_minutes, p.language,
                      s.plan, s.status as sub_status, s.expires_at as sub_expires
               FROM users u
               LEFT JOIN profiles p ON p.user_id = u.id
               LEFT JOIN subscriptions s ON s.user_id = u.id
               WHERE u.id=$1 AND u.is_deleted=false)",
            [cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                if (r.empty()) { cb(err("User not found", 404)); return; }
                auto& row = r[0];
                nlohmann::json user = {
                    {"id",             row["id"].as<std::string>()},
                    {"email",          row["email"].as<std::string>()},
                    {"username",       row["username"].as<std::string>()},
                    {"role",           row["role"].as<std::string>()},
                    {"display_name",   row["display_name"].as<std::string>()},
                    {"avatar_url",     row["avatar_url"].as<std::string>()},
                    {"reading_streak", row["reading_streak"].as<int>()},
                    {"total_read",     row["total_read"].as<int>()},
                    {"listen_minutes", row["listen_minutes"].as<int>()},
                    {"language",       row["language"].as<std::string>()},
                    {"subscription",   {
                        {"plan",    row["plan"].as<std::string>()},
                        {"status",  row["sub_status"].as<std::string>()},
                        {"expires", row["sub_expires"].as<std::string>()}
                    }}
                };
                cb(ok(user));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(err(e.base().what(), 500));
            }, user_id);
    }

private:
    JwtService jwt_;

    std::pair<std::string,std::string> issue_tokens(const std::string& uid,
                                                     const std::string& uname,
                                                     const std::string& role)
    {
        TokenClaims c{uid, uname, role, uid + "_access", "access"};
        std::string access  = jwt_.sign_access(c);
        std::string refresh = jwt_.sign_refresh(uid, uid + "_refresh");
        return {access, refresh};
    }

    void store_session(const std::string& user_id, const std::string& refresh,
                       const drogon::HttpRequestPtr& req)
    {
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            R"(INSERT INTO user_sessions (user_id, refresh_token, ip_address, user_agent, expires_at)
               VALUES ($1,$2,$3,$4, NOW() + INTERVAL '30 days'))",
            [](const drogon::orm::Result&){},
            [](const drogon::orm::DrogonDbException& e){
                spdlog::error("Session store error: {}", e.base().what());
            },
            user_id, refresh,
            req->getPeerAddr().toIp(),
            req->getHeader("User-Agent"));
    }

    void upsert_oauth_user(const OAuthUser& ouser,
                           const std::string& provider,
                           std::function<void(const drogon::HttpResponsePtr&)>& cb,
                           const drogon::HttpRequestPtr& req)
    {
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            R"(INSERT INTO users (email, username, auth_provider, provider_id, is_verified, is_active)
               VALUES ($1,$2,$3,$4,true,true)
               ON CONFLICT (email) DO UPDATE
                   SET last_login_at=NOW(), provider_id=EXCLUDED.provider_id
               RETURNING id, username, role)",
            [=, &cb](const drogon::orm::Result& r) mutable {
                auto uid   = r[0]["id"].as<std::string>();
                auto uname = r[0]["username"].as<std::string>();
                auto role  = r[0]["role"].as<std::string>();
                // Upsert profile avatar
                drogon::app().getDbClient()->execSqlAsync(
                    "UPDATE profiles SET avatar_url=$1, display_name=$2 WHERE user_id=$3",
                    [](const drogon::orm::Result&){},
                    [](const drogon::orm::DrogonDbException&){},
                    ouser.avatar_url, ouser.display_name, uid);
                auto [access, refresh] = issue_tokens(uid, uname, role);
                store_session(uid, refresh, req);
                cb(ok({{"access_token",  access},
                       {"refresh_token", refresh},
                       {"user_id",       uid},
                       {"username",      uname}}));
            },
            [&cb](const drogon::orm::DrogonDbException& e) mutable {
                cb(err(e.base().what(), 500));
            },
            ouser.email,
            ouser.email.substr(0, ouser.email.find('@')),
            provider,
            ouser.provider_id);
    }

    static std::optional<nlohmann::json> parse_body(const drogon::HttpRequestPtr& req) {
        try { return nlohmann::json::parse(req->body()); }
        catch (...) { return std::nullopt; }
    }

    static drogon::HttpResponsePtr ok(nlohmann::json data) {
        nlohmann::json j = {{"success", true}, {"data", std::move(data)}};
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(j.dump());
        return resp;
    }

    static drogon::HttpResponsePtr err(const std::string& msg, int code = 400) {
        nlohmann::json j = {{"success", false}, {"error", msg}};
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(code));
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(j.dump());
        return resp;
    }
};

} // namespace news::auth
