cmake_minimum_required(VERSION 3.20)
project(ai_news_platform VERSION 1.0.0 LANGUAGES CXX)

# ─────────────────────────────────────────────
# C++20 Standard
# ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────────────────────────
# Build type
# ─────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined")

# ─────────────────────────────────────────────
# Find dependencies
# ─────────────────────────────────────────────
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost 1.81 REQUIRED COMPONENTS system filesystem)
find_package(Threads REQUIRED)

# JWT-cpp (header-only)
find_path(JWT_CPP_INCLUDE_DIR NAMES jwt-cpp/jwt.h)

# librdkafka
find_library(RDKAFKA_LIB    rdkafka)
find_library(RDKAFKA_CPP_LIB rdkafka++)
find_path(RDKAFKA_INCLUDE_DIR NAMES librdkafka/rdkafkacpp.h)

# hiredis + redis-plus-plus
find_library(HIREDIS_LIB hiredis)
find_library(REDIS_PLUS_PLUS_LIB redis++)
find_path(REDIS_PLUS_PLUS_INCLUDE_DIR NAMES sw/redis++/redis++.h)

# libcurl (for news fetcher + HTTP calls)
find_package(CURL REQUIRED)

# nlohmann/json (header-only)
find_package(nlohmann_json 3.11 REQUIRED)

# spdlog
find_package(spdlog REQUIRED)

# ─────────────────────────────────────────────
# Collect all source files
# ─────────────────────────────────────────────
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# ─────────────────────────────────────────────
# Main executable
# ─────────────────────────────────────────────
add_executable(news_server ${SOURCES})

target_include_directories(news_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${JWT_CPP_INCLUDE_DIR}
    ${RDKAFKA_INCLUDE_DIR}
    ${REDIS_PLUS_PLUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(news_server PRIVATE
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
    Boost::filesystem
    Threads::Threads
    CURL::libcurl
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${RDKAFKA_LIB}
    ${RDKAFKA_CPP_LIB}
    ${HIREDIS_LIB}
    ${REDIS_PLUS_PLUS_LIB}
)

# ─────────────────────────────────────────────
# Tests
# ─────────────────────────────────────────────
enable_testing()
find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

add_executable(news_tests ${TEST_SOURCES}
    # Include non-main sources for testing
    src/core/auth/jwt_service.cpp
    src/core/news/news_repository.cpp
    src/infrastructure/redis_client.cpp
)

target_include_directories(news_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${JWT_CPP_INCLUDE_DIR}
    ${REDIS_PLUS_PLUS_INCLUDE_DIR}
)

target_link_libraries(news_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${HIREDIS_LIB}
    ${REDIS_PLUS_PLUS_LIB}
)

add_test(NAME AllTests COMMAND news_tests)

# ─────────────────────────────────────────────
# Install
# ─────────────────────────────────────────────
install(TARGETS news_server DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/config DESTINATION etc/news_server)
