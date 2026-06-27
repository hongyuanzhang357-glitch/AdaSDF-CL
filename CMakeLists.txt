cmake_minimum_required(VERSION 3.20)

include(cmake/adasdf_cl_version.cmake)

project(AdaSDFCL
  VERSION ${ADASDF_CL_VERSION}
  DESCRIPTION "Adaptive Signed Distance Field Collision Library"
  LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(ADASDF_CL_BUILD_EXAMPLES "Expose AdaSDF-CL example source targets" OFF)
option(ADASDF_CL_BUILD_TESTS "Expose AdaSDF-CL interface test source targets" OFF)
option(ADASDF_CL_ENABLE_CUDA "Reserve CUDA backend integration hooks" OFF)
option(ADASDF_CL_USE_EXISTING_CORE "Use existing OctreeBlockLowRankSDF core for sdfbin loading and query" ON)
option(ADASDF_CL_ENABLE_ADAPTIVE_BUILDER "Enable adaptive SDF builder API" ON)
option(ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER "Enable DASH-SDF surrogate recommendation API" ON)

set(ADASDF_CL_EXISTING_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH
  "Path to the existing OctreeBlockLowRankSDF project root")

set(_adasdf_vcpkg_prefix_candidates
  "${ADASDF_CL_EXISTING_ROOT}/build/windows-debug/vcpkg_installed/x64-windows"
  "${ADASDF_CL_EXISTING_ROOT}/build/windows-release/vcpkg_installed/x64-windows"
  "${ADASDF_CL_EXISTING_ROOT}/build/windows-release-cuda/vcpkg_installed/x64-windows"
  "${ADASDF_CL_EXISTING_ROOT}/build/windows-release-fcl/vcpkg_installed/x64-windows-vs2022")

foreach(_prefix IN LISTS _adasdf_vcpkg_prefix_candidates)
  if(EXISTS "${_prefix}")
    list(APPEND CMAKE_PREFIX_PATH "${_prefix}")
  endif()
endforeach()

add_library(adasdf_cl INTERFACE)
add_library(adasdf::cl ALIAS adasdf_cl)

target_compile_features(adasdf_cl INTERFACE cxx_std_17)
target_include_directories(adasdf_cl INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_compile_definitions(adasdf_cl INTERFACE
  ADASDF_CL_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  ADASDF_CL_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  ADASDF_CL_VERSION_PATCH=${PROJECT_VERSION_PATCH}
  ADASDF_CL_VERSION_SUFFIX="${ADASDF_CL_VERSION_SUFFIX}"
  ADASDF_CL_ENABLE_CUDA=$<BOOL:${ADASDF_CL_ENABLE_CUDA}>)

set(ADASDF_CL_RUNTIME_SOURCES
  src/adapters/FCLAdapter.cpp
  src/backend/Backend.cpp
  src/backend/GpuBackend.cpp
  src/compression/CompressedSDF.cpp
  src/generation/AdaptiveSDFBuilder.cpp
  src/generation/ExistingBuilderBridge.cpp
  src/generation/SDFBuilder.cpp
  src/generation/SurrogateRecommender.cpp
  src/geometry/MeshModel.cpp
  src/geometry/SDFModel.cpp
  src/geometry/Transform.cpp
  src/io/ContactOnlySDFBin.cpp
  src/io/ExistingSDFBridge.cpp
  src/io/SDFBinReader.cpp
  src/io/SDFBinWriter.cpp
  src/query/CandidatePointSampler.cpp
  src/query/CollisionObject.cpp
  src/query/CollisionResult.cpp
  src/query/ContactGenerator.cpp
  src/query/ContactReducer.cpp
  src/query/CpuNarrowPhase.cpp
  src/query/DistanceResult.cpp
  src/query/ExistingPairCollisionBridge.cpp
  src/query/QueryAPI.cpp)

set(ADASDF_CL_HAS_EXISTING_CORE OFF)

if(ADASDF_CL_USE_EXISTING_CORE)
  find_package(Eigen3 CONFIG QUIET)
  find_package(nlohmann_json CONFIG QUIET)
  find_package(fmt CONFIG QUIET)

  set(_existing_required_source
    "${ADASDF_CL_EXISTING_ROOT}/src/ModelIO.cpp")
  if(EXISTS "${_existing_required_source}" AND TARGET Eigen3::Eigen AND
      TARGET nlohmann_json::nlohmann_json AND TARGET fmt::fmt)
    set(ADASDF_CL_HAS_EXISTING_CORE ON)
  else()
    message(WARNING
      "ADASDF_CL_USE_EXISTING_CORE=ON but the existing core or dependencies "
      "were not found. Building AdaSDF-CL runtime without real sdfbin query bridge.")
  endif()
endif()

if(ADASDF_CL_HAS_EXISTING_CORE)
  set(ADASDF_CL_EXISTING_CORE_SOURCES
    "${ADASDF_CL_EXISTING_ROOT}/src/AABB.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/AdaptiveBlockCompressor.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/AdaptiveLowRankModel.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/AllPointsSDFContact.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/BlockPartition.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/BVH.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/BVHCandidateSampler.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/BVHPairFilter.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/ContactPointExporter.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/ContactPipelineDecisionReport.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/ContactSummaryReport.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/DenseBlockGenerator.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/DenseBlockCache.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/FCLContactExporter.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/GlobalDenseGrid.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/LowRankCompressionStats.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/LowRankModel.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/LowRankReconstruction.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/MatrixSVDCompressor.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/Mesh.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/ModelIO.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/OctreeSDF.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/PairCollisionCaseCalibrator.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/PairCollisionSDF.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/QueryPointGenerator.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/RayTriangle.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/ReportGenerator.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/RobustCollisionCaseCalibrator.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/SDFBooleanEarlyExit.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/SDFModelPackage.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/SDFNormal.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/SDFQueryBenchmark.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/SignedDistance.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/StlReader.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/StreamingBVHSDFContact.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/SurfaceSampler.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/Transform3D.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/TraversalProfiler.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/TriangleDistance.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/TrianglePairFilter.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/TTCompressor.cpp"
    "${ADASDF_CL_EXISTING_ROOT}/src/TuckerCompressor.cpp")

  add_library(adasdf_existing_core STATIC ${ADASDF_CL_EXISTING_CORE_SOURCES})
  target_compile_features(adasdf_existing_core PUBLIC cxx_std_20)
  target_include_directories(adasdf_existing_core PUBLIC
    $<BUILD_INTERFACE:${ADASDF_CL_EXISTING_ROOT}/include>)
  target_link_libraries(adasdf_existing_core PUBLIC
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
    fmt::fmt)
  target_compile_definitions(adasdf_existing_core PUBLIC
    SDF_PROJECT_VERSION="${PROJECT_VERSION}"
    SDF_ENABLE_FCL=0
    SDF_HAS_FCL=0
    SDF_ENABLE_CUDA=0
    SDF_HAS_CUDA=0
    SDF_ENABLE_OPENMP=0)
endif()

message(STATUS "")
message(STATUS "AdaSDF-CL configuration:")
message(STATUS "  Version: ${ADASDF_CL_VERSION_STRING}")
message(STATUS "  Build examples: ${ADASDF_CL_BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${ADASDF_CL_BUILD_TESTS}")
message(STATUS "  Existing core requested: ${ADASDF_CL_USE_EXISTING_CORE}")
message(STATUS "  Existing core found: ${ADASDF_CL_HAS_EXISTING_CORE}")
message(STATUS "  Adaptive builder: ${ADASDF_CL_ENABLE_ADAPTIVE_BUILDER}")
message(STATUS "  Surrogate recommender: ${ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER}")
message(STATUS "  CUDA: not required")
message(STATUS "  FCL: not required")
message(STATUS "")

add_library(adasdf_cl_runtime STATIC ${ADASDF_CL_RUNTIME_SOURCES})
add_library(adasdf::cl_runtime ALIAS adasdf_cl_runtime)
target_compile_features(adasdf_cl_runtime PUBLIC cxx_std_17)
target_link_libraries(adasdf_cl_runtime PUBLIC adasdf::cl)
target_compile_definitions(adasdf_cl_runtime PUBLIC
  ADASDF_CL_HAS_EXISTING_CORE=$<BOOL:${ADASDF_CL_HAS_EXISTING_CORE}>
  ADASDF_CL_ENABLE_ADAPTIVE_BUILDER=$<BOOL:${ADASDF_CL_ENABLE_ADAPTIVE_BUILDER}>
  ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=$<BOOL:${ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER}>)

if(ADASDF_CL_HAS_EXISTING_CORE)
  target_link_libraries(adasdf_cl_runtime PRIVATE adasdf_existing_core)
endif()

set(ADASDF_CL_EXAMPLE_SOURCES
  examples/01_build_adaptive_sdf.cpp
  examples/02_load_sdfbin_and_query.cpp
  examples/03_collision_between_two_objects.cpp
  examples/04_batched_gpu_query.cpp
  examples/05_fcl_style_api.cpp
  examples/07_contact_reduction_demo.cpp)

if(ADASDF_CL_BUILD_EXAMPLES)
  add_executable(adasdf_load_sdfbin_and_query
    examples/02_load_sdfbin_and_query.cpp)
  target_link_libraries(adasdf_load_sdfbin_and_query PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_collision_between_two_objects
    examples/03_collision_between_two_objects.cpp)
  target_link_libraries(adasdf_collision_between_two_objects PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_fcl_style_api
    examples/05_fcl_style_api.cpp)
  target_link_libraries(adasdf_fcl_style_api PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_build_then_query
    examples/06_build_then_query.cpp)
  target_link_libraries(adasdf_build_then_query PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_contact_reduction_demo
    examples/07_contact_reduction_demo.cpp)
  target_link_libraries(adasdf_contact_reduction_demo PRIVATE adasdf::cl_runtime)

  add_custom_target(adasdf_cl_api_preview_examples SOURCES
    examples/01_build_adaptive_sdf.cpp
    examples/04_batched_gpu_query.cpp)
endif()

add_executable(adasdf_build tools/adasdf_build.cpp)
target_link_libraries(adasdf_build PRIVATE adasdf::cl_runtime)

set(ADASDF_CL_TEST_SOURCES
  tests/test_sdf_io.cpp
  tests/test_collision_query.cpp
  tests/test_distance_query.cpp
  tests/test_collision_object.cpp
  tests/test_pair_distance_query.cpp
  tests/test_pair_collision_query.cpp
  tests/test_candidate_point_sampler.cpp
  tests/test_contact_generator.cpp
  tests/test_contact_reducer.cpp
  tests/test_cpu_narrowphase.cpp
  tests/test_build_options.cpp
  tests/test_adaptive_builder.cpp
  tests/test_sdfbin_writer_roundtrip.cpp
  tests/test_surrogate_recommender.cpp
  tests/test_contact_only_sdfbin.cpp
  tests/test_sdfbin_real_load.cpp
  tests/test_sdfmodel_query.cpp)

file(GLOB_RECURSE ADASDF_CL_DISCOVERED_FIXTURES
  "${ADASDF_CL_EXISTING_ROOT}/models/*.sdfbin")
list(SORT ADASDF_CL_DISCOVERED_FIXTURES)
list(LENGTH ADASDF_CL_DISCOVERED_FIXTURES _adasdf_fixture_count)
if(_adasdf_fixture_count GREATER 0)
  list(GET ADASDF_CL_DISCOVERED_FIXTURES 0 ADASDF_CL_TEST_FIXTURE)
else()
  set(ADASDF_CL_TEST_FIXTURE "")
endif()

if(ADASDF_CL_BUILD_TESTS)
  enable_testing()
  set(ADASDF_CL_TEST_CUBE_STL
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/cube_closed_ascii.stl")
  foreach(test_source IN LISTS ADASDF_CL_TEST_SOURCES)
    get_filename_component(test_name "${test_source}" NAME_WE)
    add_executable(${test_name} "${test_source}")
    target_link_libraries(${test_name} PRIVATE adasdf::cl_runtime)
    target_compile_definitions(${test_name} PRIVATE
      ADASDF_CL_TEST_FIXTURE="${ADASDF_CL_TEST_FIXTURE}"
      ADASDF_CL_TEST_CUBE_STL="${ADASDF_CL_TEST_CUBE_STL}")
    add_test(NAME ${test_name} COMMAND ${test_name})
  endforeach()
endif()

set_target_properties(adasdf_cl PROPERTIES EXPORT_NAME adasdf_cl)
set_target_properties(adasdf_cl_runtime PROPERTIES EXPORT_NAME adasdf_cl_runtime)
set(ADASDF_CL_INSTALL_TARGETS adasdf_cl adasdf_cl_runtime)
if(ADASDF_CL_HAS_EXISTING_CORE)
  set_target_properties(adasdf_existing_core PROPERTIES EXPORT_NAME adasdf_existing_core)
  list(APPEND ADASDF_CL_INSTALL_TARGETS adasdf_existing_core)
endif()
set(ADASDF_CL_INSTALL_RUNTIME_TARGET TRUE)

install(TARGETS ${ADASDF_CL_INSTALL_TARGETS}
  EXPORT AdaSDFCLTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT AdaSDFCLTargets
  FILE AdaSDFCLTargets.cmake
  NAMESPACE AdaSDFCL::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdaSDFCL)

configure_package_config_file(
  cmake/AdaSDFCLConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/AdaSDFCLConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdaSDFCL)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/AdaSDFCLConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/AdaSDFCLConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/AdaSDFCLConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdaSDFCL)
