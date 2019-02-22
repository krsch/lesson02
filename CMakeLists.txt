cmake_minimum_required(VERSION 3.12.0)
project(test-vscode VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

add_subdirectory(vendor/fmt)
add_subdirectory(vendor/GSL)

find_package(Boost REQUIRED system)
find_package(Threads)
find_package(OpenSSL)

add_executable(lesson02 main.cpp init.cpp)
target_link_libraries(lesson02 fmt::fmt GSL Boost::boost Boost::system Threads::Threads OpenSSL::SSL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
