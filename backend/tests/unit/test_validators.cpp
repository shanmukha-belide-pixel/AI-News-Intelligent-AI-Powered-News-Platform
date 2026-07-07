#include <gtest/gtest.h>
#include "api/validators/request_validators.hpp"

using namespace news::validation;

TEST(RequestValidatorsTest, EmailValidation) {
    EXPECT_TRUE(is_valid_email("user@example.com"));
    EXPECT_TRUE(is_valid_email("john.doe+label@mail.co.uk"));
    EXPECT_TRUE(is_valid_email("a@b.cd"));

    EXPECT_FALSE(is_valid_email("plainaddress"));
    EXPECT_FALSE(is_valid_email("#@%^%#$@#$@#.com"));
    EXPECT_FALSE(is_valid_email("@example.com"));
    EXPECT_FALSE(is_valid_email("user@"));
    EXPECT_FALSE(is_valid_email("user@localhost")); // Needs a TLD
}

TEST(RequestValidatorsTest, PasswordStrength) {
    std::string err;
    
    // Strong password
    EXPECT_TRUE(is_strong_password("SecurePass123", err));

    // Too short
    EXPECT_FALSE(is_strong_password("Sh0rt", err));
    EXPECT_EQ(err, "Password must be at least 8 characters long.");

    // No digits
    EXPECT_FALSE(is_strong_password("NoDigitsHere", err));
    EXPECT_EQ(err, "Password must contain at least one digit.");
}

TEST(RequestValidatorsTest, PaginationClamping) {
    int page = 0;
    int limit = 200;
    
    validate_pagination(page, limit);
    EXPECT_EQ(page, 1);       // 0 clamped to 1
    EXPECT_EQ(limit, 100);    // 200 clamped to 100 max
    
    page = 5;
    limit = 25;
    validate_pagination(page, limit);
    EXPECT_EQ(page, 5);       // unmodified
    EXPECT_EQ(limit, 25);     // unmodified
}
