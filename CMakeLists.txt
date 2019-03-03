cmake_minimum_required(VERSION 3.8.0)
project(lesson02 VERSION 0.1.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(vendor/fmt)
add_subdirectory(vendor/GSL)
add_subdirectory(vendor/json)

find_package(Boost REQUIRED system)
find_package(Threads)
find_package(OpenSSL)

add_executable(lesson02 main.cpp init.cpp)
target_link_libraries(lesson02 PRIVATE fmt::fmt GSL Boost::boost Boost::system Threads::Threads OpenSSL::SSL nlohmann_json::nlohmann_json)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
