#pragma once
// =============================================================================
// ai_controller.hpp  —  REST endpoints for article summarization, quiz, TTS, translation
// =============================================================================
// POST /api/v1/ai/summarize  →  Generate dynamic summary of text
// GET  /api/v1/ai/quiz       →  Generate quiz from article id
// POST /api/v1/ai/translate  →  Translate text
// POST /api/v1/ai/tts        →  Text-to-speech generation (returns MP3 stream)
// =============================================================================
#include <drogon/HttpController.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "ai_service.hpp"
#include "../../api/middleware/auth_middleware.hpp"

namespace news::ai {

class AIController
    : public drogon::HttpController<AIController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AIController::summarize_text, "/api/v1/ai/summarize", drogon::Post, "news::AuthMiddleware");
        ADD_METHOD_TO(AIController::get_quiz,       "/api/v1/ai/quiz",      drogon::Get);
        ADD_METHOD_TO(AIController::translate_text, "/api/v1/ai/translate", drogon::Post);
        ADD_METHOD_TO(AIController::text_to_speech, "/api/v1/ai/tts",       drogon::Post);
    METHOD_LIST_END

    // ── POST /api/v1/ai/summarize ────────────────────────────────────────────
    void summarize_text(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto body_json = nlohmann::json::parse(req->body(), nullptr, false);
        if (body_json.is_discarded() || !body_json.contains("content")) {
            cb(error_response("Invalid request. 'content' field is required."));
            return;
        }

        std::string title   = body_json.value("title", "Untitled Article");
        std::string content = body_json.value("content", "");
        std::string lang    = body_json.value("language", "en");

        ai_.summarize(title, content, lang,
            [cb=std::move(cb)](std::optional<SummaryResult> res) mutable {
                if (!res) {
                    cb(error_response("Failed to generate AI summary", 500));
                    return;
                }
                nlohmann::json j = {
                    {"success", true},
                    {"data", {
                        {"one_line",        res->one_line},
                        {"thirty_sec",      res->thirty_sec},
                        {"one_min",         res->one_min},
                        {"detailed",        res->detailed},
                        {"why_it_matters",  res->why_it_matters},
                        {"timeline",        res->timeline},
                        {"sentiment",       res->sentiment},
                        {"sentiment_score", res->sentiment_score},
                        {"is_fake_news",    res->is_fake_news},
                        {"fake_score",      res->fake_score},
                        {"bias_score",      res->bias_score},
                        {"key_points",      res->key_points},
                        {"important_facts", res->important_facts}
                    }}
                };
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(j.dump());
                cb(resp);
            });
    }

    // ── GET /api/v1/ai/quiz?article_id=... ───────────────────────────────────
    void get_quiz(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params = req->getParameters();
        std::string article_id = params.count("article_id") ? params.at("article_id") : "";
        if (article_id.empty()) {
            cb(error_response("article_id parameter is required"));
            return;
        }

        // Fetch article content from DB
        auto db = drogon::app().getDbClient();
        db->execSqlAsync(
            "SELECT title, content FROM news_articles WHERE id=$1",
            [=, cb=std::move(cb)](const drogon::orm::Result& r) mutable {
                if (r.empty()) {
                    cb(error_response("Article not found", 404));
                    return;
                }
                std::string title   = r[0]["title"].as<std::string>();
                std::string content = r[0]["content"].as<std::string>();
                
                ai_.generate_quiz(title + "\n\n" + content,
                    [cb=std::move(cb)](nlohmann::json quiz) mutable {
                        nlohmann::json j = {{"success", true}, {"data", quiz}};
                        auto resp = drogon::HttpResponse::newHttpResponse();
                        resp->setStatusCode(drogon::k200OK);
                        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                        resp->setBody(j.dump());
                        cb(resp);
                    });
            },
            [cb=std::move(cb)](const drogon::orm::DrogonDbException& e) mutable {
                cb(error_response(e.base().what(), 500));
            }, article_id);
    }

    // ── POST /api/v1/ai/translate ────────────────────────────────────────────
    void translate_text(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto body_json = nlohmann::json::parse(req->body(), nullptr, false);
        if (body_json.is_discarded() || !body_json.contains("text") || !body_json.contains("target_lang")) {
            cb(error_response("Invalid request. 'text' and 'target_lang' fields are required."));
            return;
        }

        std::string text        = body_json.value("text", "");
        std::string target_lang = body_json.value("target_lang", "en");

        ai_.translate(text, target_lang,
            [cb=std::move(cb)](std::string translated) mutable {
                nlohmann::json j = {{"success", true}, {"translated_text", translated}};
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(j.dump());
                cb(resp);
            });
    }

    // ── POST /api/v1/ai/tts ──────────────────────────────────────────────────
    void text_to_speech(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto body_json = nlohmann::json::parse(req->body(), nullptr, false);
        if (body_json.is_discarded() || !body_json.contains("text")) {
            cb(error_response("Invalid request. 'text' field is required."));
            return;
        }

        std::string text  = body_json.value("text", "");
        std::string voice = body_json.value("voice", "alloy");

        ai_.text_to_speech(text, voice,
            [cb=std::move(cb)](std::vector<uint8_t> audio_data) mutable {
                if (audio_data.empty()) {
                    cb(error_response("Failed to generate text-to-speech audio", 500));
                    return;
                }
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeHeader("audio/mpeg");
                resp->setBody(std::string(audio_data.begin(), audio_data.end()));
                cb(resp);
            });
    }

private:
    AIService ai_;

    static drogon::HttpResponsePtr error_response(const std::string& msg, int status = 400) {
        nlohmann::json j = {{"success", false}, {"error", msg}};
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(status));
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(j.dump());
        return resp;
    }
};

} // namespace news::ai
