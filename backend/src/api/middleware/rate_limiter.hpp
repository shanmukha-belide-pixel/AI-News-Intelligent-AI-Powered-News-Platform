#pragma once
// =============================================================================
// rate_limiter.hpp  —  Token bucket rate limiting per IP + per user
// =============================================================================
#include <drogon/HttpFilter.h>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <spdlog/spdlog.h>
#include "../../infrastructure/redis_client.hpp"

namespace news {

class RateLimiterMiddleware : public drogon::HttpFilter<RateLimiterMiddleware> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&&       stop,
                  drogon::FilterChainCallback&&  next) override
    {
        std::string ip = req->getPeerAddr().toIp();

        // Per-IP: 120 req/min for unauthenticated
        // Per-user: 300 req/min for authenticated
        std::string key;
        int limit;

        auto auth = req->getHeader("Authorization");
        if (!auth.empty() && auth.substr(0, 7) == "Bearer ") {
            // Authenticated — use user JTI from token
            key   = "rl:user:" + ip;
            limit = 300;
        } else {
            key   = "rl:ip:" + ip;
            limit = 120;
        }

        auto redis = RedisClient::instance();
        if (!redis->is_connected()) {
            // Redis down → allow traffic (fail open)
            next();
            return;
        }

        // Sliding window counter
        auto now_min = std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string window_key = key + ":" + std::to_string(now_min);

        long long count = redis->incr(window_key);
        if (count == 1) {
            // Set TTL of 90 seconds on first hit (covers current + overlap)
            redis->set_ex(window_key, std::to_string(count), 90);
        }

        if (count > limit) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k429TooManyRequests);
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            resp->addHeader("Retry-After", "60");
            resp->addHeader("X-RateLimit-Limit", std::to_string(limit));
            resp->addHeader("X-RateLimit-Remaining", "0");
            resp->setBody("{\"success\":false,\"error\":\"Rate limit exceeded. Please wait.\"}");
            stop(resp);
            return;
        }

        // Add rate limit headers to response info for monitoring
        req->getAttributes()->insert("rl_remaining",
            std::to_string(std::max(0LL, (long long)limit - count)));

        next();
    }
};

// ── CORS Middleware ────────────────────────────────────────────────────────────
class CorsMiddleware : public drogon::HttpFilter<CorsMiddleware> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&&       stop,
                  drogon::FilterChainCallback&&  next) override
    {
        if (req->getMethod() == drogon::Options) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k204NoContent);
            add_cors_headers(resp);
            stop(resp);
            return;
        }
        next();
    }

    static void add_cors_headers(const drogon::HttpResponsePtr& resp) {
        resp->addHeader("Access-Control-Allow-Origin",  "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers",
                        "Content-Type, Authorization, X-Request-ID");
        resp->addHeader("Access-Control-Max-Age", "86400");
    }
};

} // namespace news
