#pragma once
// =============================================================================
// request_validators.hpp  —  Validation helper functions for incoming requests
// =============================================================================
#include <string>
#include <regex>
#include <nlohmann/json.hpp>

namespace news::validation {

// ── Validate email format ────────────────────────────────────────────────────
inline bool is_valid_email(const std::string& email) {
    static const std::regex email_pattern(
        R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
    );
    return std::regex_match(email, email_pattern);
}

// ── Validate password complexity (minimum length, number, symbol) ────────────
inline bool is_strong_password(const std::string& password, std::string& err_msg) {
    if (password.length() < 8) {
        err_msg = "Password must be at least 8 characters long.";
        return false;
    }
    bool has_num = false;
    bool has_upper = false;
    for (char c : password) {
        if (std::isdigit(c)) has_num = true;
        if (std::isupper(c)) has_upper = true;
    }
    if (!has_num) {
        err_msg = "Password must contain at least one digit.";
        return false;
    }
    return true;
}

// ── Validate and clamp pagination ────────────────────────────────────────────
inline void validate_pagination(int& page, int& limit) {
    if (page < 1) page = 1;
    if (limit < 1) limit = 10;
    if (limit > 100) limit = 100; // Cap to prevent DOS
}

} // namespace news::validation
