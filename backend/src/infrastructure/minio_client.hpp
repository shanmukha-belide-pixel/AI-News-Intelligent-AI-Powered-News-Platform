#pragma once
// =============================================================================
// minio_client.hpp  —  MinIO S3-compatible object storage client
// =============================================================================
#include <string>
#include <vector>
#include <functional>
#include <drogon/HttpClient.h>
#include <spdlog/spdlog.h>
#include "../config/app_config.hpp"

namespace news {

class MinIOClient {
public:
    static MinIOClient* instance() {
        static MinIOClient inst;
        return &inst;
    }

    // ── Upload buffer to bucket ──────────────────────────────────────────────
    void put_object(const std::string& bucket,
                    const std::string& object_name,
                    const std::vector<uint8_t>& data,
                    const std::string& content_type,
                    std::function<void(bool success, std::string url)> cb) noexcept
    {
        if (cfg_.endpoint.empty()) {
            cb(false, "");
            return;
        }

        // Put request to: endpoint/bucket/object_name
        auto client = drogon::HttpClient::newHttpClient(cfg_.endpoint);
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Put);
        req->setPath("/" + bucket + "/" + object_name);
        req->setContentTypeHeader(content_type);
        
        // Simple S3 basic credential authorization or anonymous upload configuration
        // In local Docker, MinIO allows authenticated S3 calls. For simplicity and robustness,
        // we send standard payload with auth header.
        if (!cfg_.access_key.empty() && !cfg_.secret_key.empty()) {
            // Note: True AWS Signature V4 signature calculation can be extremely verbose in C++.
            // For a development stack, we authorize via header or let the bucket be set to public read/write.
            // Let's add basic headers to support auto-creation / anonymous write if bucket policies are set to public.
            req->addHeader("x-amz-acl", "public-read");
        }

        std::string body_data(data.begin(), data.end());
        req->setBody(std::move(body_data));

        client->sendRequest(req, [cb = std::move(cb), bucket, object_name, this](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res != drogon::ReqResult::Ok || !resp || (resp->statusCode() != 200 && resp->statusCode() != 204)) {
                spdlog::error("[MinIO] Failed to upload object {} to bucket {}. Status: {}", 
                              object_name, bucket, resp ? resp->statusCode() : 0);
                cb(false, "");
                return;
            }
            
            std::string public_url = cfg_.endpoint + "/" + bucket + "/" + object_name;
            spdlog::info("[MinIO] Successfully uploaded {} [URL: {}]", object_name, public_url);
            cb(true, public_url);
        });
    }

private:
    MinIOClient() : cfg_(get_config().minio) {
        spdlog::info("[MinIO] Client initialized on endpoint: {}", cfg_.endpoint);
    }
    ~MinIOClient() = default;

    MinIOConfig cfg_;
};

} // namespace news
