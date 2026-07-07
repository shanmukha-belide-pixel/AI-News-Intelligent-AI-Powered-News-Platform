#include <gtest/gtest.h>
#include "core/auth/jwt_service.hpp"

using namespace news::auth;

TEST(JwtServiceTest, SignAndVerifyAccessToken) {
    news::JWTConfig cfg;
    cfg.secret = "test_super_secret_key_which_is_long_enough_for_hs256";
    cfg.issuer = "test-issuer";
    cfg.access_ttl_sec = 3600;

    JwtService service(cfg);

    TokenClaims claims;
    claims.user_id = "user_12345";
    claims.username = "testuser";
    claims.role = "user";
    claims.jti = "jti_abcde";
    claims.token_type = "access";

    std::string token = service.sign_access(claims);
    ASSERT_FALSE(token.empty());

    auto verified = service.verify(token);
    ASSERT_TRUE(verified.has_value());
    EXPECT_EQ(verified->user_id, claims.user_id);
    EXPECT_EQ(verified->username, claims.username);
    EXPECT_EQ(verified->role, claims.role);
    EXPECT_EQ(verified->jti, claims.jti);
    EXPECT_EQ(verified->token_type, "access");
}

TEST(JwtServiceTest, SignAndVerifyRefreshToken) {
    news::JWTConfig cfg;
    cfg.secret = "test_super_secret_key_which_is_long_enough_for_hs256";
    cfg.issuer = "test-issuer";
    cfg.refresh_ttl_sec = 86400;

    JwtService service(cfg);

    std::string user_id = "user_99999";
    std::string jti = "refresh_jti_123";

    std::string token = service.sign_refresh(user_id, jti);
    ASSERT_FALSE(token.empty());

    auto verified = service.verify(token);
    ASSERT_TRUE(verified.has_value());
    EXPECT_EQ(verified->user_id, user_id);
    EXPECT_EQ(verified->jti, jti);
    EXPECT_EQ(verified->token_type, "refresh");
}

TEST(JwtServiceTest, RejectInvalidOrExpiredTokens) {
    news::JWTConfig cfg;
    cfg.secret = "test_super_secret_key_which_is_long_enough_for_hs256";
    cfg.issuer = "test-issuer";
    cfg.access_ttl_sec = 3600;

    JwtService service(cfg);

    // 1. Invalid signature
    std::string bad_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.invalid_signature";
    auto verified = service.verify(bad_token);
    EXPECT_FALSE(verified.has_value());

    // 2. Different secret
    news::JWTConfig other_cfg = cfg;
    other_cfg.secret = "some_other_secret_key_used_to_sign_this_bad_token";
    JwtService other_service(other_cfg);

    TokenClaims claims;
    claims.user_id = "user_123";
    claims.token_type = "access";
    
    std::string token_from_other = other_service.sign_access(claims);
    auto verified_with_original = service.verify(token_from_other);
    EXPECT_FALSE(verified_with_original.has_value());
}

TEST(JwtServiceTest, ExtractBearerToken) {
    auto t1 = JwtService::extract_bearer("Bearer token123");
    ASSERT_TRUE(t1.has_value());
    EXPECT_EQ(*t1, "token123");

    auto t2 = JwtService::extract_bearer("InvalidHeader format");
    EXPECT_FALSE(t2.has_value());

    auto t3 = JwtService::extract_bearer("Bearer ");
    EXPECT_FALSE(t3.has_value());
}
