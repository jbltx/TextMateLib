cmake_minimum_required(VERSION 3.14)
project(PackageTest CXX)

find_package(TextMateLib REQUIRED CONFIG)

add_executable(example src/example.cpp)
target_link_libraries(example PRIVATE TextMateLib::tml)
