#pragma once
// =============================================================================
// app_config.hpp  —  Centralized configuration loading
// =============================================================================
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace news {

struct DatabaseConfig {
    std::string host     = "localhost";
    int         port     = 5432;
    std::string name     = "ai_news";
    std::string user     = "news_user";
    std::string password;
    int         pool_size = 10;
    std::string connection_string() const {
        return "host=" + host + " port=" + std::to_string(port) +
               " dbname=" + name + " user=" + user +
               " password=" + password + " sslmode=prefer";
    }
};

struct RedisConfig {
    std::string host      = "localhost";
    int         port      = 6379;
    std::string password;
    int         db        = 0;
    int         pool_size = 8;
    int         timeout_ms = 200;
};

struct KafkaConfig {
    std::string brokers = "localhost:9092";
    std::string group_id = "news_platform";
    std::string client_id = "news_server";
};

struct ElasticsearchConfig {
    std::string url      = "http://localhost:9200";
    std::string user     = "elastic";
    std::string password;
    std::string index_articles = "news_articles";
};

struct MinIOConfig {
    std::string endpoint  = "http://localhost:9000";
    std::string access_key;
    std::string secret_key;
    std::string bucket_articles = "article-images";
    std::string bucket_audio    = "article-audio";
};

struct JWTConfig {
    std::string secret;
    std::string issuer         = "ai-news-platform";
    int         access_ttl_sec  = 3600;          // 1 hour
    int         refresh_ttl_sec = 2592000;        // 30 days
    std::string algorithm       = "HS256";
};

struct AIConfig {
    std::string openai_api_key;
    std::string openai_model    = "gpt-4o";
    std::string openai_base_url = "https://api.openai.com/v1";
    std::string newsapi_key;
    std::string tts_provider    = "openai";
    int         max_tokens      = 1024;
    double      temperature     = 0.3;
};

struct ServerConfig {
    std::string host        = "0.0.0.0";
    int         port        = 8080;
    int         threads     = 8;
    bool        ssl_enabled = false;
    std::string ssl_cert;
    std::string ssl_key;
    std::string log_level   = "info";
    int         request_timeout_ms = 30000;
};

struct AppConfig {
    ServerConfig        server;
    DatabaseConfig      database;
    RedisConfig         redis;
    KafkaConfig         kafka;
    ElasticsearchConfig elasticsearch;
    MinIOConfig         minio;
    JWTConfig           jwt;
    AIConfig            ai;

    // ── Load from environment variables (12-factor app) ──────────────────────
    static AppConfig from_env() {
        AppConfig cfg;

        // Server
        if (auto* v = std::getenv("SERVER_PORT"))    cfg.server.port    = std::stoi(v);
        if (auto* v = std::getenv("SERVER_THREADS")) cfg.server.threads = std::stoi(v);
        if (auto* v = std::getenv("LOG_LEVEL"))      cfg.server.log_level = v;

        // Database
        if (auto* v = std::getenv("DB_HOST"))        cfg.database.host     = v;
        if (auto* v = std::getenv("DB_PORT"))        cfg.database.port     = std::stoi(v);
        if (auto* v = std::getenv("DB_NAME"))        cfg.database.name     = v;
        if (auto* v = std::getenv("DB_USER"))        cfg.database.user     = v;
        if (auto* v = std::getenv("DB_PASSWORD"))    cfg.database.password = v;
        if (auto* v = std::getenv("DB_POOL_SIZE"))   cfg.database.pool_size = std::stoi(v);

        // Redis
        if (auto* v = std::getenv("REDIS_HOST"))     cfg.redis.host     = v;
        if (auto* v = std::getenv("REDIS_PORT"))     cfg.redis.port     = std::stoi(v);
        if (auto* v = std::getenv("REDIS_PASSWORD")) cfg.redis.password = v;

        // Kafka
        if (auto* v = std::getenv("KAFKA_BROKERS"))  cfg.kafka.brokers  = v;

        // Elasticsearch
        if (auto* v = std::getenv("ES_URL"))         cfg.elasticsearch.url      = v;
        if (auto* v = std::getenv("ES_USER"))        cfg.elasticsearch.user     = v;
        if (auto* v = std::getenv("ES_PASSWORD"))    cfg.elasticsearch.password = v;

        // MinIO
        if (auto* v = std::getenv("MINIO_ENDPOINT"))   cfg.minio.endpoint   = v;
        if (auto* v = std::getenv("MINIO_ACCESS_KEY"))  cfg.minio.access_key = v;
        if (auto* v = std::getenv("MINIO_SECRET_KEY"))  cfg.minio.secret_key = v;

        // JWT
        if (auto* v = std::getenv("JWT_SECRET"))        cfg.jwt.secret     = v;
        else throw std::runtime_error("JWT_SECRET env var is required");

        // AI
        if (auto* v = std::getenv("OPENAI_API_KEY"))    cfg.ai.openai_api_key = v;
        if (auto* v = std::getenv("NEWSAPI_KEY"))        cfg.ai.newsapi_key   = v;
        if (auto* v = std::getenv("OPENAI_MODEL"))       cfg.ai.openai_model  = v;

        return cfg;
    }
};

// Singleton accessor
inline AppConfig& get_config() {
    static AppConfig instance = AppConfig::from_env();
    return instance;
}

} // namespace news
