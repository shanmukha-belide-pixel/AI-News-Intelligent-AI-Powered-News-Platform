#pragma once
// =============================================================================
// auth_middleware.hpp  —  JWT validation pre-handling filter for Drogon
// =============================================================================
#include <drogon/HttpFilter.h>
#include <spdlog/spdlog.h>
#include "../core/auth/jwt_service.hpp"

namespace news {

class AuthMiddleware : public drogon::HttpFilter<AuthMiddleware> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&&       stop,
                  drogon::FilterChainCallback&&  next) override
    {
        auto auth_header = req->getHeader("Authorization");
        auto token       = auth::JwtService::extract_bearer(auth_header);

        if (!token) {
            stop(unauthorized("Missing Authorization header"));
            return;
        }

        auth::JwtService jwt;
        auto claims = jwt.verify(*token);

        if (!claims) {
            stop(unauthorized("Invalid or expired token"));
            return;
        }

        // Guest users can access some routes — but blocked from premium routes
        // (route-level enforcement done via role checking in controller)

        // Attach claims to request attributes for downstream use
        req->getAttributes()->insert("user_id",  claims->user_id);
        req->getAttributes()->insert("username", claims->username);
        req->getAttributes()->insert("role",     claims->role);
        req->getAttributes()->insert("jti",      claims->jti);

        next();
    }

private:
    static drogon::HttpResponsePtr unauthorized(const std::string& msg) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody("{\"success\":false,\"error\":\"" + msg + "\"}");
        return resp;
    }
};

// ── Role guard helper (use inside controllers) ────────────────────────────────
inline bool require_role(const drogon::HttpRequestPtr& req,
                         const std::string& min_role,
                         std::function<void(const drogon::HttpResponsePtr&)>& cb)
{
    static const std::unordered_map<std::string,int> RANK = {
        {"guest",0},{"user",1},{"premium",2},{"admin",3},{"super_admin",4}
    };
    std::string role = req->getAttributes()->get<std::string>("role");
    int required = RANK.count(min_role) ? RANK.at(min_role) : 99;
    int actual   = RANK.count(role)    ? RANK.at(role)    : -1;
    if (actual < required) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody("{\"success\":false,\"error\":\"Insufficient permissions\"}");
        cb(resp);
        return false;
    }
    return true;
}

} // namespace news
