#pragma once
// =============================================================================
// elasticsearch_client.hpp  —  Async Elasticsearch client for semantic & full-text search
// =============================================================================
#include <string>
#include <functional>
#include <memory>
#include <drogon/HttpClient.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "../config/app_config.hpp"

namespace news {

class ElasticsearchClient {
public:
    static ElasticsearchClient* instance() {
        static ElasticsearchClient inst;
        return &inst;
    }

    // ── Search articles with boolean query filters ────────────────────────────
    void search(const std::string& q,
                const std::string& category,
                const std::string& from_dt,
                const std::string& to_dt,
                const std::string& scope,
                int limit,
                std::function<void(nlohmann::json)> cb) noexcept
    {
        if (cfg_.url.empty()) {
            spdlog::warn("[Elasticsearch] Client not configured. Returning empty results.");
            cb(nlohmann::json::array());
            return;
        }

        // Build Elasticsearch DSL query
        nlohmann::json must_clause = nlohmann::json::array();
        nlohmann::json filter_clause = nlohmann::json::array();

        // 1. Text Search matching title & content
        must_clause.push_back({
            {"multi_match", {
                {"query", q},
                {"fields", nlohmann::json::array({"title^3", "content^1", "excerpt^2"})},
                {"fuzziness", "AUTO"}
            }}
        });

        // 2. Category filter
        if (!category.empty()) {
            filter_clause.push_back({{"term", {{"category.keyword", category}}}});
        }

        // 3. Scope filter (international / national / state)
        if (!scope.empty()) {
            filter_clause.push_back({{"term", {{"scope.keyword", scope}}}});
        }

        // 4. Date range filter
        if (!from_dt.empty() || !to_dt.empty()) {
            nlohmann::json range = nlohmann::json::object();
            if (!from_dt.empty()) range["gte"] = from_dt;
            if (!to_dt.empty())   range["lte"] = to_dt;
            filter_clause.push_back({{"range", {{"published_at", range}}}});
        }

        // Aggregate DSL
        nlohmann::json query_body = {
            {"size", limit},
            {"query", {
                {"bool", {
                    {"must", must_clause},
                    {"filter", filter_clause}
                }}
            }},
            // Highlight matching text fragments
            {"highlight", {
                {"fields", {
                    {"title", nlohmann::json::object()},
                    {"excerpt", nlohmann::json::object()}
                }}
            }}
        };

        // Create HTTP client referencing Elasticsearch host
        auto client = drogon::HttpClient::newHttpClient(cfg_.url);
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/" + cfg_.index_articles + "/_search");
        req->addHeader("Content-Type", "application/json");
        
        if (!cfg_.user.empty() && !cfg_.password.empty()) {
            req->addHeader("Authorization", "Basic " + drogon::utils::base64Encode(cfg_.user + ":" + cfg_.password));
        }
        
        req->setBody(query_body.dump());

        client->sendRequest(req, [cb = std::move(cb)](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res != drogon::ReqResult::Ok || !resp || resp->statusCode() != 200) {
                spdlog::warn("[Elasticsearch] Search query failed. Check connection.");
                // Return fallback: empty array
                cb(nlohmann::json::array());
                return;
            }

            try {
                auto body_json = nlohmann::json::parse(resp->body());
                nlohmann::json results = nlohmann::json::array();
                
                auto& hits = body_json["hits"]["hits"];
                for (const auto& hit : hits) {
                    auto& source = hit["_source"];
                    nlohmann::json item = {
                        {"id",           hit["_id"].get<std::string>()},
                        {"title",        source.value("title", "")},
                        {"excerpt",      source.value("excerpt", "")},
                        {"url",          source.value("url", "")},
                        {"image_url",    source.value("image_url", "")},
                        {"publisher",    source.value("publisher", "")},
                        {"category",     source.value("category", "")},
                        {"published_at", source.value("published_at", "")},
                        {"score",        hit["_score"].get<double>()}
                    };
                    
                    // Add ES highlight fragments
                    if (hit.contains("highlight")) {
                        item["highlight"] = hit["highlight"];
                    }
                    
                    results.push_back(item);
                }
                cb(results);
            } catch (const std::exception& e) {
                spdlog::error("[Elasticsearch] JSON parse error: {}", e.what());
                cb(nlohmann::json::array());
            }
        });
    }

private:
    ElasticsearchClient() : cfg_(get_config().elasticsearch) {
        spdlog::info("[Elasticsearch] Client initialized on host: {}", cfg_.url);
    }
    ~ElasticsearchClient() = default;

    ElasticsearchConfig cfg_;
};

} // namespace news
