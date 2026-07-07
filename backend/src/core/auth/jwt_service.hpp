#pragma once
// =============================================================================
// jwt_service.hpp  —  JWT sign / verify with HS256 (RS256-ready)
// =============================================================================
#include <string>
#include <optional>
#include <chrono>
#include <stdexcept>
#include <jwt-cpp/jwt.h>
#include "config/app_config.hpp"

namespace news::auth {

struct TokenClaims {
    std::string user_id;
    std::string username;
    std::string role;
    std::string jti;       // JWT ID (for revocation)
    std::string token_type; // "access" | "refresh"
};

class JwtService {
public:
    explicit JwtService(const JWTConfig& cfg = get_config().jwt)
        : cfg_(cfg) {}

    // ── Sign an access token ─────────────────────────────────────────────────
    [[nodiscard]] std::string sign_access(const TokenClaims& claims) const {
        auto now = std::chrono::system_clock::now();
        return jwt::create()
            .set_issuer(cfg_.issuer)
            .set_type("JWT")
            .set_subject(claims.user_id)
            .set_id(claims.jti)
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::seconds(cfg_.access_ttl_sec))
            .set_payload_claim("username",   jwt::claim(claims.username))
            .set_payload_claim("role",       jwt::claim(claims.role))
            .set_payload_claim("token_type", jwt::claim(std::string("access")))
            .sign(jwt::algorithm::hs256{cfg_.secret});
    }

    // ── Sign a refresh token ─────────────────────────────────────────────────
    [[nodiscard]] std::string sign_refresh(const std::string& user_id,
                                           const std::string& jti) const {
        auto now = std::chrono::system_clock::now();
        return jwt::create()
            .set_issuer(cfg_.issuer)
            .set_type("JWT")
            .set_subject(user_id)
            .set_id(jti)
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::seconds(cfg_.refresh_ttl_sec))
            .set_payload_claim("token_type", jwt::claim(std::string("refresh")))
            .sign(jwt::algorithm::hs256{cfg_.secret});
    }

    // ── Verify and decode ────────────────────────────────────────────────────
    [[nodiscard]] std::optional<TokenClaims> verify(const std::string& token) const noexcept {
        try {
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{cfg_.secret})
                .with_issuer(cfg_.issuer);

            auto decoded = jwt::decode(token);
            verifier.verify(decoded);

            TokenClaims claims;
            claims.user_id    = decoded.get_subject();
            claims.jti        = decoded.get_id();
            claims.token_type = decoded.get_payload_claim("token_type").as_string();

            if (decoded.has_payload_claim("username"))
                claims.username = decoded.get_payload_claim("username").as_string();
            if (decoded.has_payload_claim("role"))
                claims.role = decoded.get_payload_claim("role").as_string();

            return claims;
        } catch (...) {
            return std::nullopt;
        }
    }

    // ── Extract bearer token from Authorization header ───────────────────────
    [[nodiscard]] static std::optional<std::string>
    extract_bearer(const std::string& header) {
        constexpr std::string_view prefix = "Bearer ";
        if (header.size() > prefix.size() &&
            header.substr(0, prefix.size()) == prefix) {
            return header.substr(prefix.size());
        }
        return std::nullopt;
    }

private:
    JWTConfig cfg_;
};

} // namespace news::auth
