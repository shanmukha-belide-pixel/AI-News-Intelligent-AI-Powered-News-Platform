// =============================================================================
// main.cpp  —  Drogon application entry point
// =============================================================================
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "config/app_config.hpp"
#include "core/auth/auth_controller.hpp"
#include "core/news/news_controller.hpp"
#include "core/ai/chat_controller.hpp"
#include "core/ai/ai_controller.hpp"
#include "core/weather/weather_controller.hpp"
#include "core/notification/ws_manager.hpp"
#include "api/middleware/auth_middleware.hpp"
#include "api/middleware/rate_limiter.hpp"
#include "api/middleware/cors_middleware.hpp"
#include "infrastructure/redis_client.hpp"
#include "infrastructure/kafka_producer.hpp"

int main() {
    // ── Logger setup ─────────────────────────────────────────────────────────
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink    = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/news_platform.log", 10 * 1024 * 1024, 5);
    auto logger = std::make_shared<spdlog::logger>("news",
        spdlog::sinks_init_list{console_sink, file_sink});
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    // ── Load config ──────────────────────────────────────────────────────────
    news::AppConfig cfg;
    try {
        cfg = news::AppConfig::from_env();
        spdlog::info("Configuration loaded successfully");
    } catch (const std::exception& e) {
        spdlog::critical("Config error: {}", e.what());
        return 1;
    }

    // ── Set log level ────────────────────────────────────────────────────────
    if (cfg.server.log_level == "debug")   spdlog::set_level(spdlog::level::debug);
    else if (cfg.server.log_level == "warn") spdlog::set_level(spdlog::level::warn);
    else                                    spdlog::set_level(spdlog::level::info);

    // ── Drogon app configuration ─────────────────────────────────────────────
    auto& app = drogon::app();

    // Thread pool
    app.setThreadNum(cfg.server.threads);

    // PostgreSQL connection pool
    app.addDbClient(
        drogon::orm::ClientType::PostgreSQL,
        cfg.database.connection_string(),
        cfg.database.pool_size,
        "default"
    );

    // SSL
    if (cfg.server.ssl_enabled) {
        app.setSSLFiles(cfg.server.ssl_cert, cfg.server.ssl_key);
    }

    // Request timeout
    app.setClientMaxBodySize(8 * 1024 * 1024); // 8MB
    app.setIdleConnectionTimeout(60);

    // ── Global middleware (applied in order) ─────────────────────────────────
    app.registerPreRoutingAdvice<news::CorsMiddleware>();
    app.registerPreHandlingAdvice<news::RateLimiterMiddleware>();

    // ── Drogon controllers (auto-registered via DROGON_REGISTER_CONTROLLER) ──
    // Controllers self-register; no manual route setup needed.
    // Routes defined inside each controller class.

    // ── WebSocket notification manager ───────────────────────────────────────
    news::WsManager::instance().start();

    // ── Startup banner ───────────────────────────────────────────────────────
    spdlog::info("╔══════════════════════════════════════════╗");
    spdlog::info("║   AI News Platform  v1.0.0              ║");
    spdlog::info("║   {}:{:<30}║", cfg.server.host, cfg.server.port);
    spdlog::info("║   Threads: {:<30}║", cfg.server.threads);
    spdlog::info("╚══════════════════════════════════════════╝");

    // ── Run ──────────────────────────────────────────────────────────────────
    app.addListener(cfg.server.host, cfg.server.port)
       .run();

    return 0;
}
