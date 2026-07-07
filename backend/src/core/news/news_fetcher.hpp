#pragma once
// =============================================================================
// news_fetcher.hpp  —  3-tier real news: International · National · State
// =============================================================================
// INTERNATIONAL  : BBC, Reuters, AP, Al Jazeera, France24, DW, Guardian, NYT
// NATIONAL (India): Times of India, NDTV, The Hindu, India Today, HT, IE
// STATE (India)   : Eenadu, Dinamalar, Mathrubhumi, Punjab Kesari, Lokmat +30
// Other APIs      : NewsAPI.org, GNews, NYT, The Guardian (key-based)
// =============================================================================
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <atomic>
#include <nlohmann/json.hpp>
#include <drogon/HttpClient.h>
#include <spdlog/spdlog.h>
#include "config/app_config.hpp"

namespace news {

// =============================================================================
// Data model
// =============================================================================
enum class NewsScope { International, National, State };

struct RawArticle {
    std::string external_id;
    std::string title;
    std::string content;
    std::string excerpt;
    std::string url;
    std::string image_url;
    std::string author;
    std::string publisher;
    std::string publisher_icon;
    std::string category;        // world / technology / business / etc.
    std::string language;        // en / hi / ta / te / mr / kn / ...
    std::string country;         // IN / US / GB / ...
    std::string state;           // Maharashtra / Tamil Nadu / Punjab ...
    std::string published_at;    // ISO-8601
    std::vector<std::string> tags;
    NewsScope   scope  = NewsScope::International;
    bool        is_breaking = false;
};

using ArticleCallback = std::function<void(std::vector<RawArticle>)>;

// =============================================================================
// Utility: minimal RSS XML parser (no extra dependency)
// =============================================================================
namespace rss_parser {
    inline std::string extract(const std::string& src,
                               const std::string& open,
                               const std::string& close)
    {
        auto s = src.find(open);
        if (s == std::string::npos) return "";
        s += open.size();
        auto e = src.find(close, s);
        if (e == std::string::npos) return "";
        return src.substr(s, e - s);
    }

    inline std::string strip_cdata(std::string s) {
        if (s.size() >= 12 && s.substr(0, 9) == "<![CDATA[")
            s = s.substr(9, s.size() - 12);
        // Strip HTML tags
        std::string out; bool in_tag = false;
        for (char c : s) {
            if (c == '<') in_tag = true;
            else if (c == '>') in_tag = false;
            else if (!in_tag) out += c;
        }
        return out;
    }

    inline std::vector<RawArticle> parse(const std::string& xml,
                                         const std::string& publisher,
                                         const std::string& category,
                                         const std::string& language,
                                         const std::string& country,
                                         const std::string& state,
                                         NewsScope scope)
    {
        std::vector<RawArticle> articles;
        std::string rem = xml;
        while (true) {
            auto s = rem.find("<item>");
            if (s == std::string::npos) s = rem.find("<entry>");
            if (s == std::string::npos) break;

            std::string close_tag = (rem[s+1] == 'i') ? "</item>" : "</entry>";
            auto e = rem.find(close_tag, s);
            if (e == std::string::npos) break;

            std::string item = rem.substr(s, e - s);
            rem = rem.substr(e + close_tag.size());

            RawArticle a;
            a.title        = strip_cdata(extract(item, "<title>", "</title>"));
            a.url          = strip_cdata(extract(item, "<link>", "</link>"));
            if (a.url.empty()) a.url = strip_cdata(extract(item, "<link href=\"", "\""));
            a.excerpt      = strip_cdata(extract(item, "<description>", "</description>"));
            if (a.excerpt.empty()) a.excerpt = strip_cdata(extract(item, "<summary>", "</summary>"));
            a.published_at = extract(item, "<pubDate>", "</pubDate>");
            if (a.published_at.empty()) a.published_at = extract(item, "<published>", "</published>");
            a.publisher    = publisher;
            a.category     = category;
            a.language     = language;
            a.country      = country;
            a.state        = state;
            a.scope        = scope;
            a.external_id  = "rss_" + a.url.substr(0, 100);

            // Image
            a.image_url = extract(item, "<media:thumbnail url=\"", "\"");
            if (a.image_url.empty()) a.image_url = extract(item, "<enclosure url=\"", "\"");
            if (a.image_url.empty()) a.image_url = extract(item, "<media:content url=\"", "\"");

            if (!a.title.empty() && !a.url.empty())
                articles.push_back(std::move(a));
        }
        return articles;
    }
} // namespace rss_parser

// =============================================================================
// Base Provider
// =============================================================================
class NewsProvider {
public:
    virtual ~NewsProvider() = default;
    virtual std::string name()  const = 0;
    virtual NewsScope   scope() const = 0;
    virtual void fetch(const std::string& category,
                       const std::string& country,
                       int page_size,
                       ArticleCallback callback) = 0;
};

// =============================================================================
// ─── TIER 1: INTERNATIONAL RSS PROVIDERS ────────────────────────────────────
// =============================================================================
struct RssFeedDef {
    std::string name;
    std::string host;
    std::string path;
    std::string category;
    std::string language;
    std::string country;
    std::string state;
    NewsScope   scope;
};

// All free RSS feeds — grouped by scope
static const std::vector<RssFeedDef> INTERNATIONAL_FEEDS = {
    // ── BBC ──────────────────────────────────────────────────────────
    {"BBC World News",    "https://feeds.bbci.co.uk", "/news/world/rss.xml",                       "world",        "en","GB","",NewsScope::International},
    {"BBC Technology",    "https://feeds.bbci.co.uk", "/news/technology/rss.xml",                  "technology",   "en","GB","",NewsScope::International},
    {"BBC Business",      "https://feeds.bbci.co.uk", "/news/business/rss.xml",                    "business",     "en","GB","",NewsScope::International},
    {"BBC Science",       "https://feeds.bbci.co.uk", "/news/science_and_environment/rss.xml",     "science",      "en","GB","",NewsScope::International},
    {"BBC Health",        "https://feeds.bbci.co.uk", "/news/health/rss.xml",                      "health",       "en","GB","",NewsScope::International},
    {"BBC Politics",      "https://feeds.bbci.co.uk", "/news/politics/rss.xml",                    "politics",     "en","GB","",NewsScope::International},
    // ── Reuters ──────────────────────────────────────────────────────
    {"Reuters World",     "https://feeds.reuters.com","/reuters/worldNews",                        "world",        "en","US","",NewsScope::International},
    {"Reuters Business",  "https://feeds.reuters.com","/reuters/businessNews",                     "business",     "en","US","",NewsScope::International},
    {"Reuters Tech",      "https://feeds.reuters.com","/reuters/technologyNews",                   "technology",   "en","US","",NewsScope::International},
    // ── AP News ──────────────────────────────────────────────────────
    {"AP Top News",       "https://feeds.apnews.com", "/apf-topnews",                              "world",        "en","US","",NewsScope::International},
    {"AP Technology",     "https://feeds.apnews.com", "/apf-technology",                           "technology",   "en","US","",NewsScope::International},
    {"AP Business",       "https://feeds.apnews.com", "/apf-business",                             "business",     "en","US","",NewsScope::International},
    {"AP Sports",         "https://feeds.apnews.com", "/apf-sports",                               "sports",       "en","US","",NewsScope::International},
    // ── Al Jazeera ───────────────────────────────────────────────────
    {"Al Jazeera",        "https://www.aljazeera.com","/xml/rss/all.xml",                          "world",        "en","QA","",NewsScope::International},
    // ── France 24 ────────────────────────────────────────────────────
    {"France24 World",    "https://www.france24.com", "/en/rss",                                   "world",        "en","FR","",NewsScope::International},
    // ── DW ───────────────────────────────────────────────────────────
    {"DW World",          "https://rss.dw.com",       "/rdf/rss-en-world",                         "world",        "en","DE","",NewsScope::International},
    {"DW Business",       "https://rss.dw.com",       "/rdf/rss-en-bus",                           "business",     "en","DE","",NewsScope::International},
    // ── Tech ─────────────────────────────────────────────────────────
    {"TechCrunch",        "https://techcrunch.com",   "/feed/",                                    "technology",   "en","US","",NewsScope::International},
    {"Ars Technica",      "https://feeds.arstechnica.com","/arstechnica/index",                    "technology",   "en","US","",NewsScope::International},
    {"The Verge",         "https://www.theverge.com", "/rss/index.xml",                            "technology",   "en","US","",NewsScope::International},
    {"Wired",             "https://www.wired.com",    "/feed/rss",                                 "technology",   "en","US","",NewsScope::International},
    {"MIT Tech Review",   "https://www.technologyreview.com","/topstories.rss",                    "technology",   "en","US","",NewsScope::International},
    // ── Science ──────────────────────────────────────────────────────
    {"Nature News",       "https://www.nature.com",   "/nature.rss",                               "science",      "en","GB","",NewsScope::International},
    {"Science Daily",     "https://www.sciencedaily.com","/rss/all.xml",                           "science",      "en","US","",NewsScope::International},
    // ── NPR ──────────────────────────────────────────────────────────
    {"NPR Top Stories",   "https://feeds.npr.org",    "/1001/rss.xml",                             "world",        "en","US","",NewsScope::International},
};

// =============================================================================
// TIER 2: NATIONAL — INDIA (English + Hindi)
// =============================================================================
static const std::vector<RssFeedDef> NATIONAL_INDIA_FEEDS = {
    // ── English National ────────────────────────────────────────────
    {"Times of India",          "https://timesofindia.indiatimes.com","/rssfeedstopstories.cms",           "world",      "en","IN","",NewsScope::National},
    {"Times of India Tech",     "https://timesofindia.indiatimes.com","/rssfeeds/5880659.cms",             "technology", "en","IN","",NewsScope::National},
    {"Times of India Business", "https://timesofindia.indiatimes.com","/rssfeeds/1898055.cms",             "business",   "en","IN","",NewsScope::National},
    {"Times of India Sports",   "https://timesofindia.indiatimes.com","/rssfeeds/4719161.cms",             "sports",     "en","IN","",NewsScope::National},
    {"NDTV India",              "https://feeds.feedburner.com",        "/ndtvnews-top-stories",            "world",      "en","IN","",NewsScope::National},
    {"NDTV Technology",         "https://feeds.feedburner.com",        "/NdtvNewsGadgets360-LatestNews",   "technology", "en","IN","",NewsScope::National},
    {"NDTV Business",           "https://feeds.feedburner.com",        "/ndtv/6BGX",                       "business",   "en","IN","",NewsScope::National},
    {"The Hindu",               "https://www.thehindu.com",            "/news/national/?service=rss",       "world",      "en","IN","",NewsScope::National},
    {"The Hindu Business",      "https://www.thehindu.com",            "/business/?service=rss",            "business",   "en","IN","",NewsScope::National},
    {"The Hindu Science",       "https://www.thehindu.com",            "/sci-tech/science/?service=rss",    "science",    "en","IN","",NewsScope::National},
    {"India Today",             "https://www.indiatoday.in",           "/rss/home",                         "world",      "en","IN","",NewsScope::National},
    {"India Today Tech",        "https://www.indiatoday.in",           "/rss/technology",                   "technology", "en","IN","",NewsScope::National},
    {"Hindustan Times",         "https://www.hindustantimes.com",      "/rss/topnews/rssfeed.xml",          "world",      "en","IN","",NewsScope::National},
    {"The Indian Express",      "https://indianexpress.com",           "/feed/",                            "world",      "en","IN","",NewsScope::National},
    {"LiveMint",                "https://www.livemint.com",            "/rss/news.xml",                     "business",   "en","IN","",NewsScope::National},
    {"Economic Times",          "https://economictimes.indiatimes.com","/rssfeedstopstories.cms",           "business",   "en","IN","",NewsScope::National},
    {"Business Standard",       "https://www.business-standard.com",   "/rss/home_page_top_stories.rss",   "business",   "en","IN","",NewsScope::National},
    {"Financial Express",       "https://www.financialexpress.com",    "/feed/",                            "business",   "en","IN","",NewsScope::National},
    // ── Hindi National ─────────────────────────────────────────────
    {"Dainik Jagran",           "https://www.jagran.com",              "/rss/national.xml",                 "world",      "hi","IN","",NewsScope::National},
    {"Amar Ujala",              "https://www.amarujala.com",           "/rss/breaking-news.xml",            "world",      "hi","IN","",NewsScope::National},
    {"Navbharat Times",         "https://navbharattimes.indiatimes.com","/rssfeedstopstories.cms",          "world",      "hi","IN","",NewsScope::National},
    {"Bhaskar",                 "https://www.bhaskar.com",             "/rss-v1-0/language/hindi/state/national.xml","world","hi","IN","",NewsScope::National},
    {"NDTV India Hindi",        "https://feeds.feedburner.com",        "/ndtv-khaskhabar-news",             "world",      "hi","IN","",NewsScope::National},
};

// =============================================================================
// TIER 3: STATE-LEVEL — INDIA (29 states covered)
// =============================================================================
static const std::vector<RssFeedDef> STATE_INDIA_FEEDS = {
    // ── Andhra Pradesh / Telangana ──────────────────────────────────
    {"Eenadu",              "https://www.eenadu.net",     "/rss/telangana.xml",          "world","te","IN","Telangana",     NewsScope::State},
    {"Sakshi Post",         "https://english.sakshi.com", "/feed",                       "world","te","IN","Andhra Pradesh",NewsScope::State},
    {"Andhra Jyothy",       "https://www.andhrajyothy.com","/rss/latest.xml",            "world","te","IN","Andhra Pradesh",NewsScope::State},
    // ── Tamil Nadu ─────────────────────────────────────────────────
    {"Dinamalar",           "https://www.dinamalar.com",  "/rss/feeds.asp",              "world","ta","IN","Tamil Nadu",    NewsScope::State},
    {"Dinamani",            "https://www.dinamani.com",   "/rss/feeds.asp",              "world","ta","IN","Tamil Nadu",    NewsScope::State},
    {"The Hindu Chennai",   "https://www.thehindu.com",   "/news/cities/chennai/?service=rss","world","en","IN","Tamil Nadu",NewsScope::State},
    // ── Kerala ─────────────────────────────────────────────────────
    {"Mathrubhumi",         "https://www.mathrubhumi.com","/rss/news.xml",               "world","ml","IN","Kerala",       NewsScope::State},
    {"Malayala Manorama",   "https://www.manoramaonline.com","/rss/news.xml",            "world","ml","IN","Kerala",       NewsScope::State},
    {"The Hindu Thiruvananthapuram","https://www.thehindu.com","/news/cities/Thiruvananthapuram/?service=rss","world","en","IN","Kerala",NewsScope::State},
    // ── Karnataka ──────────────────────────────────────────────────
    {"Prajavani",           "https://www.prajavani.net",  "/feed",                       "world","kn","IN","Karnataka",    NewsScope::State},
    {"Vijaya Karnataka",    "https://vijayakarnataka.com","/rss",                        "world","kn","IN","Karnataka",    NewsScope::State},
    {"The Hindu Bengaluru", "https://www.thehindu.com",   "/news/cities/bangalore/?service=rss","world","en","IN","Karnataka",NewsScope::State},
    // ── Maharashtra ────────────────────────────────────────────────
    {"Lokmat",              "https://www.lokmat.com",     "/rss",                        "world","mr","IN","Maharashtra",  NewsScope::State},
    {"Maharashtra Times",   "https://maharashtratimes.com","/rssfeedstopstories.cms",    "world","mr","IN","Maharashtra",  NewsScope::State},
    {"Hindustan Times Mumbai","https://www.hindustantimes.com","/rss/mumbai/rssfeed.xml","world","en","IN","Maharashtra",  NewsScope::State},
    {"Mid-Day Mumbai",      "https://www.mid-day.com",    "/rss/news.xml",               "world","en","IN","Maharashtra",  NewsScope::State},
    // ── Punjab / Haryana ───────────────────────────────────────────
    {"Punjab Kesari",       "https://www.punjabkesari.in","/feed",                       "world","hi","IN","Punjab",       NewsScope::State},
    {"Tribune Chandigarh",  "https://www.tribuneindia.com","/rssfeed/default.rss",       "world","en","IN","Punjab",       NewsScope::State},
    // ── Rajasthan ──────────────────────────────────────────────────
    {"Rajasthan Patrika",   "https://www.patrika.com",    "/rss/rajasthan-news",         "world","hi","IN","Rajasthan",    NewsScope::State},
    // ── Gujarat ────────────────────────────────────────────────────
    {"Gujarat Samachar",    "https://www.gujaratsamachar.com","/feeds.xml",              "world","gu","IN","Gujarat",      NewsScope::State},
    {"Sandesh",             "https://www.sandesh.com",    "/rss",                        "world","gu","IN","Gujarat",      NewsScope::State},
    // ── West Bengal ────────────────────────────────────────────────
    {"Anandabazar Patrika", "https://www.anandabazar.com","/rss/latest-news.xml",        "world","bn","IN","West Bengal",  NewsScope::State},
    {"The Telegraph Calcutta","https://www.telegraphindia.com","/feeds/feeds.xml",       "world","en","IN","West Bengal",  NewsScope::State},
    // ── Odisha ─────────────────────────────────────────────────────
    {"Sambad",              "https://sambadenglish.com",  "/feed",                       "world","or","IN","Odisha",       NewsScope::State},
    // ── Delhi / NCR ────────────────────────────────────────────────
    {"Delhi Times",         "https://timesofindia.indiatimes.com","/rssfeeds/-2128936835.cms","world","en","IN","Delhi",  NewsScope::State},
    {"Hindustan Times Delhi","https://www.hindustantimes.com","/rss/delhi/rssfeed.xml",  "world","en","IN","Delhi",       NewsScope::State},
    // ── Madhya Pradesh ─────────────────────────────────────────────
    {"Nai Dunia",           "https://www.naidunia.com",   "/rss",                        "world","hi","IN","Madhya Pradesh",NewsScope::State},
    // ── Assam / North East ─────────────────────────────────────────
    {"The Assam Tribune",   "https://www.assamtribune.com","/rss.xml",                   "world","en","IN","Assam",       NewsScope::State},
    {"North East Now",      "https://nenow.in",            "/feed",                      "world","en","IN","Assam",       NewsScope::State},
    // ── Jammu & Kashmir ────────────────────────────────────────────
    {"Kashmir Monitor",     "https://www.kashmirmonitor.in","/feed",                     "world","en","IN","Jammu & Kashmir",NewsScope::State},
    // ── Uttar Pradesh ──────────────────────────────────────────────
    {"Lucknow Times",       "https://timesofindia.indiatimes.com","/rssfeeds/3908999.cms","world","en","IN","Uttar Pradesh",NewsScope::State},
    // ── Bihar / Jharkhand ──────────────────────────────────────────
    {"Hindustan Hindi Bihar","https://www.livehindustan.com","/rss/statenews/bihar.xml", "world","hi","IN","Bihar",       NewsScope::State},
};

// =============================================================================
// RssBatchProvider — fetches a list of RSS feed definitions
// =============================================================================
class RssBatchProvider : public NewsProvider {
public:
    explicit RssBatchProvider(std::string  name,
                              NewsScope    scope,
                              std::vector<RssFeedDef> feeds)
        : name_(std::move(name)), scope_(scope), feeds_(std::move(feeds)) {}

    std::string name()  const override { return name_;  }
    NewsScope   scope() const override { return scope_; }

    void fetch(const std::string& category,
               const std::string& /*country*/,
               int /*page_size*/,
               ArticleCallback callback) override
    {
        // Filter by category (empty = all)
        std::vector<const RssFeedDef*> targets;
        for (const auto& f : feeds_) {
            if (category.empty() || f.category == category || category == "all")
                targets.push_back(&f);
        }
        if (targets.empty()) {
            for (const auto& f : feeds_) targets.push_back(&f); // fallback: all
        }

        auto results   = std::make_shared<std::vector<RawArticle>>();
        auto remaining = std::make_shared<std::atomic<int>>(
            static_cast<int>(targets.size()));
        auto mtx       = std::make_shared<std::mutex>();

        for (auto* fd : targets) {
            auto client = drogon::HttpClient::newHttpClient(fd->host);
            auto req    = drogon::HttpRequest::newHttpRequest();
            req->setMethod(drogon::Get);
            req->setPath(fd->path);
            req->addHeader("User-Agent", "AI-News-Platform/1.0");
            req->addHeader("Accept", "application/rss+xml, application/xml, text/xml");

            client->sendRequest(req,
                [results, remaining, mtx, fd, callback]
                (drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                    if (res == drogon::ReqResult::Ok && resp) {
                        auto parsed = rss_parser::parse(
                            resp->body(),
                            fd->name, fd->category, fd->language,
                            fd->country, fd->state, fd->scope);

                        std::lock_guard<std::mutex> lk(*mtx);
                        for (auto& a : parsed) results->push_back(std::move(a));
                    }
                    if (--(*remaining) == 0) {
                        callback(std::move(*results));
                    }
                });
        }
    }

private:
    std::string            name_;
    NewsScope              scope_;
    std::vector<RssFeedDef> feeds_;
};

// =============================================================================
// NewsAPI.org (key-based, 100 req/day free)
// Env var: NEWSAPI_KEY
// =============================================================================
class NewsApiProvider : public NewsProvider {
public:
    explicit NewsApiProvider(std::string key) : key_(std::move(key)) {}
    std::string name()  const override { return "NewsAPI.org"; }
    NewsScope   scope() const override { return NewsScope::International; }

    void fetch(const std::string& category,
               const std::string& country,
               int page_size,
               ArticleCallback callback) override
    {
        auto client = drogon::HttpClient::newHttpClient("https://newsapi.org");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath("/v2/top-headlines?country=" + (country.empty() ? "us" : country) +
                     "&category=" + (category.empty() ? "general" : category) +
                     "&pageSize=" + std::to_string(page_size) +
                     "&apiKey=" + key_);
        client->sendRequest(req,
            [callback](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res != drogon::ReqResult::Ok || !resp) { callback({}); return; }
                try {
                    auto j = nlohmann::json::parse(resp->body());
                    std::vector<RawArticle> out;
                    for (const auto& item : j["articles"]) {
                        if (item.value("title","") == "[Removed]") continue;
                        RawArticle a;
                        a.title        = item.value("title","");
                        a.excerpt      = item.value("description","");
                        a.content      = item.value("content","");
                        a.url          = item.value("url","");
                        a.image_url    = item.value("urlToImage","");
                        a.author       = item.value("author","");
                        a.published_at = item.value("publishedAt","");
                        a.language     = "en";
                        a.scope        = NewsScope::International;
                        if (item.contains("source"))
                            a.publisher = item["source"].value("name","");
                        a.external_id = "newsapi_" + a.url.substr(0,80);
                        if (!a.title.empty() && !a.url.empty()) out.push_back(std::move(a));
                    }
                    callback(std::move(out));
                } catch (...) { callback({}); }
            });
    }
private:
    std::string key_;
};

// =============================================================================
// NewsAPI.org — INDIA endpoint  (country=in)
// =============================================================================
class NewsApiIndiaProvider : public NewsProvider {
public:
    explicit NewsApiIndiaProvider(std::string key) : key_(std::move(key)) {}
    std::string name()  const override { return "NewsAPI India"; }
    NewsScope   scope() const override { return NewsScope::National; }

    void fetch(const std::string& category,
               const std::string& /*country*/,
               int page_size,
               ArticleCallback callback) override
    {
        auto client = drogon::HttpClient::newHttpClient("https://newsapi.org");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath("/v2/top-headlines?country=in&pageSize=" +
                     std::to_string(page_size) +
                     (category.empty() ? "" : "&category=" + category) +
                     "&apiKey=" + key_);
        client->sendRequest(req,
            [callback](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res != drogon::ReqResult::Ok || !resp) { callback({}); return; }
                try {
                    auto j = nlohmann::json::parse(resp->body());
                    std::vector<RawArticle> out;
                    for (const auto& item : j["articles"]) {
                        if (item.value("title","") == "[Removed]") continue;
                        RawArticle a;
                        a.title        = item.value("title","");
                        a.excerpt      = item.value("description","");
                        a.url          = item.value("url","");
                        a.image_url    = item.value("urlToImage","");
                        a.author       = item.value("author","");
                        a.published_at = item.value("publishedAt","");
                        a.country      = "IN";
                        a.language     = "en";
                        a.scope        = NewsScope::National;
                        if (item.contains("source"))
                            a.publisher = item["source"].value("name","");
                        a.external_id = "newsapi_in_" + a.url.substr(0,80);
                        if (!a.title.empty() && !a.url.empty()) out.push_back(std::move(a));
                    }
                    callback(std::move(out));
                } catch (...) { callback({}); }
            });
    }
private:
    std::string key_;
};

// =============================================================================
// The Guardian (free API key)
// Env var: GUARDIAN_API_KEY
// =============================================================================
class GuardianProvider : public NewsProvider {
public:
    explicit GuardianProvider(std::string key) : key_(std::move(key)) {}
    std::string name()  const override { return "The Guardian"; }
    NewsScope   scope() const override { return NewsScope::International; }

    void fetch(const std::string& category,
               const std::string& /*country*/,
               int page_size,
               ArticleCallback callback) override
    {
        static const std::unordered_map<std::string,std::string> sec = {
            {"technology","technology"},{"politics","politics"},
            {"business","business"},   {"sports","sport"},
            {"science","science"},     {"health","society"},
            {"world","world"},         {"ai","technology"},
        };
        std::string section = sec.count(category) ? sec.at(category) : "news";

        auto client = drogon::HttpClient::newHttpClient("https://content.guardianapis.com");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath("/search?section=" + section +
                     "&page-size=" + std::to_string(page_size) +
                     "&show-fields=trailText,thumbnail,bodyText,byline" +
                     "&order-by=newest&api-key=" + key_);
        client->sendRequest(req,
            [callback](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res != drogon::ReqResult::Ok || !resp) { callback({}); return; }
                try {
                    auto j = nlohmann::json::parse(resp->body());
                    std::vector<RawArticle> out;
                    for (const auto& item : j["response"]["results"]) {
                        RawArticle a;
                        a.external_id  = "guardian_" + item.value("id","");
                        a.title        = item.value("webTitle","");
                        a.url          = item.value("webUrl","");
                        a.published_at = item.value("webPublicationDate","");
                        a.publisher    = "The Guardian";
                        a.language     = "en"; a.country = "GB";
                        a.scope        = NewsScope::International;
                        if (item.contains("fields")) {
                            auto& f = item["fields"];
                            a.excerpt   = f.value("trailText","");
                            a.content   = f.value("bodyText","");
                            a.image_url = f.value("thumbnail","");
                            a.author    = f.value("byline","");
                        }
                        if (!a.title.empty()) out.push_back(std::move(a));
                    }
                    callback(std::move(out));
                } catch (...) { callback({}); }
            });
    }
private:
    std::string key_;
};

// =============================================================================
// GNews API (100 req/day free)
// Env var: GNEWS_API_KEY
// =============================================================================
class GNewsProvider : public NewsProvider {
public:
    explicit GNewsProvider(std::string key) : key_(std::move(key)) {}
    std::string name()  const override { return "GNews"; }
    NewsScope   scope() const override { return NewsScope::International; }

    void fetch(const std::string& category,
               const std::string& country,
               int page_size,
               ArticleCallback callback) override
    {
        std::string cc = country.empty() ? "in" : country; // default India
        auto client = drogon::HttpClient::newHttpClient("https://gnews.io");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath("/api/v4/top-headlines?topic=" +
                     (category.empty() ? "general" : category) +
                     "&country=" + cc + "&max=" + std::to_string(page_size) +
                     "&lang=en&apikey=" + key_);
        client->sendRequest(req,
            [callback, cc](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res != drogon::ReqResult::Ok || !resp) { callback({}); return; }
                try {
                    auto j = nlohmann::json::parse(resp->body());
                    std::vector<RawArticle> out;
                    for (const auto& item : j["articles"]) {
                        RawArticle a;
                        a.title        = item.value("title","");
                        a.excerpt      = item.value("description","");
                        a.content      = item.value("content","");
                        a.url          = item.value("url","");
                        a.image_url    = item.value("image","");
                        a.published_at = item.value("publishedAt","");
                        a.language     = "en";
                        a.country      = cc == "in" ? "IN" : cc;
                        a.scope        = (cc == "in") ? NewsScope::National : NewsScope::International;
                        if (item.contains("source"))
                            a.publisher = item["source"].value("name","");
                        a.external_id = "gnews_" + a.url.substr(0,80);
                        if (!a.title.empty()) out.push_back(std::move(a));
                    }
                    callback(std::move(out));
                } catch (...) { callback({}); }
            });
    }
private:
    std::string key_;
};

// =============================================================================
// NYT API (free)
// Env var: NYT_API_KEY
// =============================================================================
class NYTimesProvider : public NewsProvider {
public:
    explicit NYTimesProvider(std::string key) : key_(std::move(key)) {}
    std::string name()  const override { return "New York Times"; }
    NewsScope   scope() const override { return NewsScope::International; }

    void fetch(const std::string& category,
               const std::string& /*country*/,
               int page_size,
               ArticleCallback callback) override
    {
        static const std::unordered_map<std::string,std::string> sec = {
            {"world","world"},{"technology","technology"},{"science","science"},
            {"health","health"},{"business","business"},{"sports","sports"},
            {"politics","politics"},{"ai","technology"},
        };
        std::string section = sec.count(category) ? sec.at(category) : "home";
        auto client = drogon::HttpClient::newHttpClient("https://api.nytimes.com");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath("/svc/topstories/v2/" + section + ".json?api-key=" + key_);
        client->sendRequest(req,
            [callback, page_size](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res != drogon::ReqResult::Ok || !resp) { callback({}); return; }
                try {
                    auto j = nlohmann::json::parse(resp->body());
                    std::vector<RawArticle> out;
                    int cnt = 0;
                    for (const auto& item : j["results"]) {
                        if (cnt++ >= page_size) break;
                        RawArticle a;
                        a.external_id  = "nyt_" + item.value("uri","");
                        a.title        = item.value("title","");
                        a.excerpt      = item.value("abstract","");
                        a.url          = item.value("url","");
                        a.author       = item.value("byline","");
                        a.published_at = item.value("published_date","");
                        a.publisher    = "New York Times";
                        a.language     = "en"; a.country = "US";
                        a.scope        = NewsScope::International;
                        a.category     = item.value("section","");
                        if (item.contains("multimedia") && item["multimedia"].is_array()) {
                            for (const auto& m : item["multimedia"]) {
                                if (m.value("format","") == "Large Thumbnail") {
                                    a.image_url = m.value("url",""); break;
                                }
                            }
                        }
                        if (!a.title.empty()) out.push_back(std::move(a));
                    }
                    callback(std::move(out));
                } catch (...) { callback({}); }
            });
    }
private:
    std::string key_;
};

// =============================================================================
// TIER 4: SPORTS — Global + India  (Cricket, Football, F1, Tennis, Basketball)
// =============================================================================
static const std::vector<RssFeedDef> SPORTS_FEEDS = {
    // ── Cricket (India's #1 sport) ────────────────────────────────────
    {"Cricbuzz",           "https://www.cricbuzz.com",          "/rss/cricket-news",                   "sports","en","IN","",NewsScope::International},
    {"ESPNcricinfo",       "https://www.espncricinfo.com",       "/rss/content/story/feeds/0.xml",       "sports","en","GB","",NewsScope::International},
    {"NDTV Sports Cricket","https://sports.ndtv.com",            "/rss/feeds/news/cricket.xml",          "sports","en","IN","",NewsScope::National},
    {"Sportskeeda Cricket","https://www.sportskeeda.com",        "/feed/cricket",                       "sports","en","IN","",NewsScope::International},
    {"Times of India Cricket","https://timesofindia.indiatimes.com","/rssfeeds/4719161.cms",           "sports","en","IN","",NewsScope::National},
    // ── Football / Soccer ─────────────────────────────────────────────
    {"Goal.com",           "https://www.goal.com",               "/en-in/feeds/news?fmt=rss",           "sports","en","IN","",NewsScope::International},
    {"Sky Sports Football","https://www.skysports.com",          "/rss/12040",                          "sports","en","GB","",NewsScope::International},
    {"ESPN FC",            "https://www.espnfc.com",             "/rss/feeds/0.xml",                    "sports","en","US","",NewsScope::International},
    {"BBC Sport Football", "https://feeds.bbci.co.uk",           "/sport/football/rss.xml",             "sports","en","GB","",NewsScope::International},
    {"Guardian Football",  "https://www.theguardian.com",        "/football/rss",                       "sports","en","GB","",NewsScope::International},
    // ── Formula 1 ─────────────────────────────────────────────────────
    {"Formula 1",          "https://www.formula1.com",           "/en/latest/all.xml",                  "sports","en","GB","",NewsScope::International},
    {"Autosport F1",       "https://www.autosport.com",          "/rss/f1/news",                        "sports","en","GB","",NewsScope::International},
    // ── Tennis ─────────────────────────────────────────────────────────
    {"ATP Tour",           "https://www.atptour.com",            "/en/media/rss-feed/xml-feed.xml",     "sports","en","US","",NewsScope::International},
    {"BBC Sport Tennis",   "https://feeds.bbci.co.uk",           "/sport/tennis/rss.xml",               "sports","en","GB","",NewsScope::International},
    // ── Basketball / NBA ──────────────────────────────────────────────
    {"NBA News",           "https://www.nba.com",                "/feeds/home.rss",                     "sports","en","US","",NewsScope::International},
    {"ESPN NBA",           "https://www.espn.com",               "/espn/rss/nba/news",                  "sports","en","US","",NewsScope::International},
    // ── General Sports ─────────────────────────────────────────────────
    {"ESPN Top Sports",    "https://www.espn.com",               "/espn/rss/news",                      "sports","en","US","",NewsScope::International},
    {"Sky Sports All",     "https://www.skysports.com",          "/rss/12040",                          "sports","en","GB","",NewsScope::International},
    {"BBC Sport All",      "https://feeds.bbci.co.uk",           "/sport/rss.xml",                      "sports","en","GB","",NewsScope::International},
    {"Reuters Sports",     "https://feeds.reuters.com",          "/reuters/sportsNews",                 "sports","en","US","",NewsScope::International},
    {"AP Sports",          "https://feeds.apnews.com",           "/apf-sports",                         "sports","en","US","",NewsScope::International},
    {"Sportskeeda All",    "https://www.sportskeeda.com",        "/feed",                               "sports","en","IN","",NewsScope::International},
    {"NDTV Sports",        "https://sports.ndtv.com",            "/rss/feeds/news/allsports.xml",       "sports","en","IN","",NewsScope::National},
    {"India Today Sports", "https://www.indiatoday.in",          "/rss/sports",                         "sports","en","IN","",NewsScope::National},
    {"Hindustan Times Sports","https://www.hindustantimes.com",  "/rss/sports/rssfeed.xml",             "sports","en","IN","",NewsScope::National},
    // ── Olympics ───────────────────────────────────────────────────────
    {"Olympics",           "https://olympics.com",               "/en/feed/",                           "sports","en","CH","",NewsScope::International},
    // ── Badminton / Kabaddi (India specific) ──────────────────────────
    {"Badminton World",    "https://bwfbadminton.com",           "/en/latest/news",                     "sports","en","MY","",NewsScope::International},
};

// =============================================================================
// NewsFetcherService — orchestrates ALL providers across all 3 tiers
// =============================================================================
class NewsFetcherService {
public:
    NewsFetcherService() {
        // ── Tier 1: International (RSS — always active, no key needed) ────────
        providers_.push_back(std::make_unique<RssBatchProvider>(
            "International RSS", NewsScope::International, INTERNATIONAL_FEEDS));

        // ── Tier 2: National India (RSS — always active) ──────────────────────
        providers_.push_back(std::make_unique<RssBatchProvider>(
            "National India RSS", NewsScope::National, NATIONAL_INDIA_FEEDS));

        // ── Tier 3: State India (RSS — always active) ────────────────────────
        providers_.push_back(std::make_unique<RssBatchProvider>(
            "State India RSS", NewsScope::State, STATE_INDIA_FEEDS));

        // ── Tier 4: Sports (RSS — always active) ─────────────────────────────
        providers_.push_back(std::make_unique<RssBatchProvider>(
            "Sports RSS", NewsScope::International, SPORTS_FEEDS));

        // ── Key-based providers (optional, loaded from env) ───────────────────
        const auto& cfg = get_config();

        if (!cfg.ai.newsapi_key.empty()) {
            // Global headlines
            providers_.push_back(std::make_unique<NewsApiProvider>(cfg.ai.newsapi_key));
            // India-specific headlines
            providers_.push_back(std::make_unique<NewsApiIndiaProvider>(cfg.ai.newsapi_key));
        }
        if (auto* k = std::getenv("GUARDIAN_API_KEY"); k && *k)
            providers_.push_back(std::make_unique<GuardianProvider>(k));
        if (auto* k = std::getenv("GNEWS_API_KEY"); k && *k)
            providers_.push_back(std::make_unique<GNewsProvider>(k));
        if (auto* k = std::getenv("NYT_API_KEY"); k && *k)
            providers_.push_back(std::make_unique<NYTimesProvider>(k));

        spdlog::info("[NewsFetcher] {} providers loaded (Int+Nat+State)", providers_.size());
    }

    // Fetch from ALL providers, merge & deduplicate
    void fetch_all(const std::string& category,
                   const std::string& country,
                   int page_size,
                   ArticleCallback callback)
    {
        fetch_by_scope({NewsScope::International,
                        NewsScope::National,
                        NewsScope::State},
                       category, country, page_size, callback);
    }

    // Fetch only International news
    void fetch_international(const std::string& category, int page_size, ArticleCallback cb) {
        fetch_by_scope({NewsScope::International}, category, "us", page_size, cb);
    }

    // Fetch only National (India) news
    void fetch_national(const std::string& category, int page_size, ArticleCallback cb) {
        fetch_by_scope({NewsScope::National}, category, "in", page_size, cb);
    }

    // Fetch only Sports news (optionally filtered by sport keyword)
    void fetch_sports(const std::string& sport_keyword, int page_size, ArticleCallback cb) {
        // providers_[3] = Sports RSS batch provider
        providers_[3]->fetch("sports", "", page_size,
            [sport_keyword, cb](std::vector<RawArticle> batch) {
                if (!sport_keyword.empty()) {
                    std::string kw_lower = sport_keyword;
                    std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                    batch.erase(
                        std::remove_if(batch.begin(), batch.end(),
                            [&kw_lower](const RawArticle& a) {
                                std::string t_lower = a.title;
                                std::transform(t_lower.begin(), t_lower.end(), t_lower.begin(), ::tolower);
                                return t_lower.find(kw_lower) == std::string::npos;
                            }),
                        batch.end());
                }
                cb(std::move(batch));
            });
    }

    // Fetch only State news (optionally filtered by state name)
    void fetch_state(const std::string& state_name, int page_size, ArticleCallback cb) {
        auto results   = std::make_shared<std::vector<RawArticle>>();
        auto remaining = std::make_shared<std::atomic<int>>(1);
        auto mtx       = std::make_shared<std::mutex>();

        // Use RSS state provider directly
        providers_[2]->fetch("", "", page_size,
            [results, remaining, mtx, state_name, cb]
            (std::vector<RawArticle> batch) {
                {
                    std::lock_guard<std::mutex> lk(*mtx);
                    for (auto& a : batch) {
                        if (state_name.empty() || a.state == state_name)
                            results->push_back(std::move(a));
                    }
                }
                if (--(*remaining) == 0) {
                    deduplicate(*results);
                    cb(std::move(*results));
                }
            });
    }

    size_t provider_count() const { return providers_.size(); }

    // List all available states (for the UI state picker)
    static std::vector<std::string> available_states() {
        std::unordered_set<std::string> seen;
        std::vector<std::string> states;
        for (const auto& f : STATE_INDIA_FEEDS) {
            if (!f.state.empty() && seen.insert(f.state).second)
                states.push_back(f.state);
        }
        std::sort(states.begin(), states.end());
        return states;
    }

private:
    void fetch_by_scope(const std::vector<NewsScope>& scopes,
                        const std::string& category,
                        const std::string& country,
                        int page_size,
                        ArticleCallback callback)
    {
        std::vector<NewsProvider*> targets;
        for (auto& p : providers_) {
            for (auto s : scopes) {
                if (p->scope() == s) { targets.push_back(p.get()); break; }
            }
        }
        if (targets.empty()) { callback({}); return; }

        auto results   = std::make_shared<std::vector<RawArticle>>();
        auto remaining = std::make_shared<std::atomic<int>>(static_cast<int>(targets.size()));
        auto mtx       = std::make_shared<std::mutex>();

        for (auto* p : targets) {
            p->fetch(category, country, page_size,
                [results, remaining, mtx, callback]
                (std::vector<RawArticle> batch) {
                    {
                        std::lock_guard<std::mutex> lk(*mtx);
                        for (auto& a : batch) results->push_back(std::move(a));
                    }
                    if (--(*remaining) == 0) {
                        deduplicate(*results);
                        callback(std::move(*results));
                    }
                });
        }
    }

    static void deduplicate(std::vector<RawArticle>& articles) {
        std::unordered_set<std::string> seen;
        articles.erase(
            std::remove_if(articles.begin(), articles.end(),
                [&seen](const RawArticle& a) {
                    return !seen.insert(a.url).second;
                }),
            articles.end());
    }

    std::vector<std::unique_ptr<NewsProvider>> providers_;
};

} // namespace news
