#pragma once
// =============================================================================
// ws_manager.hpp  —  WebSocket notification system with Redis Pub/Sub integration
// =============================================================================
#include <drogon/WebSocketController.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <mutex>
#include <unordered_set>
#include <thread>
#include "../../infrastructure/redis_client.hpp"

namespace news {

class WsManager {
public:
    static WsManager& instance() {
        static WsManager inst;
        return inst;
    }

    void start() {
        // Start a thread to listen on Redis Pub/Sub for breaking news/live events
        std::thread([this]() {
            auto redis = RedisClient::instance();
            if (!redis->is_connected()) {
                spdlog::warn("[WS Manager] Redis not connected. Pub/Sub disabled.");
                return;
            }
            spdlog::info("[WS Manager] Redis Pub/Sub listener started");
            
            // We use sw::redis::Redis directly for blocking subscription
            try {
                const auto& cfg = get_config().redis;
                sw::redis::ConnectionOptions opts;
                opts.host = cfg.host;
                opts.port = cfg.port;
                opts.db = cfg.db;
                if (!cfg.password.empty()) opts.password = cfg.password;
                
                sw::redis::Redis r(opts);
                auto sub = r.subscriber();
                
                sub.on_message([this](std::string channel, std::string msg) {
                    spdlog::info("[WS Manager] Pub/Sub message received on {}: {}", channel, msg);
                    broadcast(msg);
                });
                
                sub.subscribe("news_channel");
                sub.subscribe("sports_channel");
                sub.subscribe("weather_channel");
                
                while (true) {
                    try {
                        sub.consume();
                    } catch (const sw::redis::Error& err) {
                        spdlog::error("[WS Manager] Pub/Sub consume error: {}", err.what());
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("[WS Manager] Pub/Sub listener exception: {}", e.what());
            }
        }).detach();
    }

    void add_connection(const drogon::WebSocketConnectionPtr& conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.insert(conn);
        spdlog::debug("[WS Manager] Connection added. Total active: {}", connections_.size());
    }

    void remove_connection(const drogon::WebSocketConnectionPtr& conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(conn);
        spdlog::debug("[WS Manager] Connection removed. Total active: {}", connections_.size());
    }

    void broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& conn : connections_) {
            if (conn->connected()) {
                conn->send(message);
            }
        }
    }

    void send_to_user(const std::string& user_id, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& conn : connections_) {
            if (conn->connected() && conn->hasParameter("user_id") && 
                conn->getParameter<std::string>("user_id") == user_id) {
                conn->send(message);
            }
        }
    }

private:
    WsManager() = default;
    ~WsManager() = default;
    WsManager(const WsManager&) = delete;
    WsManager& operator=(const WsManager&) = delete;

    std::mutex mutex_;
    std::unordered_set<drogon::WebSocketConnectionPtr> connections_;
};

// ── WebSocket Routing Controller for Drogon ──────────────────────────────────
class WsController : public drogon::WebSocketController<WsController> {
public:
    void handleNewConnection(const drogon::HttpRequestPtr& req,
                             const drogon::WebSocketConnectionPtr& conn) override
    {
        // Try to parse query parameters (e.g., /api/v1/ws?user_id=123)
        auto params = req->getParameters();
        if (params.count("user_id")) {
            conn->setParameter("user_id", params.at("user_id"));
        }
        WsManager::instance().add_connection(conn);
        conn->send(R"({"status":"connected","message":"AI News Live Notification Stream"})");
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override {
        WsManager::instance().remove_connection(conn);
    }

    void handleNewMessage(const drogon::WebSocketConnectionPtr& conn,
                          std::string&& message,
                          const drogon::WebSocketMessageType& type) override
    {
        if (type == drogon::WebSocketMessageType::Ping) {
            conn->send("", drogon::WebSocketMessageType::Pong);
            return;
        }
        
        // Simple echo / heartbeats or incoming analytics tracking from client
        try {
            auto j = nlohmann::json::parse(message);
            if (j.value("event", "") == "ping") {
                conn->send(R"({"event":"pong"})");
            }
        } catch (...) {
            // Ignore malformed text
        }
    }

    WS_PATH_LIST_BEGIN
        // Map WebSocket endpoint
        ADD_PATH_TO("/api/v1/ws");
    WS_PATH_LIST_END
};

} // namespace news
