cmake_minimum_required(VERSION 3.20)

project(AdaSDFCLDownstreamExample LANGUAGES CXX)

find_package(AdaSDFCL CONFIG REQUIRED)

add_executable(adasdf_downstream main.cpp)
target_link_libraries(adasdf_downstream PRIVATE AdaSDFCL::adasdf_cl)
target_compile_features(adasdf_downstream PRIVATE cxx_std_17)
