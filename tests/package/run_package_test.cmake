cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED ADASDF_CL_INSTALL_PREFIX)
  message(FATAL_ERROR "ADASDF_CL_INSTALL_PREFIX is required.")
endif()
if(NOT DEFINED ADASDF_CL_PACKAGE_TEST_BUILD)
  message(FATAL_ERROR "ADASDF_CL_PACKAGE_TEST_BUILD is required.")
endif()

if(NOT DEFINED ADASDF_CL_PACKAGE_TEST_SOURCE)
  set(ADASDF_CL_PACKAGE_TEST_SOURCE "${CMAKE_CURRENT_LIST_DIR}")
endif()
if(NOT DEFINED ADASDF_CL_CONFIG)
  set(ADASDF_CL_CONFIG "Release")
endif()

file(TO_CMAKE_PATH "${ADASDF_CL_INSTALL_PREFIX}" _adasdf_install_prefix)
file(TO_CMAKE_PATH "${ADASDF_CL_PACKAGE_TEST_SOURCE}" _adasdf_test_source)
file(TO_CMAKE_PATH "${ADASDF_CL_PACKAGE_TEST_BUILD}" _adasdf_test_build)

message(STATUS "Configuring AdaSDF-CL package consumer")
execute_process(
  COMMAND "${CMAKE_COMMAND}"
    -S "${_adasdf_test_source}"
    -B "${_adasdf_test_build}"
    "-DCMAKE_PREFIX_PATH=${_adasdf_install_prefix}"
    "-DCMAKE_BUILD_TYPE=${ADASDF_CL_CONFIG}"
  RESULT_VARIABLE _configure_result)
if(NOT _configure_result EQUAL 0)
  message(FATAL_ERROR "Package consumer configure failed.")
endif()

message(STATUS "Building AdaSDF-CL package consumer")
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_adasdf_test_build}"
    --config "${ADASDF_CL_CONFIG}" --parallel
  RESULT_VARIABLE _build_result)
if(NOT _build_result EQUAL 0)
  message(FATAL_ERROR "Package consumer build failed.")
endif()

message(STATUS "Running AdaSDF-CL package consumer")
execute_process(
  COMMAND "${CMAKE_CTEST_COMMAND}"
    --test-dir "${_adasdf_test_build}"
    -C "${ADASDF_CL_CONFIG}"
    --output-on-failure
  RESULT_VARIABLE _test_result)
if(NOT _test_result EQUAL 0)
  message(FATAL_ERROR "Package consumer test failed.")
endif()
