#pragma once
// =============================================================================
// chat_controller.hpp  —  REST endpoint for AI analysis and streaming chat
// =============================================================================
// POST /api/v1/ai/chat  →  Start or continue AI chat session with article context
// =============================================================================
#include <drogon/HttpController.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "ai_service.hpp"
#include "../../api/middleware/auth_middleware.hpp"

namespace news::ai {

class ChatController
    : public drogon::HttpController<ChatController>
{
public:
    METHOD_LIST_BEGIN
        // Authenticated chat endpoint
        ADD_METHOD_TO(ChatController::chat, "/api/v1/ai/chat", drogon::Post, "news::AuthMiddleware");
    METHOD_LIST_END

    // ── POST /api/v1/ai/chat ─────────────────────────────────────────────────
    void chat(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto body_json = nlohmann::json::parse(req->body(), nullptr, false);
        if (body_json.is_discarded() || !body_json.contains("message")) {
            cb(error_response("Invalid request format. 'message' field is required."));
            return;
        }

        std::string user_msg       = body_json.value("message", "");
        std::string article_ctx    = body_json.value("article_context", "");
        
        std::vector<nlohmann::json> history;
        if (body_json.contains("history") && body_json["history"].is_array()) {
            for (const auto& msg : body_json["history"]) {
                history.push_back(msg);
            }
        }

        auto resp = drogon::HttpResponse::newAsyncStreamReader(
            [user_msg, article_ctx, history, ai = ai_](drogon::ResponseStream* stream) mutable {
                ai.chat_stream(article_ctx, history, user_msg,
                    [stream](const std::string& chunk, bool done) {
                        if (done) {
                            stream->close();
                            return;
                        }
                        nlohmann::json delta = {{"delta", chunk}, {"done", false}};
                        stream->send(delta.dump() + "\n");
                    });
            });
        
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        cb(resp);
    }

private:
    AIService ai_;

    static drogon::HttpResponsePtr error_response(const std::string& msg) {
        nlohmann::json j = {{"success", false}, {"error", msg}};
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(j.dump());
        return resp;
    }
};

} // namespace news::ai
