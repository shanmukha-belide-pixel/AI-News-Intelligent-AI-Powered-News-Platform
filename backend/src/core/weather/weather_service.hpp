#pragma once
// =============================================================================
// weather_service.hpp  —  Live weather via OpenWeatherMap API
// =============================================================================
// Free tier: 60 calls/min, current weather + 5-day forecast
// Env var: OPENWEATHER_API_KEY
// Docs: https://openweathermap.org/api
// =============================================================================
#include <string>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <drogon/HttpClient.h>
#include <spdlog/spdlog.h>

namespace news {

// ── Data models ──────────────────────────────────────────────────────────────
struct WeatherCurrent {
    std::string city;
    std::string country;
    double      temp_c;
    double      feels_like_c;
    double      temp_min_c;
    double      temp_max_c;
    int         humidity_pct;
    int         pressure_hpa;
    double      wind_speed_ms;
    int         wind_deg;
    int         visibility_m;
    int         cloud_pct;
    std::string condition;          // "Clear", "Rain", "Thunderstorm" …
    std::string description;        // "light rain", "scattered clouds" …
    std::string icon_code;          // "01d", "10n" …
    std::string icon_url;           // full URL to icon PNG
    long long   sunrise_unix;
    long long   sunset_unix;
    long long   updated_unix;
    std::string timezone;
};

struct WeatherForecastSlot {
    std::string datetime;           // ISO-8601
    double      temp_c;
    double      feels_like_c;
    double      temp_min_c;
    double      temp_max_c;
    int         humidity_pct;
    double      wind_speed_ms;
    double      rain_mm;            // rain in last 3 hours (may be 0)
    std::string condition;
    std::string description;
    std::string icon_code;
    std::string icon_url;
};

struct WeatherForecast {
    std::string              city;
    std::string              country;
    std::vector<WeatherForecastSlot> slots; // up to 40 × 3-hour slots = 5 days
};

struct AirQuality {
    std::string city;
    int         aqi;            // 1=Good, 2=Fair, 3=Moderate, 4=Poor, 5=Very Poor
    std::string aqi_label;
    double      pm2_5;
    double      pm10;
    double      co;
    double      no2;
    double      o3;
};

using WeatherCallback   = std::function<void(std::optional<WeatherCurrent>)>;
using ForecastCallback  = std::function<void(std::optional<WeatherForecast>)>;
using AirQualCallback   = std::function<void(std::optional<AirQuality>)>;

// =============================================================================
// WeatherService
// =============================================================================
class WeatherService {
public:
    explicit WeatherService() {
        api_key_ = []() -> std::string {
            if (auto* k = std::getenv("OPENWEATHER_API_KEY"); k && *k) return k;
            return "";
        }();
        if (api_key_.empty())
            spdlog::warn("[Weather] OPENWEATHER_API_KEY not set — weather disabled");
    }

    bool is_enabled() const { return !api_key_.empty(); }

    // ── Current weather by city name ─────────────────────────────────────────
    void current_by_city(const std::string& city,
                          const std::string& country_code,  // "IN", "US", ""
                          WeatherCallback callback)
    {
        if (!is_enabled()) { callback(std::nullopt); return; }

        std::string q = city + (country_code.empty() ? "" : "," + country_code);
        std::string path = "/data/2.5/weather?q=" + url_encode(q) +
                           "&units=metric&lang=en&appid=" + api_key_;
        get(path, [callback, city](const std::string& body) {
            callback(parse_current(body, city));
        });
    }

    // ── Current weather by lat/lon (more accurate for mobile) ────────────────
    void current_by_coords(double lat, double lon, WeatherCallback callback) {
        if (!is_enabled()) { callback(std::nullopt); return; }

        std::string path = "/data/2.5/weather?lat="  + std::to_string(lat) +
                           "&lon=" + std::to_string(lon) +
                           "&units=metric&lang=en&appid=" + api_key_;
        get(path, [callback](const std::string& body) {
            callback(parse_current(body, ""));
        });
    }

    // ── 5-day / 3-hour forecast by city ──────────────────────────────────────
    void forecast_by_city(const std::string& city,
                           const std::string& country_code,
                           ForecastCallback callback)
    {
        if (!is_enabled()) { callback(std::nullopt); return; }

        std::string q    = city + (country_code.empty() ? "" : "," + country_code);
        std::string path = "/data/2.5/forecast?q=" + url_encode(q) +
                           "&units=metric&lang=en&cnt=40&appid=" + api_key_;
        get(path, [callback, city](const std::string& body) {
            callback(parse_forecast(body, city));
        });
    }

    // ── 5-day forecast by coords ─────────────────────────────────────────────
    void forecast_by_coords(double lat, double lon, ForecastCallback callback) {
        if (!is_enabled()) { callback(std::nullopt); return; }

        std::string path = "/data/2.5/forecast?lat=" + std::to_string(lat) +
                           "&lon=" + std::to_string(lon) +
                           "&units=metric&lang=en&cnt=40&appid=" + api_key_;
        get(path, [callback](const std::string& body) {
            callback(parse_forecast(body, ""));
        });
    }

    // ── Air Quality Index by coords ──────────────────────────────────────────
    void air_quality(double lat, double lon, AirQualCallback callback) {
        if (!is_enabled()) { callback(std::nullopt); return; }

        std::string path = "/data/2.5/air_pollution?lat=" + std::to_string(lat) +
                           "&lon=" + std::to_string(lon) +
                           "&appid=" + api_key_;
        get(path, [callback](const std::string& body) {
            callback(parse_air_quality(body));
        });
    }

    // ── Serialise current weather to JSON (for REST response) ────────────────
    static nlohmann::json to_json(const WeatherCurrent& w) {
        return {
            {"city",           w.city},
            {"country",        w.country},
            {"temp_c",         w.temp_c},
            {"feels_like_c",   w.feels_like_c},
            {"temp_min_c",     w.temp_min_c},
            {"temp_max_c",     w.temp_max_c},
            {"humidity_pct",   w.humidity_pct},
            {"pressure_hpa",   w.pressure_hpa},
            {"wind_speed_ms",  w.wind_speed_ms},
            {"wind_deg",       w.wind_deg},
            {"visibility_m",   w.visibility_m},
            {"cloud_pct",      w.cloud_pct},
            {"condition",      w.condition},
            {"description",    w.description},
            {"icon_code",      w.icon_code},
            {"icon_url",       w.icon_url},
            {"sunrise_unix",   w.sunrise_unix},
            {"sunset_unix",    w.sunset_unix},
            {"updated_unix",   w.updated_unix},
            {"timezone",       w.timezone}
        };
    }

    static nlohmann::json to_json(const WeatherForecast& f) {
        nlohmann::json slots = nlohmann::json::array();
        for (const auto& s : f.slots) {
            slots.push_back({
                {"datetime",       s.datetime},
                {"temp_c",         s.temp_c},
                {"feels_like_c",   s.feels_like_c},
                {"temp_min_c",     s.temp_min_c},
                {"temp_max_c",     s.temp_max_c},
                {"humidity_pct",   s.humidity_pct},
                {"wind_speed_ms",  s.wind_speed_ms},
                {"rain_mm",        s.rain_mm},
                {"condition",      s.condition},
                {"description",    s.description},
                {"icon_code",      s.icon_code},
                {"icon_url",       s.icon_url}
            });
        }
        return {{"city", f.city}, {"country", f.country}, {"forecast", slots}};
    }

private:
    // ── HTTP GET helper ──────────────────────────────────────────────────────
    void get(const std::string& path,
             std::function<void(const std::string&)> on_body)
    {
        auto client = drogon::HttpClient::newHttpClient("https://api.openweathermap.org");
        auto req    = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath(path);
        client->sendRequest(req,
            [on_body](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
                if (res == drogon::ReqResult::Ok && resp)
                    on_body(resp->body());
                else
                    on_body("");
            });
    }

    // ── Parsers ──────────────────────────────────────────────────────────────
    static std::optional<WeatherCurrent> parse_current(const std::string& body,
                                                       const std::string& city_hint)
    {
        if (body.empty()) return std::nullopt;
        try {
            auto j = nlohmann::json::parse(body);
            if (j.value("cod", 200) != 200) return std::nullopt;

            WeatherCurrent w;
            w.city        = j.value("name", city_hint);
            w.country     = j.contains("sys") ? j["sys"].value("country","") : "";
            w.temp_c      = j["main"].value("temp",  0.0);
            w.feels_like_c= j["main"].value("feels_like", 0.0);
            w.temp_min_c  = j["main"].value("temp_min", 0.0);
            w.temp_max_c  = j["main"].value("temp_max", 0.0);
            w.humidity_pct= j["main"].value("humidity",  0);
            w.pressure_hpa= j["main"].value("pressure",  0);
            w.wind_speed_ms= j.contains("wind") ? j["wind"].value("speed", 0.0) : 0.0;
            w.wind_deg    = j.contains("wind") ? j["wind"].value("deg",   0)   : 0;
            w.visibility_m= j.value("visibility", 0);
            w.cloud_pct   = j.contains("clouds") ? j["clouds"].value("all", 0) : 0;
            w.updated_unix= j.value("dt", 0LL);
            w.timezone    = std::to_string(j.value("timezone", 0));

            if (j.contains("weather") && !j["weather"].empty()) {
                auto& wt = j["weather"][0];
                w.condition   = wt.value("main","");
                w.description = wt.value("description","");
                w.icon_code   = wt.value("icon","");
                w.icon_url    = "https://openweathermap.org/img/wn/" + w.icon_code + "@2x.png";
            }
            if (j.contains("sys")) {
                w.sunrise_unix = j["sys"].value("sunrise", 0LL);
                w.sunset_unix  = j["sys"].value("sunset",  0LL);
            }
            return w;
        } catch (const std::exception& e) {
            spdlog::error("[Weather] parse_current error: {}", e.what());
            return std::nullopt;
        }
    }

    static std::optional<WeatherForecast> parse_forecast(const std::string& body,
                                                         const std::string& city_hint)
    {
        if (body.empty()) return std::nullopt;
        try {
            auto j = nlohmann::json::parse(body);
            WeatherForecast f;
            f.city    = j.contains("city") ? j["city"].value("name", city_hint) : city_hint;
            f.country = j.contains("city") ? j["city"]["coord"].value("country","") : "";

            for (const auto& slot : j["list"]) {
                WeatherForecastSlot s;
                s.datetime    = slot.value("dt_txt","");
                s.temp_c      = slot["main"].value("temp",  0.0);
                s.feels_like_c= slot["main"].value("feels_like", 0.0);
                s.temp_min_c  = slot["main"].value("temp_min", 0.0);
                s.temp_max_c  = slot["main"].value("temp_max", 0.0);
                s.humidity_pct= slot["main"].value("humidity",  0);
                s.wind_speed_ms= slot.contains("wind") ? slot["wind"].value("speed",0.0) : 0.0;
                s.rain_mm     = slot.contains("rain")  ? slot["rain"].value("3h", 0.0)   : 0.0;

                if (slot.contains("weather") && !slot["weather"].empty()) {
                    s.condition   = slot["weather"][0].value("main","");
                    s.description = slot["weather"][0].value("description","");
                    s.icon_code   = slot["weather"][0].value("icon","");
                    s.icon_url    = "https://openweathermap.org/img/wn/" + s.icon_code + "@2x.png";
                }
                f.slots.push_back(std::move(s));
            }
            return f;
        } catch (...) { return std::nullopt; }
    }

    static std::optional<AirQuality> parse_air_quality(const std::string& body) {
        if (body.empty()) return std::nullopt;
        try {
            auto j = nlohmann::json::parse(body);
            auto& item = j["list"][0];
            AirQuality aq;
            aq.aqi    = item["main"].value("aqi", 0);
            static const char* labels[] = {"","Good","Fair","Moderate","Poor","Very Poor"};
            aq.aqi_label = (aq.aqi >= 1 && aq.aqi <= 5) ? labels[aq.aqi] : "Unknown";
            auto& comp   = item["components"];
            aq.pm2_5 = comp.value("pm2_5", 0.0);
            aq.pm10  = comp.value("pm10",  0.0);
            aq.co    = comp.value("co",    0.0);
            aq.no2   = comp.value("no2",   0.0);
            aq.o3    = comp.value("o3",    0.0);
            return aq;
        } catch (...) { return std::nullopt; }
    }

    static std::string url_encode(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += c;
            else if (c == ' ') out += '+';
            else { char buf[4]; snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c); out += buf; }
        }
        return out;
    }

    std::string api_key_;
};

} // namespace news
