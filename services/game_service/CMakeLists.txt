cmake_minimum_required(VERSION 3.20)
project(et_game_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# nlohmann/json — JSON parsing
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# spdlog — logging
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)

# cpp-httplib — HTTP client (header-only)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)

# IXWebSocket — WebSocket server
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.5
)

set(USE_TLS OFF CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(nlohmann_json spdlog httplib ixwebsocket)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RABBITMQ REQUIRED librabbitmq)

file(GLOB_RECURSE MODELS_SOURCES CONFIGURE_DEPENDS "models/*.cc")
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "engine/*.cc")

add_executable(et_game_service
    main.cc
    ${MODELS_SOURCES}
    ${ENGINE_SOURCES}
)

target_include_directories(et_game_service PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${RABBITMQ_INCLUDE_DIRS}
)

target_link_libraries(et_game_service PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
    ixwebsocket
    ${RABBITMQ_LIBRARIES}
)

target_link_directories(et_game_service PRIVATE
    ${RABBITMQ_LIBRARY_DIRS}
)

