#pragma once
// =============================================================================
// news_controller.hpp  —  REST API for news: feed, details, sports, search
// =============================================================================
// GET  /api/v1/news/feed                  → Personalised + trending feed
// GET  /api/v1/news/international         → International news
// GET  /api/v1/news/national              → National (India) news
// GET  /api/v1/news/state?state=Karnataka → State news
// GET  /api/v1/news/sports?sport=cricket  → Sports news (with sport filter)
// GET  /api/v1/news/breaking              → Breaking news banner
// GET  /api/v1/news/trending              → Trending topics
// GET  /api/v1/news/search?q=...          → Full-text + semantic search
// GET  /api/v1/news/:id                   → Article detail + AI summary
// POST /api/v1/news/:id/bookmark          → Add bookmark (auth required)
// DELETE /api/v1/news/:id/bookmark        → Remove bookmark (auth required)
// POST /api/v1/news/:id/like              → Like article (auth required)
// POST /api/v1/news/:id/read              → Record reading progress (auth)
// =============================================================================
#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include "news_fetcher.hpp"
#include "../../infrastructure/redis_client.hpp"
#include "../../infrastructure/elasticsearch_client.hpp"
#include "../../api/middleware/auth_middleware.hpp"

namespace news {

class NewsController
    : public drogon::HttpController<NewsController>
{
public:
    METHOD_LIST_BEGIN
        // Public endpoints
        ADD_METHOD_TO(NewsController::get_feed,          "/api/v1/news/feed",          drogon::Get);
        ADD_METHOD_TO(NewsController::get_international, "/api/v1/news/international", drogon::Get);
        ADD_METHOD_TO(NewsController::get_national,      "/api/v1/news/national",      drogon::Get);
        ADD_METHOD_TO(NewsController::get_state,         "/api/v1/news/state",         drogon::Get);
        ADD_METHOD_TO(NewsController::get_sports,        "/api/v1/news/sports",        drogon::Get);
        ADD_METHOD_TO(NewsController::get_breaking,      "/api/v1/news/breaking",      drogon::Get);
        ADD_METHOD_TO(NewsController::get_trending,      "/api/v1/news/trending",      drogon::Get);
        ADD_METHOD_TO(NewsController::search,            "/api/v1/news/search",        drogon::Get);
        ADD_METHOD_TO(NewsController::get_article,       "/api/v1/news/{id}",          drogon::Get);
        // Authenticated endpoints
        ADD_METHOD_TO(NewsController::bookmark_add,      "/api/v1/news/{id}/bookmark", drogon::Post,
                      "news::AuthMiddleware");
        ADD_METHOD_TO(NewsController::bookmark_remove,   "/api/v1/news/{id}/bookmark", drogon::Delete,
                      "news::AuthMiddleware");
        ADD_METHOD_TO(NewsController::like_article,      "/api/v1/news/{id}/like",     drogon::Post,
                      "news::AuthMiddleware");
        ADD_METHOD_TO(NewsController::record_read,       "/api/v1/news/{id}/read",     drogon::Post,
                      "news::AuthMiddleware");
    METHOD_LIST_END

    // ── GET /api/v1/news/feed ─────────────────────────────────────────────────
    void get_feed(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params   = req->getParameters();
        std::string cat     = params.count("category") ? params.at("category") : "";
        std::string country = params.count("country")  ? params.at("country")  : "in";
        int limit = get_int(params, "limit", 30);
        int page  = get_int(params, "page", 1);

        std::string cache_key = "feed:" + cat + ":" + country + ":" + std::to_string(page);

        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }

                // Fetch international + national together for home feed
                fetcher_.fetch_all(cat, country, limit,
                    [=, cb=std::move(cb)](std::vector<RawArticle> articles) mutable {
                        auto j = articles_to_json(articles, page, limit);
                        std::string body = j.dump();
                        RedisClient::instance()->set_ex(cache_key, body, 300); // 5 min
                        cb(json_resp(body));
                    });
            });
    }

    // ── GET /api/v1/news/international ───────────────────────────────────────
    void get_international(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params = req->getParameters();
        std::string cat   = params.count("category") ? params.at("category") : "";
        int limit = get_int(params, "limit", 20);

        std::string cache_key = "news:intl:" + cat;
        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }
                fetcher_.fetch_international(cat, limit,
                    [=, cb=std::move(cb)](std::vector<RawArticle> arts) mutable {
                        auto body = articles_to_json(arts, 1, limit).dump();
                        RedisClient::instance()->set_ex(cache_key, body, 600);
                        cb(json_resp(body));
                    });
            });
    }

    // ── GET /api/v1/news/national ─────────────────────────────────────────────
    void get_national(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params = req->getParameters();
        std::string cat  = params.count("category") ? params.at("category") : "";
        std::string lang = params.count("language")  ? params.at("language") : "";
        int limit = get_int(params, "limit", 20);

        std::string cache_key = "news:national:" + cat + ":" + lang;
        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }
                fetcher_.fetch_national(cat, limit,
                    [=, cb=std::move(cb)](std::vector<RawArticle> arts) mutable {
                        // Filter by language if requested
                        if (!lang.empty()) {
                            arts.erase(std::remove_if(arts.begin(), arts.end(),
                                [&lang](const RawArticle& a){ return a.language != lang; }),
                                arts.end());
                        }
                        auto body = articles_to_json(arts, 1, limit).dump();
                        RedisClient::instance()->set_ex(cache_key, body, 600);
                        cb(json_resp(body));
                    });
            });
    }

    // ── GET /api/v1/news/state?state=Maharashtra&language=mr ─────────────────
    void get_state(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params = req->getParameters();
        std::string state = params.count("state") ? params.at("state") : "";
        std::string lang  = params.count("language") ? params.at("language") : "";
        int limit = get_int(params, "limit", 20);

        std::string cache_key = "news:state:" + state + ":" + lang;
        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }
                fetcher_.fetch_state(state, limit,
                    [=, cb=std::move(cb)](std::vector<RawArticle> arts) mutable {
                        if (!lang.empty()) {
                            arts.erase(std::remove_if(arts.begin(), arts.end(),
                                [&lang](const RawArticle& a){ return a.language != lang; }),
                                arts.end());
                        }
                        auto body = articles_to_json(arts, 1, limit).dump();
                        RedisClient::instance()->set_ex(cache_key, body, 600);
                        cb(json_resp(body));
                    });
            });
    }

    // ── GET /api/v1/news/sports?sport=cricket ────────────────────────────────
    void get_sports(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params = req->getParameters();
        std::string sport = params.count("sport") ? params.at("sport") : "";
        int limit = get_int(params, "limit", 30);

        std::string cache_key = "news:sports:" + sport;
        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }
                fetcher_.fetch_sports(sport, limit,
                    [=, cb=std::move(cb)](std::vector<RawArticle> arts) mutable {
                        auto body = articles_to_json(arts, 1, limit).dump();
                        RedisClient::instance()->set_ex(cache_key, body, 300);
                        cb(json_resp(body));
                    });
            });
    }

    // ── GET /api/v1/news/breaking ─────────────────────────────────────────────
    void get_breaking(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params  = req->getParameters();
        std::string country = params.count("country") ? params.at("country") : "in";
        std::string cache_key = "news:breaking:" + country;

        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }
                // For breaking news, use DB query (articles marked is_breaking=true)
                auto db = drogon::app().getDbClient();
                db->execSqlAsync(
                    R"(SELECT a.id, a.title, a.excerpt, a.image_url, a.publisher,
                              a.published_at, a.url, c.name as category
                       FROM news_articles a
                       LEFT JOIN categories c ON c.id = a.category_id
                       WHERE a.is_breaking = TRUE AND a.status = 'published'
                         AND a.published_at > NOW() - INTERVAL '24 hours'
                       ORDER BY a.published_at DESC LIMIT 10)",
                    [=, cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                        nlohmann::json arr = nlohmann::json::array();
                        for (const auto& row : r) {
                            arr.push_back({
                                {"id",           row["id"].as<std::string>()},
                                {"title",        row["title"].as<std::string>()},
                                {"excerpt",      row["excerpt"].as<std::string>()},
                                {"image_url",    row["image_url"].as<std::string>()},
                                {"publisher",    row["publisher"].as<std::string>()},
                                {"published_at", row["published_at"].as<std::string>()},
                                {"url",          row["url"].as<std::string>()},
                                {"category",     row["category"].as<std::string>()}
                            });
                        }
                        nlohmann::json resp_j = {{"success",true},{"data",arr},{"breaking",true}};
                        auto body = resp_j.dump();
                        RedisClient::instance()->set_ex(cache_key, body, 120); // 2 min
                        cb(json_resp(body));
                    },
                    [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                        spdlog::error("[Breaking] DB error: {}", e.base().what());
                        cb(error_resp("DB error", 500));
                    });
            });
    }

    // ── GET /api/v1/news/trending ─────────────────────────────────────────────
    void get_trending(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        std::string cache_key = "news:trending";
        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }
                auto db = drogon::app().getDbClient();
                db->execSqlAsync(
                    R"(SELECT keyword, slug, trend_score, article_count, search_count,
                              velocity, country
                       FROM trending_topics
                       WHERE is_active = TRUE
                       ORDER BY trend_score DESC LIMIT 20)",
                    [=, cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                        nlohmann::json arr = nlohmann::json::array();
                        for (const auto& row : r) {
                            arr.push_back({
                                {"keyword",       row["keyword"].as<std::string>()},
                                {"slug",          row["slug"].as<std::string>()},
                                {"trend_score",   row["trend_score"].as<double>()},
                                {"article_count", row["article_count"].as<int>()},
                                {"search_count",  row["search_count"].as<int>()},
                                {"velocity",      row["velocity"].as<double>()}
                            });
                        }
                        nlohmann::json j = {{"success",true},{"data",arr}};
                        auto body = j.dump();
                        RedisClient::instance()->set_ex(cache_key, body, 900); // 15 min
                        cb(json_resp(body));
                    },
                    [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                        cb(error_resp(e.base().what(), 500));
                    });
            });
    }

    // ── GET /api/v1/news/search?q=ipl&category=sports&from=2026-07-01 ────────
    void search(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params  = req->getParameters();
        std::string q       = params.count("q")        ? params.at("q")        : "";
        std::string cat     = params.count("category") ? params.at("category") : "";
        std::string from_dt = params.count("from")     ? params.at("from")     : "";
        std::string to_dt   = params.count("to")       ? params.at("to")       : "";
        std::string scope   = params.count("scope")    ? params.at("scope")    : ""; // intl/national/state/sports
        int limit = get_int(params, "limit", 20);

        if (q.empty()) { cb(error_resp("Query 'q' is required")); return; }

        // Elasticsearch semantic search
        ElasticsearchClient::instance()->search(
            q, cat, from_dt, to_dt, scope, limit,
            [cb=std::move(cb), q](nlohmann::json results) mutable {
                nlohmann::json j = {{"success",true}, {"query",q}, {"data",results}};
                cb(json_resp(j.dump()));
            });
    }

    // ── GET /api/v1/news/:id ──────────────────────────────────────────────────
    void get_article(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                     const std::string& id)
    {
        std::string cache_key = "article:" + id;
        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) { return cb(json_resp(*cached)); }
                auto db = drogon::app().getDbClient();
                db->execSqlAsync(
                    R"(SELECT a.*, c.name as cat_name, c.slug as cat_slug,
                              s.content as summary_one_line,
                              s2.content as summary_thirty_sec,
                              s2.key_points, s2.why_it_matters, s2.sentiment
                       FROM news_articles a
                       LEFT JOIN categories c ON c.id = a.category_id
                       LEFT JOIN ai_summaries s  ON s.article_id  = a.id AND s.summary_type  = 'one_line'
                       LEFT JOIN ai_summaries s2 ON s2.article_id = a.id AND s2.summary_type = 'thirty_sec'
                       WHERE a.id = $1 AND a.status = 'published')",
                    [=, cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                        if (r.empty()) { cb(error_resp("Article not found", 404)); return; }
                        auto& row = r[0];
                        nlohmann::json art = {
                            {"id",            row["id"].as<std::string>()},
                            {"title",         row["title"].as<std::string>()},
                            {"content",       row["content"].as<std::string>()},
                            {"excerpt",       row["excerpt"].as<std::string>()},
                            {"url",           row["url"].as<std::string>()},
                            {"image_url",     row["image_url"].as<std::string>()},
                            {"author",        row["author"].as<std::string>()},
                            {"publisher",     row["publisher"].as<std::string>()},
                            {"category",      row["cat_name"].as<std::string>()},
                            {"category_slug", row["cat_slug"].as<std::string>()},
                            {"published_at",  row["published_at"].as<std::string>()},
                            {"reading_time",  row["reading_time_min"].as<int>()},
                            {"view_count",    row["view_count"].as<int>()},
                            {"like_count",    row["like_count"].as<int>()},
                            {"ai_summary",    row["summary_one_line"].as<std::string>()},
                            {"ai_summary_30s",row["summary_thirty_sec"].as<std::string>()},
                            {"why_it_matters",row["why_it_matters"].as<std::string>()}
                        };
                        nlohmann::json j = {{"success",true},{"data",art}};
                        auto body = j.dump();
                        RedisClient::instance()->set_ex(cache_key, body, 1800);
                        cb(json_resp(body));
                    },
                    [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                        cb(error_resp(e.base().what(), 500));
                    }, id);
            });
    }

    // ── POST /api/v1/news/:id/bookmark ────────────────────────────────────────
    void bookmark_add(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                      const std::string& article_id)
    {
        std::string user_id = req->getAttributes()->get<std::string>("user_id");
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            "INSERT INTO bookmarks (user_id, article_id) VALUES ($1,$2) ON CONFLICT DO NOTHING",
            [cb=std::move(cb)](const drogon::orm::Result&) mutable {
                cb(json_resp(R"({"success":true,"message":"Bookmarked"})"));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(error_resp(e.base().what(), 500));
            }, user_id, article_id);
    }

    // ── DELETE /api/v1/news/:id/bookmark ──────────────────────────────────────
    void bookmark_remove(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                         const std::string& article_id)
    {
        std::string user_id = req->getAttributes()->get<std::string>("user_id");
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            "DELETE FROM bookmarks WHERE user_id=$1 AND article_id=$2",
            [cb=std::move(cb)](const drogon::orm::Result&) mutable {
                cb(json_resp(R"({"success":true,"message":"Bookmark removed"})"));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(error_resp(e.base().what(), 500));
            }, user_id, article_id);
    }

    // ── POST /api/v1/news/:id/like ────────────────────────────────────────────
    void like_article(const drogon::HttpRequestPtr&,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                      const std::string& article_id)
    {
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            "UPDATE news_articles SET like_count = like_count + 1 WHERE id = $1",
            [cb=std::move(cb)](const drogon::orm::Result&) mutable {
                cb(json_resp(R"({"success":true})"));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(error_resp(e.base().what(), 500));
            }, article_id);
    }

    // ── POST /api/v1/news/:id/read ────────────────────────────────────────────
    void record_read(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                     const std::string& article_id)
    {
        std::string user_id = req->getAttributes()->get<std::string>("user_id");
        auto body_json = nlohmann::json::parse(req->body(), nullptr, false);
        double progress   = body_json.value("progress", 0.0);
        int read_seconds  = body_json.value("read_seconds", 0);
        bool completed    = body_json.value("completed", false);

        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            "SELECT upsert_reading_history($1,$2,$3,$4,$5)",
            [cb=std::move(cb)](const drogon::orm::Result&) mutable {
                cb(json_resp(R"({"success":true})"));
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(error_resp(e.base().what(), 500));
            }, user_id, article_id, progress, read_seconds, completed);
    }

private:
    NewsFetcherService fetcher_;

    // ── Helpers ───────────────────────────────────────────────────────────────
    static int get_int(const std::map<std::string,std::string>& p,
                       const std::string& key, int def) {
        try { return p.count(key) ? std::stoi(p.at(key)) : def; }
        catch (...) { return def; }
    }

    static nlohmann::json articles_to_json(const std::vector<RawArticle>& arts,
                                           int page, int limit)
    {
        nlohmann::json arr = nlohmann::json::array();
        int start = (page - 1) * limit;
        int end   = std::min(start + limit, (int)arts.size());
        for (int i = start; i < end; ++i) {
            const auto& a = arts[i];
            std::string scope_str;
            switch (a.scope) {
                case NewsScope::International: scope_str = "international"; break;
                case NewsScope::National:      scope_str = "national";      break;
                case NewsScope::State:         scope_str = "state";         break;
            }
            arr.push_back({
                {"id",           a.external_id},
                {"title",        a.title},
                {"excerpt",      a.excerpt},
                {"url",          a.url},
                {"image_url",    a.image_url},
                {"publisher",    a.publisher},
                {"publisher_icon",a.publisher_icon},
                {"author",       a.author},
                {"category",     a.category},
                {"language",     a.language},
                {"country",      a.country},
                {"state",        a.state},
                {"scope",        scope_str},
                {"published_at", a.published_at},
                {"is_breaking",  a.is_breaking}
            });
        }
        return {
            {"success", true},
            {"data",    arr},
            {"total",   (int)arts.size()},
            {"page",    page},
            {"limit",   limit}
        };
    }

    static drogon::HttpResponsePtr json_resp(const std::string& body) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(body);
        return resp;
    }

    static drogon::HttpResponsePtr error_resp(const std::string& msg, int code = 400) {
        nlohmann::json j = {{"success", false}, {"error", msg}};
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(code));
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(j.dump());
        return resp;
    }
};

} // namespace news
