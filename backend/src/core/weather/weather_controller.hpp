#pragma once
// =============================================================================
// weather_controller.hpp  —  REST endpoints for weather + air quality
// =============================================================================
// GET /api/v1/weather/current?city=Mumbai&country=IN
// GET /api/v1/weather/current?lat=19.07&lon=72.87
// GET /api/v1/weather/forecast?city=Delhi&country=IN
// GET /api/v1/weather/forecast?lat=28.61&lon=77.20
// GET /api/v1/weather/air-quality?lat=28.61&lon=77.20
// GET /api/v1/weather/cities  (preset Indian cities quick-list)
// =============================================================================
#include <drogon/HttpController.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "weather_service.hpp"
#include "../../infrastructure/redis_client.hpp"

namespace news {

class WeatherController
    : public drogon::HttpController<WeatherController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(WeatherController::get_current,   "/api/v1/weather/current",    drogon::Get);
        ADD_METHOD_TO(WeatherController::get_forecast,  "/api/v1/weather/forecast",   drogon::Get);
        ADD_METHOD_TO(WeatherController::get_air,       "/api/v1/weather/air-quality",drogon::Get);
        ADD_METHOD_TO(WeatherController::get_cities,    "/api/v1/weather/cities",     drogon::Get);
    METHOD_LIST_END

    // ── GET /api/v1/weather/current ──────────────────────────────────────────
    void get_current(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params  = req->getParameters();
        std::string city    = params.count("city")    ? params.at("city")    : "";
        std::string country = params.count("country") ? params.at("country") : "IN";
        std::string lat_s   = params.count("lat")     ? params.at("lat")     : "";
        std::string lon_s   = params.count("lon")     ? params.at("lon")     : "";

        // Cache key
        std::string cache_key = "weather:current:" + (lat_s.empty() ? city + ":" + country : lat_s + ":" + lon_s);

        // Try Redis cache first (TTL 10 min)
        auto redis = RedisClient::instance();
        redis->get(cache_key, [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
            if (cached) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(*cached);
                cb(resp);
                return;
            }

            auto on_result = [=, cb=std::move(cb)](std::optional<WeatherCurrent> w) mutable {
                if (!w) { cb(error_response("Weather data unavailable")); return; }
                nlohmann::json j = {
                    {"success", true},
                    {"data",    WeatherService::to_json(*w)}
                };
                std::string body = j.dump();
                // Cache for 10 minutes
                RedisClient::instance()->set_ex(cache_key, body, 600);
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(body);
                cb(resp);
            };

            if (!lat_s.empty() && !lon_s.empty()) {
                try {
                    double lat = std::stod(lat_s), lon = std::stod(lon_s);
                    weather_.current_by_coords(lat, lon, on_result);
                } catch (...) { cb(error_response("Invalid lat/lon")); }
            } else if (!city.empty()) {
                weather_.current_by_city(city, country, on_result);
            } else {
                cb(error_response("Provide city or lat/lon parameters"));
            }
        });
    }

    // ── GET /api/v1/weather/forecast ─────────────────────────────────────────
    void get_forecast(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params  = req->getParameters();
        std::string city    = params.count("city")    ? params.at("city")    : "";
        std::string country = params.count("country") ? params.at("country") : "IN";
        std::string lat_s   = params.count("lat")     ? params.at("lat")     : "";
        std::string lon_s   = params.count("lon")     ? params.at("lon")     : "";

        std::string cache_key = "weather:forecast:" + (lat_s.empty() ? city + ":" + country : lat_s + ":" + lon_s);

        RedisClient::instance()->get(cache_key,
            [=, cb=std::move(cb)](std::optional<std::string> cached) mutable {
                if (cached) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k200OK);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setBody(*cached);
                    cb(resp);
                    return;
                }

                auto on_result = [=, cb=std::move(cb)](std::optional<WeatherForecast> f) mutable {
                    if (!f) { cb(error_response("Forecast unavailable")); return; }
                    nlohmann::json j = {{"success", true}, {"data", WeatherService::to_json(*f)}};
                    std::string body = j.dump();
                    RedisClient::instance()->set_ex(cache_key, body, 1800); // 30 min
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k200OK);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setBody(body);
                    cb(resp);
                };

                if (!lat_s.empty() && !lon_s.empty()) {
                    try { weather_.forecast_by_coords(std::stod(lat_s), std::stod(lon_s), on_result); }
                    catch (...) { cb(error_response("Invalid lat/lon")); }
                } else if (!city.empty()) {
                    weather_.forecast_by_city(city, country, on_result);
                } else {
                    cb(error_response("Provide city or lat/lon"));
                }
            });
    }

    // ── GET /api/v1/weather/air-quality ──────────────────────────────────────
    void get_air(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto params = req->getParameters();
        if (!params.count("lat") || !params.count("lon")) {
            cb(error_response("lat and lon are required")); return;
        }
        try {
            double lat = std::stod(params.at("lat")), lon = std::stod(params.at("lon"));
            weather_.air_quality(lat, lon,
                [cb=std::move(cb)](std::optional<AirQuality> aq) mutable {
                    if (!aq) { cb(error_response("AQI unavailable")); return; }
                    nlohmann::json j = {"success", true, "data", {
                        {"aqi",       aq->aqi},
                        {"aqi_label", aq->aqi_label},
                        {"pm2_5",     aq->pm2_5},
                        {"pm10",      aq->pm10},
                        {"co",        aq->co},
                        {"no2",       aq->no2},
                        {"o3",        aq->o3}
                    }};
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k200OK);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setBody(j.dump());
                    cb(resp);
                });
        } catch (...) { cb(error_response("Invalid lat/lon")); }
    }

    // ── GET /api/v1/weather/cities ───────────────────────────────────────────
    // Returns preset cities list for the home screen quick-select
    void get_cities(const drogon::HttpRequestPtr&,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        static const nlohmann::json cities = {
            {"international", {
                {"name","New York"},    {"lat",40.71}, {"lon",-74.01},
                {"name","London"},      {"lat",51.51}, {"lon",-0.13},
                {"name","Tokyo"},       {"lat",35.69}, {"lon",139.69},
                {"name","Dubai"},       {"lat",25.20}, {"lon",55.27},
                {"name","Singapore"},   {"lat",1.35},  {"lon",103.82}
            }},
            {"india", {
                {"name","Mumbai"},      {"lat",19.07},  {"lon",72.88},
                {"name","Delhi"},       {"lat",28.61},  {"lon",77.20},
                {"name","Bengaluru"},   {"lat",12.97},  {"lon",77.59},
                {"name","Chennai"},     {"lat",13.08},  {"lon",80.27},
                {"name","Kolkata"},     {"lat",22.57},  {"lon",88.36},
                {"name","Hyderabad"},   {"lat",17.38},  {"lon",78.47},
                {"name","Ahmedabad"},   {"lat",23.02},  {"lon",72.57},
                {"name","Pune"},        {"lat",18.52},  {"lon",73.86},
                {"name","Jaipur"},      {"lat",26.91},  {"lon",75.79},
                {"name","Lucknow"},     {"lat",26.85},  {"lon",80.95},
                {"name","Chandigarh"},  {"lat",30.73},  {"lon",76.78},
                {"name","Kochi"},       {"lat",9.93},   {"lon",76.26},
                {"name","Bhopal"},      {"lat",23.25},  {"lon",77.40},
                {"name","Patna"},       {"lat",25.59},  {"lon",85.14},
                {"name","Guwahati"},    {"lat",26.14},  {"lon",91.74},
                {"name","Srinagar"},    {"lat",34.08},  {"lon",74.80},
                {"name","Thiruvananthapuram"},{"lat",8.52},{"lon",76.94},
                {"name","Bhubaneswar"},{"lat",20.30},   {"lon",85.84},
                {"name","Vadodara"},    {"lat",22.30},  {"lon",73.20}
            }}
        };

        nlohmann::json resp_json = {{"success", true}, {"data", cities}};
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(resp_json.dump());
        cb(resp);
    }

private:
    WeatherService weather_;

    static drogon::HttpResponsePtr error_response(const std::string& msg) {
        nlohmann::json j = {{"success", false}, {"error", msg}};
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(j.dump());
        return resp;
    }
};

} // namespace news
