cmake_minimum_required(VERSION 3.10)
project(ET)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
file(GLOB_RECURSE SOURCE_FILES
    "src/*.c++"
    "src/*.hpp"
    "src/*.cpp"
    "include/tiny_obj_loader.h"
)
add_executable(ET ${SOURCE_FILES})
target_include_directories(ET
    PRIVATE
        src
)
target_link_libraries(ET
    PRIVATE
        Vulkan::Vulkan
        glfw
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
)