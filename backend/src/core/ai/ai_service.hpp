#pragma once
// =============================================================================
// ai_service.hpp  —  OpenAI GPT-4o integration: summarize, sentiment, chat
// =============================================================================
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>
#include <drogon/HttpClient.h>
#include <spdlog/spdlog.h>
#include "../../config/app_config.hpp"

namespace news::ai {

struct SummaryResult {
    std::string one_line;
    std::string thirty_sec;
    std::string one_min;
    std::string detailed;
    std::vector<std::string> key_points;
    std::vector<std::string> pros;
    std::vector<std::string> cons;
    std::vector<std::string> important_facts;
    std::string why_it_matters;
    std::string timeline;
    std::string sentiment;      // positive/negative/neutral/mixed
    double sentiment_score = 0.0;
    bool   is_fake_news    = false;
    double fake_score      = 0.0;
    double bias_score      = 0.0;
    std::string model_used;
    int token_count = 0;
};

using SummaryCallback = std::function<void(std::optional<SummaryResult>)>;
using ChatCallback    = std::function<void(const std::string& chunk, bool done)>;

class AIService {
public:
    explicit AIService(const AIConfig& cfg = get_config().ai)
        : cfg_(cfg) {}

    // ── Full article summarization (structured JSON response) ─────────────────
    void summarize(const std::string& title,
                   const std::string& content,
                   const std::string& language,
                   SummaryCallback callback)
    {
        std::string prompt = build_summary_prompt(title, content, language);
        call_openai(prompt, cfg_.max_tokens, false,
            [callback](const std::string& raw, bool) {
                callback(parse_summary(raw));
            });
    }

    // ── Streaming AI chat (for UI chat feature) ────────────────────────────────
    void chat_stream(const std::string& article_context,
                     const std::vector<nlohmann::json>& history,
                     const std::string& user_message,
                     ChatCallback callback)
    {
        nlohmann::json messages = nlohmann::json::array();
        messages.push_back({
            {"role", "system"},
            {"content", "You are an expert AI news analyst. You help users understand news articles. "
                        "Be concise, accurate, and cite facts from the article context provided.\n\n"
                        "ARTICLE CONTEXT:\n" + article_context}
        });
        for (const auto& h : history) messages.push_back(h);
        messages.push_back({{"role","user"},{"content",user_message}});

        nlohmann::json payload = {
            {"model",       cfg_.openai_model},
            {"messages",    messages},
            {"max_tokens",  1024},
            {"temperature", 0.4},
            {"stream",      true}
        };

        stream_openai(payload.dump(), callback);
    }

    // ── Translate article summary ──────────────────────────────────────────────
    void translate(const std::string& text,
                   const std::string& target_lang,
                   std::function<void(std::string)> callback)
    {
        std::string prompt = "Translate the following news summary to " + target_lang +
                             ". Keep it natural and concise:\n\n" + text;
        call_openai(prompt, 512, false,
            [callback](const std::string& out, bool) { callback(out); });
    }

    // ── Generate quiz from article ─────────────────────────────────────────────
    void generate_quiz(const std::string& article_text,
                       std::function<void(nlohmann::json)> callback)
    {
        std::string prompt =
            "Create 5 multiple-choice quiz questions from this article. "
            "Return JSON array: [{\"question\":\"...\",\"options\":[\"A\",\"B\",\"C\",\"D\"],\"answer\":\"A\",\"explanation\":\"...\"}]\n\n"
            + article_text;
        call_openai(prompt, 1024, false,
            [callback](const std::string& raw, bool) {
                try { callback(nlohmann::json::parse(raw)); }
                catch (...) { callback(nlohmann::json::array()); }
            });
    }

    // ── TTS (text-to-speech) via OpenAI ──────────────────────────────────────
    void text_to_speech(const std::string& text,
                        const std::string& voice,  // alloy, echo, fable, onyx, nova, shimmer
                        std::function<void(std::vector<uint8_t>)> callback)
    {
        nlohmann::json payload = {
            {"model", "tts-1"},
            {"input", text.substr(0, 4096)},
            {"voice", voice.empty() ? "alloy" : voice}
        };

        auto client = drogon::HttpClient::newHttpClient("https://api.openai.com");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/v1/audio/speech");
        req->addHeader("Authorization", "Bearer " + cfg_.openai_api_key);
        req->addHeader("Content-Type", "application/json");
        req->setBody(payload.dump());

        client->sendRequest(req,
            [callback](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res == drogon::ReqResult::Ok && resp) {
                    auto& body = resp->body();
                    callback(std::vector<uint8_t>(body.begin(), body.end()));
                } else {
                    callback({});
                }
            });
    }

private:
    AIConfig cfg_;

    // ── OpenAI Chat Completions (non-streaming) ───────────────────────────────
    void call_openai(const std::string& prompt,
                     int max_tokens,
                     bool json_mode,
                     std::function<void(const std::string&, bool)> callback)
    {
        nlohmann::json payload = {
            {"model",       cfg_.openai_model},
            {"messages",    {{{"role","user"},{"content",prompt}}}},
            {"max_tokens",  max_tokens},
            {"temperature", cfg_.temperature}
        };
        if (json_mode) payload["response_format"] = {{"type","json_object"}};

        auto client = drogon::HttpClient::newHttpClient("https://api.openai.com");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/v1/chat/completions");
        req->addHeader("Authorization", "Bearer " + cfg_.openai_api_key);
        req->addHeader("Content-Type", "application/json");
        req->setBody(payload.dump());

        client->sendRequest(req,
            [callback](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res != drogon::ReqResult::Ok || !resp) {
                    spdlog::error("[AI] OpenAI request failed");
                    callback("", true); return;
                }
                try {
                    auto j = nlohmann::json::parse(resp->body());
                    std::string content = j["choices"][0]["message"]["content"].get<std::string>();
                    callback(content, true);
                } catch (const std::exception& e) {
                    spdlog::error("[AI] Parse error: {}", e.what());
                    callback("", true);
                }
            });
    }

    // ── OpenAI streaming ─────────────────────────────────────────────────────
    void stream_openai(const std::string& payload_str, ChatCallback callback) {
        auto client = drogon::HttpClient::newHttpClient("https://api.openai.com");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/v1/chat/completions");
        req->addHeader("Authorization", "Bearer " + cfg_.openai_api_key);
        req->addHeader("Content-Type", "application/json");
        req->setBody(payload_str);

        client->sendRequest(req,
            [callback](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res != drogon::ReqResult::Ok || !resp) {
                    callback("", true); return;
                }
                // Parse SSE stream
                std::string body = resp->body();
                std::istringstream ss(body);
                std::string line;
                while (std::getline(ss, line)) {
                    if (line.substr(0,6) != "data: ") continue;
                    std::string data = line.substr(6);
                    if (data == "[DONE]") { callback("", true); return; }
                    try {
                        auto j = nlohmann::json::parse(data);
                        auto& delta = j["choices"][0]["delta"];
                        if (delta.contains("content")) {
                            callback(delta["content"].get<std::string>(), false);
                        }
                    } catch (...) {}
                }
                callback("", true);
            });
    }

    // ── Prompts ───────────────────────────────────────────────────────────────
    static std::string build_summary_prompt(const std::string& title,
                                            const std::string& content,
                                            const std::string& /*lang*/)
    {
        return R"(Analyze this news article and return a JSON object with exactly these fields:
{
  "one_line": "One sentence summary (max 20 words)",
  "thirty_sec": "Summary readable in 30 seconds (80-100 words)",
  "one_min": "Summary readable in 1 minute (200-220 words)",
  "detailed": "Detailed explanation (400-500 words)",
  "key_points": ["point 1", "point 2", "point 3", "point 4", "point 5"],
  "pros": ["pro 1", "pro 2"],
  "cons": ["con 1", "con 2"],
  "important_facts": ["fact 1", "fact 2", "fact 3"],
  "why_it_matters": "Why this news matters to readers (2-3 sentences)",
  "timeline": "Brief timeline of events if applicable",
  "sentiment": "positive|negative|neutral|mixed",
  "sentiment_score": 0.0,
  "is_fake_news": false,
  "fake_score": 0.0,
  "bias_score": 0.0
}

ARTICLE TITLE: )" + title + R"(

ARTICLE CONTENT:
)" + content.substr(0, 3000);
    }

    static std::optional<SummaryResult> parse_summary(const std::string& raw) {
        try {
            // Strip markdown code fences if present
            std::string json_str = raw;
            if (auto p = json_str.find("```json"); p != std::string::npos) {
                json_str = json_str.substr(p + 7);
                json_str = json_str.substr(0, json_str.rfind("```"));
            }
            auto j = nlohmann::json::parse(json_str);

            SummaryResult r;
            r.one_line        = j.value("one_line", "");
            r.thirty_sec      = j.value("thirty_sec", "");
            r.one_min         = j.value("one_min", "");
            r.detailed        = j.value("detailed", "");
            r.why_it_matters  = j.value("why_it_matters", "");
            r.timeline        = j.value("timeline", "");
            r.sentiment       = j.value("sentiment", "neutral");
            r.sentiment_score = j.value("sentiment_score", 0.0);
            r.is_fake_news    = j.value("is_fake_news", false);
            r.fake_score      = j.value("fake_score", 0.0);
            r.bias_score      = j.value("bias_score", 0.0);

            if (j.contains("key_points") && j["key_points"].is_array())
                for (auto& kp : j["key_points"]) r.key_points.push_back(kp.get<std::string>());
            if (j.contains("pros") && j["pros"].is_array())
                for (auto& p : j["pros"]) r.pros.push_back(p.get<std::string>());
            if (j.contains("cons") && j["cons"].is_array())
                for (auto& c : j["cons"]) r.cons.push_back(c.get<std::string>());
            if (j.contains("important_facts") && j["important_facts"].is_array())
                for (auto& f : j["important_facts"]) r.important_facts.push_back(f.get<std::string>());

            return r;
        } catch (const std::exception& e) {
            spdlog::error("[AI] Summary parse error: {}", e.what());
            return std::nullopt;
        }
    }
};

} // namespace news::ai
