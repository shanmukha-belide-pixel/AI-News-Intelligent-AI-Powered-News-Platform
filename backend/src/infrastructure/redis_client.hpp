#pragma once
// =============================================================================
// redis_client.hpp  —  Async Redis caching (singleton, redis-plus-plus)
// =============================================================================
#include <string>
#include <optional>
#include <functional>
#include <memory>
#include <sw/redis++/redis++.h>
#include <spdlog/spdlog.h>
#include "../config/app_config.hpp"

namespace news {

class RedisClient {
public:
    static RedisClient* instance() {
        static RedisClient inst;
        return &inst;
    }

    // ── GET ───────────────────────────────────────────────────────────────────
    void get(const std::string& key,
             std::function<void(std::optional<std::string>)> cb) noexcept
    {
        try {
            auto val = redis_->get(key);
            cb(val ? std::optional<std::string>(*val) : std::nullopt);
        } catch (const sw::redis::Error& e) {
            spdlog::warn("[Redis] GET {} failed: {}", key, e.what());
            cb(std::nullopt);
        }
    }

    // ── SET with TTL ──────────────────────────────────────────────────────────
    void set_ex(const std::string& key,
                const std::string& value,
                int ttl_seconds) noexcept
    {
        try {
            redis_->setex(key, ttl_seconds, value);
        } catch (const sw::redis::Error& e) {
            spdlog::warn("[Redis] SET {} failed: {}", key, e.what());
        }
    }

    // ── SET (no TTL) ──────────────────────────────────────────────────────────
    void set(const std::string& key, const std::string& value) noexcept {
        try { redis_->set(key, value); }
        catch (const sw::redis::Error& e) {
            spdlog::warn("[Redis] SET {} failed: {}", key, e.what());
        }
    }

    // ── DELETE ────────────────────────────────────────────────────────────────
    void del(const std::string& key) noexcept {
        try { redis_->del(key); }
        catch (...) {}
    }

    // ── Pattern DELETE (cache invalidation) ───────────────────────────────────
    void del_pattern(const std::string& pattern) noexcept {
        try {
            std::vector<std::string> keys;
            redis_->keys(pattern, std::back_inserter(keys));
            if (!keys.empty()) redis_->del(keys.begin(), keys.end());
        } catch (...) {}
    }

    // ── INCR (view/like counters) ─────────────────────────────────────────────
    long long incr(const std::string& key) noexcept {
        try { return redis_->incr(key); }
        catch (...) { return 0; }
    }

    // ── EXISTS ────────────────────────────────────────────────────────────────
    bool exists(const std::string& key) noexcept {
        try { return redis_->exists(key) > 0; }
        catch (...) { return false; }
    }

    // ── Pub/Sub: publish ──────────────────────────────────────────────────────
    void publish(const std::string& channel, const std::string& message) noexcept {
        try { redis_->publish(channel, message); }
        catch (const sw::redis::Error& e) {
            spdlog::warn("[Redis] PUBLISH {} failed: {}", channel, e.what());
        }
    }

    bool is_connected() const { return redis_ != nullptr; }

private:
    RedisClient() {
        const auto& cfg = get_config().redis;
        try {
            sw::redis::ConnectionOptions opts;
            opts.host     = cfg.host;
            opts.port     = cfg.port;
            opts.db       = cfg.db;
            if (!cfg.password.empty()) opts.password = cfg.password;
            opts.socket_timeout = std::chrono::milliseconds(cfg.timeout_ms);

            sw::redis::ConnectionPoolOptions pool_opts;
            pool_opts.size = cfg.pool_size;

            redis_ = std::make_unique<sw::redis::Redis>(opts, pool_opts);
            redis_->ping();
            spdlog::info("[Redis] Connected to {}:{}", cfg.host, cfg.port);
        } catch (const sw::redis::Error& e) {
            spdlog::error("[Redis] Connection failed: {}", e.what());
            redis_.reset();
        }
    }

    std::unique_ptr<sw::redis::Redis> redis_;
};

} // namespace news
