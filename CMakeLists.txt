cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(quicklinks)
add_executable(quicklinks WIN32 src/main.cpp src/pch.cpp src/icon.rc)
target_precompile_headers(quicklinks PRIVATE src/pch.h)

find_package(wil CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(quicklinks PRIVATE WIL::WIL nlohmann_json::nlohmann_json)
