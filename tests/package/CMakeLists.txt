cmake_minimum_required(VERSION 3.20)

project(AdaSDFCLPackageConsumer LANGUAGES CXX)

find_package(AdaSDFCL CONFIG REQUIRED)

add_executable(test_find_package test_find_package.cpp)
target_link_libraries(test_find_package PRIVATE AdaSDFCL::adasdf_cl)
target_compile_features(test_find_package PRIVATE cxx_std_17)

enable_testing()
add_test(NAME AdaSDFCLPackageConsumer COMMAND test_find_package)
