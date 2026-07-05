cmake_minimum_required(VERSION 3.20)

include(cmake/adasdf_cl_version.cmake)

project(AdaSDFCL
  VERSION ${ADASDF_CL_VERSION}
  DESCRIPTION "Adaptive Signed Distance Field Collision Library"
  LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Git QUIET)
set(ADASDF_CL_GIT_COMMIT "unknown")
if(Git_FOUND)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE _adasdf_git_result
    OUTPUT_VARIABLE _adasdf_git_commit
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(_adasdf_git_result EQUAL 0 AND _adasdf_git_commit)
    set(ADASDF_CL_GIT_COMMIT "${_adasdf_git_commit}")
  endif()
endif()

option(ADASDF_CL_BUILD_EXAMPLES "Expose AdaSDF-CL example source targets" OFF)
option(ADASDF_CL_BUILD_TESTS "Expose AdaSDF-CL interface test source targets" OFF)
option(ADASDF_CL_BUILD_BENCHMARKS "Build AdaSDF-CL benchmark tools" ON)
option(ADASDF_CL_ENABLE_CUDA "Enable optional CUDA batch query backend" OFF)
option(ADASDF_CL_USE_EXISTING_CORE "Use existing OctreeBlockLowRankSDF core for sdfbin loading and query" ON)
option(ADASDF_CL_ENABLE_ADAPTIVE_BUILDER "Enable adaptive SDF builder API" ON)
option(ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER "Enable DASH-SDF surrogate recommendation API" ON)
option(ADASDF_CL_ENABLE_DEMO_BACKEND "Enable core-free analytic demo SDF backend" ON)
option(ADASDF_CL_ENABLE_DEMO_SURROGATE "Enable demo surrogate recommender" ON)
option(ADASDF_CL_ENABLE_COLLISION_VIEWER "Enable SVG collision visualization" ON)

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
  ADASDF_CL_GIT_COMMIT="${ADASDF_CL_GIT_COMMIT}"
  ADASDF_CL_ENABLE_CUDA=$<BOOL:${ADASDF_CL_ENABLE_CUDA}>)

set(ADASDF_CL_HAS_CUDA_BACKEND OFF)
set(ADASDF_CL_CUDA_TOOLKIT_STATUS "not found")

if(ADASDF_CL_ENABLE_CUDA)
  include(CheckLanguage)
  check_language(CUDA)
  if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
      set(ADASDF_CL_HAS_CUDA_BACKEND ON)
      set(ADASDF_CL_CUDA_TOOLKIT_STATUS "found")
    else()
      message(WARNING
        "ADASDF_CL_ENABLE_CUDA=ON but CUDAToolkit was not found. "
        "Building without CUDA batch query kernels.")
    endif()
  else()
    message(WARNING
      "ADASDF_CL_ENABLE_CUDA=ON but no CUDA compiler was found. "
      "Building without CUDA batch query kernels.")
  endif()
endif()

target_compile_definitions(adasdf_cl INTERFACE
  ADASDF_CL_HAS_CUDA_BACKEND=$<BOOL:${ADASDF_CL_HAS_CUDA_BACKEND}>)

set(ADASDF_CL_RUNTIME_SOURCES
  src/acceleration/BuildAccelerationReport.cpp
  src/acceleration/TriangleAABB.cpp
  src/acceleration/TriangleBVH.cpp
  src/acceleration/TriangleBVHBuilder.cpp
  src/acceleration/BVHNearestTriangleQuery.cpp
  src/acceleration/BVHRayIntersectionQuery.cpp
  src/acceleration/BVHSDFSampler.cpp
  src/acceleration/ParallelSampling.cpp
  src/adapters/FCLAdapter.cpp
  src/backend/Backend.cpp
  src/backend/CudaQueryBackend.cpp
  src/backend/GpuBackend.cpp
  src/benchmark/PointCloudGenerator.cpp
  src/contact/ContactBudget.cpp
  src/contact/ContactPatch.cpp
  src/contact/ContactClusterer.cpp
  src/contact/ContactStabilizer.cpp
  src/contact/SolverContact.cpp
  src/contact/SolverContactExporter.cpp
  src/contact/ContactStabilizationReportWriter.cpp
  src/compression/CompressedSDF.cpp
  src/compression/SmallMatrixSVD.cpp
  src/compression/CompressedSDFBlock.cpp
  src/compression/BlockLowRankCompressor.cpp
  src/compression/CompressionQuality.cpp
  src/compression/CompressionReportWriter.cpp
  src/generation/AdaptiveSDFBuilder.cpp
  src/generation/AdaptiveSDFBuilderPreview.cpp
  src/generation/AdaptiveOctree.cpp
  src/generation/AdaptiveOctreeBuilder.cpp
  src/generation/AdaptiveBlock.cpp
  src/generation/AdaptiveBlockPartitioner.cpp
  src/generation/AdaptiveBlockSDFBuilder.cpp
  src/generation/AdaptiveBlockSDFBuildReportWriter.cpp
  src/generation/DemoAdaptiveSDFBuilder.cpp
  src/generation/DemoSurrogateRecommender.cpp
  src/generation/DenseSDFBuilder.cpp
  src/generation/DenseSDFBuildReportWriter.cpp
  src/generation/ExistingBuilderBridge.cpp
  src/generation/SDFBuilder.cpp
  src/generation/SurrogateRecommender.cpp
  src/geometry/AnalyticSDFModel.cpp
  src/geometry/AdaptiveBlockSDFModel.cpp
  src/geometry/CompressedAdaptiveBlockSDFModel.cpp
  src/geometry/DenseSDFModel.cpp
  src/geometry/DemoAdaptiveSDFModel.cpp
  src/geometry/BoxTriangleDistance.cpp
  src/geometry/MeshModel.cpp
  src/geometry/SDFModel.cpp
  src/geometry/Transform.cpp
  src/mesh/TriangleMesh.cpp
  src/mesh/STLReader.cpp
  src/mesh/STLWriter.cpp
  src/mesh/TriangleDistance.cpp
  src/mesh/MeshSign.cpp
  src/mesh/MeshDiagnostics.cpp
  src/mesh/MeshReadiness.cpp
  src/mesh/MeshCleanup.cpp
  src/mesh/MeshDiagnosticsWriter.cpp
  src/io/ContactOnlySDFBin.cpp
  src/io/AdaptiveBlockSDFBin.cpp
  src/io/CompressedBlockSDFBin.cpp
  src/io/DenseSDFBin.cpp
  src/io/DemoAdaptiveSDFBin.cpp
  src/io/DemoSDFBin.cpp
  src/io/ExistingSDFBridge.cpp
  src/io/SDFBinReader.cpp
  src/io/SDFBinWriter.cpp
  src/query/CandidatePointSampler.cpp
  src/query/BatchQuery.cpp
  src/query/CollisionObject.cpp
  src/query/CollisionResult.cpp
  src/query/ContactGenerator.cpp
  src/query/ContactReducer.cpp
  src/query/CpuNarrowPhase.cpp
  src/query/DistanceResult.cpp
  src/query/ExpandedSDF.cpp
  src/query/ExpansionQuality.cpp
  src/query/ExistingPairCollisionBridge.cpp
  src/query/QueryEngine.cpp
  src/query/QueryModeConfig.cpp
  src/query/QueryAPI.cpp
  src/query/SDFExpander.cpp
  src/sampling/BlockClassification.cpp
  src/sampling/ContactBandBlockSampler.cpp
  src/sampling/ContactBandDiagnostics.cpp
  src/sampling/ContactBandMarker.cpp
  src/sampling/ContactBandQualityAudit.cpp
  src/sampling/ContactBandReportWriter.cpp
  src/sampling/ContactBandSamplingPolicy.cpp
  src/sampling/ExactBlockSamplerKernel.cpp
  src/sampling/HierarchicalSamplingDiagnostics.cpp
  src/sampling/HierarchicalSamplingPolicy.cpp
  src/sampling/HierarchicalSDFPredictor.cpp
  src/sampling/LocalFallbackMask.cpp
  src/sampling/SamplingQualityGuard.cpp
  src/sampling/HierarchicalBlockSampler.cpp
  src/sampling/SpeedQualityBenchmark.cpp
  src/sampling/HierarchicalSamplingReportWriter.cpp
  src/sparse/CollisionSampleSet.cpp
  src/sparse/CollisionSampleSetIO.cpp
  src/sparse/SparseSDFQuery.cpp
  src/sparse/SparseCollisionQuery.cpp
  src/sparse/ContactCandidateReducer.cpp
  src/sparse/SparseQueryReportWriter.cpp
  src/world/WorldTransform.cpp
  src/world/CollisionGroupMask.cpp
  src/world/WorldObject.cpp
  src/world/CollisionWorld.cpp
  src/world/AABBBroadphase.cpp
  src/world/WorldSceneIO.cpp
  src/world/WorldSparseCollision.cpp
  src/world/WorldSolverContacts.cpp
  src/world/WorldReportWriter.cpp
  src/report/RunManifest.cpp
  src/report/CaseManifest.cpp
  src/report/StrictReportSchema.cpp
  src/report/StrictJsonWriter.cpp
  src/report/StrictCsvWriter.cpp
  src/report/ReportValidator.cpp
  src/report/ReportCollector.cpp
  src/runtime/ActiveBlockSelector.cpp
  src/runtime/ExpandedBlock.cpp
  src/runtime/ExpandedBlockCache.cpp
  src/runtime/BlockExpansionManager.cpp
  src/runtime/ActiveBlockQuery.cpp
  src/runtime/ActiveBlockReportWriter.cpp
  src/runtime/cuda/CudaActiveBlockBuffer.cpp
  src/runtime/cuda/CudaActiveBlockCache.cpp
  src/runtime/cuda/CudaActiveBlockQuery.cpp
  src/runtime/cuda/CudaActiveBlockBenchmark.cpp
  src/recommendation/BuildRecommendationWriter.cpp
  src/recommendation/BuildRecommender.cpp
  src/recommendation/BuildSurrogateEstimator.cpp
  src/recommendation/MeshFeatureExtractor.cpp
  src/recommendation/SurrogateProfile.cpp
  src/visualization/CollisionSVGWriter.cpp)

if(ADASDF_CL_HAS_CUDA_BACKEND)
  list(APPEND ADASDF_CL_RUNTIME_SOURCES
    src/backend/CudaQueryKernels.cu
    src/runtime/cuda/CudaActiveBlockKernels.cu)
endif()

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
message(STATUS "  Benchmarks: ${ADASDF_CL_BUILD_BENCHMARKS}")
message(STATUS "  Existing core requested: ${ADASDF_CL_USE_EXISTING_CORE}")
message(STATUS "  Existing core found: ${ADASDF_CL_HAS_EXISTING_CORE}")
message(STATUS "  Adaptive builder: ${ADASDF_CL_ENABLE_ADAPTIVE_BUILDER}")
message(STATUS "  Surrogate recommender: ${ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER}")
message(STATUS "  Demo backend: ${ADASDF_CL_ENABLE_DEMO_BACKEND}")
message(STATUS "  Demo surrogate: ${ADASDF_CL_ENABLE_DEMO_SURROGATE}")
message(STATUS "  Collision SVG viewer: ${ADASDF_CL_ENABLE_COLLISION_VIEWER}")
message(STATUS "  CUDA backend: ${ADASDF_CL_HAS_CUDA_BACKEND}")
message(STATUS "  CUDA toolkit: ${ADASDF_CL_CUDA_TOOLKIT_STATUS}")
message(STATUS "  FCL: not required")
message(STATUS "")

add_library(adasdf_cl_runtime STATIC ${ADASDF_CL_RUNTIME_SOURCES})
add_library(adasdf::cl_runtime ALIAS adasdf_cl_runtime)
target_compile_features(adasdf_cl_runtime PUBLIC cxx_std_17)
target_link_libraries(adasdf_cl_runtime PUBLIC adasdf_cl)
target_compile_definitions(adasdf_cl_runtime PUBLIC
  ADASDF_CL_HAS_EXISTING_CORE=$<BOOL:${ADASDF_CL_HAS_EXISTING_CORE}>
  ADASDF_CL_ENABLE_ADAPTIVE_BUILDER=$<BOOL:${ADASDF_CL_ENABLE_ADAPTIVE_BUILDER}>
  ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=$<BOOL:${ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER}>
  ADASDF_CL_ENABLE_DEMO_BACKEND=$<BOOL:${ADASDF_CL_ENABLE_DEMO_BACKEND}>
  ADASDF_CL_ENABLE_DEMO_SURROGATE=$<BOOL:${ADASDF_CL_ENABLE_DEMO_SURROGATE}>
  ADASDF_CL_ENABLE_COLLISION_VIEWER=$<BOOL:${ADASDF_CL_ENABLE_COLLISION_VIEWER}>
  ADASDF_CL_HAS_CUDA_BACKEND=$<BOOL:${ADASDF_CL_HAS_CUDA_BACKEND}>)

if(ADASDF_CL_HAS_EXISTING_CORE)
  target_link_libraries(adasdf_cl_runtime PRIVATE adasdf_existing_core)
endif()

if(ADASDF_CL_HAS_CUDA_BACKEND)
  target_link_libraries(adasdf_cl_runtime PUBLIC CUDA::cudart)
  set_property(TARGET adasdf_cl_runtime PROPERTY CUDA_STANDARD 17)
  set_property(TARGET adasdf_cl_runtime PROPERTY CUDA_STANDARD_REQUIRED ON)
endif()

set(ADASDF_CL_EXAMPLE_SOURCES
  examples/01_build_adaptive_sdf.cpp
  examples/02_load_sdfbin_and_query.cpp
  examples/03_collision_between_two_objects.cpp
  examples/04_batched_gpu_query.cpp
  examples/05_fcl_style_api.cpp
  examples/07_contact_reduction_demo.cpp
  examples/08_core_free_demo_collision.cpp
  examples/09_surrogate_adaptive_demo.cpp
  examples/10_two_box_collision_with_view.cpp
  examples/11_capability_walkthrough.cpp
  examples/12_mesh_diagnostics_demo.cpp
  examples/13_mesh_cleanup_demo.cpp
  examples/14_stl_to_dense_sdf_demo.cpp
  examples/15_adaptive_builder_preview_demo.cpp
  examples/16_stl_to_adaptive_block_sdf_demo.cpp
  examples/17_adaptive_vs_dense_sdf_demo.cpp
  examples/18_adaptive_block_compression_demo.cpp
  examples/19_dense_adaptive_compressed_comparison_demo.cpp
  examples/20_build_recommendation_demo.cpp)

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

  add_executable(adasdf_core_free_demo_collision
    examples/08_core_free_demo_collision.cpp)
  target_link_libraries(adasdf_core_free_demo_collision PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_surrogate_adaptive_demo
    examples/09_surrogate_adaptive_demo.cpp)
  target_link_libraries(adasdf_surrogate_adaptive_demo PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_two_box_collision_with_view
    examples/10_two_box_collision_with_view.cpp)
  target_link_libraries(adasdf_two_box_collision_with_view PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_capability_walkthrough
    examples/11_capability_walkthrough.cpp)
  target_link_libraries(adasdf_capability_walkthrough PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_mesh_diagnostics_demo
    examples/12_mesh_diagnostics_demo.cpp)
  target_link_libraries(adasdf_mesh_diagnostics_demo PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_mesh_cleanup_demo
    examples/13_mesh_cleanup_demo.cpp)
  target_link_libraries(adasdf_mesh_cleanup_demo PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_stl_to_dense_sdf_demo
    examples/14_stl_to_dense_sdf_demo.cpp)
  target_link_libraries(adasdf_stl_to_dense_sdf_demo PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_adaptive_builder_preview_demo
    examples/15_adaptive_builder_preview_demo.cpp)
  target_link_libraries(adasdf_adaptive_builder_preview_demo PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_stl_to_adaptive_block_sdf_demo
    examples/16_stl_to_adaptive_block_sdf_demo.cpp)
  target_link_libraries(adasdf_stl_to_adaptive_block_sdf_demo PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_adaptive_vs_dense_sdf_demo
    examples/17_adaptive_vs_dense_sdf_demo.cpp)
  target_link_libraries(adasdf_adaptive_vs_dense_sdf_demo PRIVATE adasdf::cl_runtime)

  add_executable(adasdf_adaptive_block_compression_demo
    examples/18_adaptive_block_compression_demo.cpp)
  target_link_libraries(adasdf_adaptive_block_compression_demo PRIVATE adasdf::cl_runtime)
  target_compile_definitions(adasdf_adaptive_block_compression_demo PRIVATE
    ADASDF_CL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

  add_executable(adasdf_compressed_compare_demo
    examples/19_dense_adaptive_compressed_comparison_demo.cpp)
  target_link_libraries(adasdf_compressed_compare_demo PRIVATE adasdf::cl_runtime)
  set_target_properties(adasdf_compressed_compare_demo PROPERTIES
    OUTPUT_NAME adasdf_dense_adaptive_compressed_comparison_demo)
  target_compile_definitions(adasdf_compressed_compare_demo PRIVATE
    ADASDF_CL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

  add_executable(adasdf_build_recommend_demo
    examples/20_build_recommendation_demo.cpp)
  target_link_libraries(adasdf_build_recommend_demo PRIVATE adasdf::cl_runtime)

  add_custom_target(adasdf_cl_api_preview_examples SOURCES
    examples/01_build_adaptive_sdf.cpp
    examples/04_batched_gpu_query.cpp)
endif()

add_executable(adasdf_build tools/adasdf_build.cpp)
target_link_libraries(adasdf_build PRIVATE adasdf_cl_runtime)

add_executable(adasdf_info tools/adasdf_info.cpp)
target_link_libraries(adasdf_info PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_info PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_query tools/adasdf_query.cpp)
target_link_libraries(adasdf_query PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_query PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_collide tools/adasdf_collide.cpp)
target_link_libraries(adasdf_collide PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_collide PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_make_demo_box tools/adasdf_make_demo_box.cpp)
target_link_libraries(adasdf_make_demo_box PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_make_demo_box PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_recommend_demo tools/adasdf_recommend_demo.cpp)
target_link_libraries(adasdf_recommend_demo PRIVATE adasdf_cl_runtime)

add_executable(adasdf_recommend_build tools/adasdf_recommend_build.cpp)
target_link_libraries(adasdf_recommend_build PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_recommend_build PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_build_demo_adaptive tools/adasdf_build_demo_adaptive.cpp)
target_link_libraries(adasdf_build_demo_adaptive PRIVATE adasdf_cl_runtime)

add_executable(adasdf_collide_boxes_demo tools/adasdf_collide_boxes_demo.cpp)
target_link_libraries(adasdf_collide_boxes_demo PRIVATE adasdf_cl_runtime)

add_executable(adasdf_query_mode_demo tools/adasdf_query_mode_demo.cpp)
target_link_libraries(adasdf_query_mode_demo PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_query_mode_demo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_expansion_quality tools/adasdf_expansion_quality.cpp)
target_link_libraries(adasdf_expansion_quality PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_expansion_quality PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_capabilities tools/adasdf_capabilities.cpp)
target_link_libraries(adasdf_capabilities PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_capabilities PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_write_manifest tools/adasdf_write_manifest.cpp)
target_link_libraries(adasdf_write_manifest PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_write_manifest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_validate_report tools/adasdf_validate_report.cpp)
target_link_libraries(adasdf_validate_report PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_validate_report PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_collect_run_summary
  tools/adasdf_collect_run_summary.cpp)
target_link_libraries(adasdf_collect_run_summary PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_collect_run_summary PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_mesh_check tools/adasdf_mesh_check.cpp)
target_link_libraries(adasdf_mesh_check PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_mesh_check PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_mesh_clean tools/adasdf_mesh_clean.cpp)
target_link_libraries(adasdf_mesh_clean PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_mesh_clean PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_build_dense_sdf tools/adasdf_build_dense_sdf.cpp)
target_link_libraries(adasdf_build_dense_sdf PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_build_dense_sdf PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_build_adaptive_sdf tools/adasdf_build_adaptive_sdf.cpp)
target_link_libraries(adasdf_build_adaptive_sdf PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_build_adaptive_sdf PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_compress_adaptive_sdf tools/adasdf_compress_adaptive_sdf.cpp)
target_link_libraries(adasdf_compress_adaptive_sdf PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_compress_adaptive_sdf PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_build_compressed_sdf tools/adasdf_build_compressed_sdf.cpp)
target_link_libraries(adasdf_build_compressed_sdf PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_build_compressed_sdf PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_build_adaptive_sdf_preview
  tools/adasdf_build_adaptive_sdf_preview.cpp)
target_link_libraries(adasdf_build_adaptive_sdf_preview PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_build_adaptive_sdf_preview PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_sparse_query tools/adasdf_sparse_query.cpp)
target_link_libraries(adasdf_sparse_query PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_sparse_query PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_sparse_collide tools/adasdf_sparse_collide.cpp)
target_link_libraries(adasdf_sparse_collide PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_sparse_collide PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_contact_candidates tools/adasdf_contact_candidates.cpp)
target_link_libraries(adasdf_contact_candidates PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_contact_candidates PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_stabilize_contacts tools/adasdf_stabilize_contacts.cpp)
target_link_libraries(adasdf_stabilize_contacts PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_stabilize_contacts PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_solver_contact_candidates
  tools/adasdf_solver_contact_candidates.cpp)
target_link_libraries(adasdf_solver_contact_candidates PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_solver_contact_candidates PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_world_broadphase tools/adasdf_world_broadphase.cpp)
target_link_libraries(adasdf_world_broadphase PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_world_broadphase PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_world_sparse_collide
  tools/adasdf_world_sparse_collide.cpp)
target_link_libraries(adasdf_world_sparse_collide PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_world_sparse_collide PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_world_solver_contacts
  tools/adasdf_world_solver_contacts.cpp)
target_link_libraries(adasdf_world_solver_contacts PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_world_solver_contacts PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_select_active_blocks tools/adasdf_select_active_blocks.cpp)
target_link_libraries(adasdf_select_active_blocks PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_select_active_blocks PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_active_block_query tools/adasdf_active_block_query.cpp)
target_link_libraries(adasdf_active_block_query PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_active_block_query PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

add_executable(adasdf_cuda_active_block_query
  tools/adasdf_cuda_active_block_query.cpp)
target_link_libraries(adasdf_cuda_active_block_query PRIVATE adasdf_cl_runtime)
set_target_properties(adasdf_cuda_active_block_query PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

if(ADASDF_CL_BUILD_BENCHMARKS)
  add_executable(adasdf_benchmark_batch_query
    benchmarks/adasdf_benchmark_batch_query.cpp)
  target_link_libraries(adasdf_benchmark_batch_query PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_batch_query PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/benchmarks")

  add_executable(adasdf_benchmark_sparse_query
    tools/adasdf_benchmark_sparse_query.cpp)
  target_link_libraries(adasdf_benchmark_sparse_query PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_sparse_query PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_block_cache
    tools/adasdf_benchmark_block_cache.cpp)
  target_link_libraries(adasdf_benchmark_block_cache PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_block_cache PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_builder_acceleration
    tools/adasdf_benchmark_builder_acceleration.cpp)
  target_link_libraries(adasdf_benchmark_builder_acceleration PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_builder_acceleration PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_contact_reduction
    tools/adasdf_benchmark_contact_reduction.cpp)
  target_link_libraries(adasdf_benchmark_contact_reduction PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_contact_reduction PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_collision_world
    tools/adasdf_benchmark_collision_world.cpp)
  target_link_libraries(adasdf_benchmark_collision_world PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_collision_world PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_cuda_block_cache
    tools/adasdf_benchmark_cuda_block_cache.cpp)
  target_link_libraries(adasdf_benchmark_cuda_block_cache PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_cuda_block_cache PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_hierarchical_sampling
    tools/adasdf_benchmark_hierarchical_sampling.cpp)
  target_link_libraries(adasdf_benchmark_hierarchical_sampling PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_hierarchical_sampling PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_exact_hotpath
    tools/adasdf_benchmark_exact_hotpath.cpp)
  target_link_libraries(adasdf_benchmark_exact_hotpath PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_exact_hotpath PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_benchmark_contact_band_sampling
    tools/adasdf_benchmark_contact_band_sampling.cpp)
  target_link_libraries(adasdf_benchmark_contact_band_sampling PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_benchmark_contact_band_sampling PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_sweep_contact_band_sampling
    tools/adasdf_sweep_contact_band_sampling.cpp)
  target_link_libraries(adasdf_sweep_contact_band_sampling PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_sweep_contact_band_sampling PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

  add_executable(adasdf_sweep_hierarchical_sampling
    tools/adasdf_sweep_hierarchical_sampling.cpp)
  target_link_libraries(adasdf_sweep_hierarchical_sampling PRIVATE adasdf_cl_runtime)
  set_target_properties(adasdf_sweep_hierarchical_sampling PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")
endif()

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
  tests/test_analytic_sdf_model.cpp
  tests/test_demo_sdfbin.cpp
  tests/test_core_free_demo_collision.cpp
  tests/test_make_demo_box_cli.cpp
  tests/test_demo_surrogate_recommender.cpp
  tests/test_demo_adaptive_sdf_model.cpp
  tests/test_demo_adaptive_sdfbin.cpp
  tests/test_demo_adaptive_builder.cpp
  tests/test_collide_boxes_demo.cpp
  tests/test_collision_svg_writer.cpp
  tests/test_batch_query_cpu.cpp
  tests/test_cuda_query_backend.cpp
  tests/test_cpu_gpu_alignment.cpp
  tests/test_block_selection.cpp
  tests/test_query_mode_config.cpp
  tests/test_sdf_expander.cpp
  tests/test_expansion_quality.cpp
  tests/test_sign_mismatch_metrics.cpp
  tests/test_expansion_resolution_control.cpp
  tests/test_existing_core_expansion_bridge.cpp
  tests/test_expansion_quality_cli.cpp
  tests/test_query_engine_cpu.cpp
  tests/test_query_engine_cuda.cpp
  tests/test_query_mode_cli.cpp
  tests/test_benchmark_timing_fields.cpp
  tests/test_benchmark_cli_modes.cpp
  tests/test_benchmark_output_modes.cpp
  tests/test_cuda_device_only_benchmark.cpp
  tests/test_benchmark_workspace_reuse.cpp
  tests/test_cuda_query_workspace.cpp
  tests/test_cuda_phi_only_kernel.cpp
  tests/test_cuda_output_modes.cpp
  tests/test_cuda_block_lookup_fastpath.cpp
  tests/test_point_cloud_generator.cpp
  tests/test_adaptive_builder.cpp
  tests/test_sdfbin_writer_roundtrip.cpp
  tests/test_surrogate_recommender.cpp
  tests/test_contact_only_sdfbin.cpp
  tests/test_sdfbin_real_load.cpp
  tests/test_sdfmodel_query.cpp
  tests/test_capabilities_cli.cpp
  tests/test_capability_walkthrough_example.cpp
  tests/test_docs_capability_links.cpp
  tests/test_stl_reader.cpp
  tests/test_mesh_diagnostics.cpp
  tests/test_mesh_diagnostics_writer.cpp
  tests/test_mesh_readiness.cpp
  tests/test_mesh_check_cli.cpp
  tests/test_mesh_check_readiness_cli.cpp
  tests/test_mesh_cleanup.cpp
  tests/test_stl_writer.cpp
  tests/test_mesh_clean_cli.cpp
  tests/test_mesh_cleanup_roundtrip.cpp
  tests/test_triangle_distance.cpp
  tests/test_triangle_aabb.cpp
  tests/test_triangle_bvh_builder.cpp
  tests/test_bvh_nearest_triangle_query.cpp
  tests/test_bvh_ray_intersection_query.cpp
  tests/test_bvh_sdf_sampler.cpp
  tests/test_parallel_sampling.cpp
  tests/test_mesh_sign.cpp
  tests/test_dense_sdf_builder.cpp
  tests/test_dense_sdf_builder_bvh.cpp
  tests/test_dense_sdfbin_roundtrip.cpp
  tests/test_build_dense_sdf_cli.cpp
  tests/test_dense_sdf_collision.cpp
  tests/test_adaptive_builder_preview.cpp
  tests/test_build_adaptive_sdf_preview_cli.cpp
  tests/test_adaptive_octree_builder.cpp
  tests/test_adaptive_block_partitioner.cpp
  tests/test_adaptive_block_sdf_builder.cpp
  tests/test_adaptive_block_sdf_builder_bvh.cpp
  tests/test_block_classification.cpp
  tests/test_hierarchical_sampling_policy.cpp
  tests/test_hierarchical_sdf_predictor.cpp
  tests/test_sampling_quality_guard.cpp
  tests/test_hierarchical_block_sampler.cpp
  tests/test_adaptive_builder_hierarchical_sampling.cpp
  tests/test_adaptive_block_sdf_model.cpp
  tests/test_adaptive_block_sdfbin_roundtrip.cpp
  tests/test_build_adaptive_sdf_cli.cpp
  tests/test_adaptive_sdf_collision.cpp
  tests/test_adaptive_vs_dense_consistency.cpp
  tests/test_adaptive_builder_preview_updated.cpp
  tests/test_small_matrix_svd.cpp
  tests/test_compressed_sdf_block.cpp
  tests/test_block_low_rank_compressor.cpp
  tests/test_compressed_adaptive_block_sdf_model.cpp
  tests/test_compressed_block_sdfbin_roundtrip.cpp
  tests/test_compression_quality.cpp
  tests/test_compress_adaptive_sdf_cli.cpp
  tests/test_build_compressed_sdf_cli.cpp
  tests/test_compressed_sdf_builder_bvh_cli.cpp
  tests/test_compressed_builder_hierarchical_sampling_cli.cpp
  tests/test_benchmark_hierarchical_sampling_cli.cpp
  tests/test_hierarchical_sampling_diagnostics.cpp
  tests/test_hierarchical_sampling_far_field_quality_modes.cpp
  tests/test_hierarchical_sampling_coarse_sample_reuse.cpp
  tests/test_exact_block_sampler_kernel.cpp
  tests/test_hierarchical_exact_hotpath.cpp
  tests/test_near_surface_banded_prediction.cpp
  tests/test_local_fallback_mask.cpp
  tests/test_far_field_sign_policy.cpp
  tests/test_contact_band_marker.cpp
  tests/test_box_triangle_distance.cpp
  tests/test_contact_band_marker_distance_aware.cpp
  tests/test_contact_band_marker_local_halo.cpp
  tests/test_contact_band_sampling_policy.cpp
  tests/test_contact_band_block_sampler.cpp
  tests/test_contact_band_quality_audit.cpp
  tests/test_contact_band_benchmark_cli.cpp
  tests/test_contact_band_marker_sweep_cli.cpp
  tests/test_sweep_hierarchical_sampling_cli.cpp
  tests/test_synthetic_complex_fixture.cpp
  tests/test_hierarchical_vs_exact_quality.cpp
  tests/test_compressed_sdf_collision.cpp
  tests/test_compressed_vs_adaptive_consistency.cpp
  tests/test_bvh_vs_bruteforce_consistency.cpp
  tests/test_benchmark_builder_acceleration_cli.cpp
  tests/test_adaptive_builder_preview_v1_7.cpp
  tests/test_mesh_diagnostics_fixtures.cpp
  tests/test_mesh_feature_extractor.cpp
  tests/test_build_surrogate_estimator.cpp
  tests/test_surrogate_profile.cpp
  tests/test_build_recommender.cpp
  tests/test_build_recommendation_writer.cpp
  tests/test_recommend_build_cli.cpp
  tests/test_recommendation_workflow.cpp
  tests/test_adaptive_builder_preview_v1_8.cpp
  tests/test_collision_sample_set_io.cpp
  tests/test_sparse_sdf_query.cpp
  tests/test_sparse_collision_query.cpp
  tests/test_contact_candidate_reducer.cpp
  tests/test_contact_budget.cpp
  tests/test_contact_patch.cpp
  tests/test_contact_clusterer.cpp
  tests/test_contact_stabilizer.cpp
  tests/test_solver_contact_builder.cpp
  tests/test_solver_contact_exporter.cpp
  tests/test_contact_stabilization_report_writer.cpp
  tests/test_solver_contact_budget_behavior.cpp
  tests/test_contact_determinism.cpp
  tests/test_sparse_query_report_writer.cpp
  tests/test_sparse_query_cli.cpp
  tests/test_sparse_collide_cli.cpp
  tests/test_contact_candidates_cli.cpp
  tests/test_stabilize_contacts_cli.cpp
  tests/test_solver_contact_candidates_cli.cpp
  tests/test_benchmark_contact_reduction_cli.cpp
  tests/test_benchmark_sparse_query_cli.cpp
  tests/test_sparse_collision_modes.cpp
  tests/test_world_transform.cpp
  tests/test_collision_group_mask.cpp
  tests/test_collision_world.cpp
  tests/test_aabb_broadphase.cpp
  tests/test_world_scene_io.cpp
  tests/test_world_sparse_collision.cpp
  tests/test_world_solver_contacts.cpp
  tests/test_world_report_writer.cpp
  tests/test_world_broadphase_cli.cpp
  tests/test_world_sparse_collide_cli.cpp
  tests/test_world_solver_contacts_cli.cpp
  tests/test_benchmark_collision_world_cli.cpp
  tests/test_strict_json_writer.cpp
  tests/test_strict_csv_writer.cpp
  tests/test_run_manifest.cpp
  tests/test_case_manifest.cpp
  tests/test_report_validator.cpp
  tests/test_report_collector.cpp
  tests/test_write_manifest_cli.cpp
  tests/test_validate_report_cli.cpp
  tests/test_collect_run_summary_cli.cpp
  tests/test_strict_json_core_cli.cpp
  tests/test_active_block_selector.cpp
  tests/test_expanded_block.cpp
  tests/test_expanded_block_cache.cpp
  tests/test_block_expansion_manager.cpp
  tests/test_active_block_query.cpp
  tests/test_active_block_report_writer.cpp
  tests/test_select_active_blocks_cli.cpp
  tests/test_active_block_query_cli.cpp
  tests/test_benchmark_block_cache_cli.cpp
  tests/test_active_block_cache_vs_direct.cpp
  tests/test_cuda_active_block_buffer.cpp
  tests/test_cuda_active_block_cache.cpp
  tests/test_cuda_active_block_query_unavailable.cpp
  tests/test_cuda_active_block_query.cpp
  tests/test_cuda_active_block_alignment.cpp
  tests/test_cuda_active_block_phi_normal.cpp
  tests/test_cuda_active_block_cli.cpp
  tests/test_benchmark_cuda_block_cache_cli.cpp
  tests/test_cuda_active_block_python_contract.cpp)

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
  set(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/mesh_diagnostics")
  set(ADASDF_CL_TEST_SAMPLES_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/samples")
  set(ADASDF_CL_TEST_TEMP_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/adasdf_cl_test_tmp")
  foreach(test_source IN LISTS ADASDF_CL_TEST_SOURCES)
    get_filename_component(test_name "${test_source}" NAME_WE)
    add_executable(${test_name} "${test_source}")
    target_link_libraries(${test_name} PRIVATE adasdf::cl_runtime)
    target_compile_definitions(${test_name} PRIVATE
      ADASDF_CL_TEST_FIXTURE="${ADASDF_CL_TEST_FIXTURE}"
      ADASDF_CL_TEST_CUBE_STL="${ADASDF_CL_TEST_CUBE_STL}"
      ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR="${ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR}"
      ADASDF_CL_TEST_SAMPLES_DIR="${ADASDF_CL_TEST_SAMPLES_DIR}"
      ADASDF_CL_TEST_TEMP_DIR="${ADASDF_CL_TEST_TEMP_DIR}"
      ADASDF_CL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(test_name STREQUAL "test_make_demo_box_cli")
      add_dependencies(${test_name} adasdf_make_demo_box)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_MAKE_DEMO_BOX="$<TARGET_FILE_NAME:adasdf_make_demo_box>")
    endif()
    if(test_name STREQUAL "test_query_mode_cli")
      add_dependencies(${test_name} adasdf_query_mode_demo)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_QUERY_MODE_DEMO="$<TARGET_FILE_NAME:adasdf_query_mode_demo>")
    endif()
    if(test_name STREQUAL "test_expansion_quality_cli")
      add_dependencies(${test_name} adasdf_expansion_quality)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_EXPANSION_QUALITY="$<TARGET_FILE_NAME:adasdf_expansion_quality>")
    endif()
    if(test_name STREQUAL "test_capabilities_cli")
      add_dependencies(${test_name} adasdf_capabilities)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_CAPABILITIES="$<TARGET_FILE_NAME:adasdf_capabilities>")
    endif()
    if(test_name STREQUAL "test_write_manifest_cli" OR
        test_name STREQUAL "test_validate_report_cli" OR
        test_name STREQUAL "test_collect_run_summary_cli" OR
        test_name STREQUAL "test_strict_json_core_cli")
      add_dependencies(${test_name}
        adasdf_write_manifest
        adasdf_validate_report
        adasdf_collect_run_summary
        adasdf_build_compressed_sdf)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_WRITE_MANIFEST="$<TARGET_FILE_NAME:adasdf_write_manifest>"
        ADASDF_CL_VALIDATE_REPORT="$<TARGET_FILE_NAME:adasdf_validate_report>"
        ADASDF_CL_COLLECT_RUN_SUMMARY="$<TARGET_FILE_NAME:adasdf_collect_run_summary>"
        ADASDF_CL_BUILD_COMPRESSED_SDF="$<TARGET_FILE_NAME:adasdf_build_compressed_sdf>")
    endif()
    if(test_name STREQUAL "test_mesh_check_cli" OR
        test_name STREQUAL "test_mesh_check_readiness_cli")
      add_dependencies(${test_name} adasdf_mesh_check)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_MESH_CHECK="$<TARGET_FILE_NAME:adasdf_mesh_check>")
    endif()
    if(test_name STREQUAL "test_mesh_clean_cli" OR
        test_name STREQUAL "test_mesh_cleanup_roundtrip")
      add_dependencies(${test_name} adasdf_mesh_clean adasdf_mesh_check)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_MESH_CLEAN="$<TARGET_FILE_NAME:adasdf_mesh_clean>"
        ADASDF_CL_MESH_CHECK="$<TARGET_FILE_NAME:adasdf_mesh_check>")
    endif()
    if(test_name STREQUAL "test_build_dense_sdf_cli" OR
        test_name STREQUAL "test_dense_sdfbin_roundtrip" OR
        test_name STREQUAL "test_dense_sdf_collision")
      add_dependencies(${test_name}
        adasdf_build_dense_sdf
        adasdf_info
        adasdf_query
        adasdf_collide)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>"
        ADASDF_CL_INFO="$<TARGET_FILE_NAME:adasdf_info>"
        ADASDF_CL_QUERY="$<TARGET_FILE_NAME:adasdf_query>"
        ADASDF_CL_COLLIDE="$<TARGET_FILE_NAME:adasdf_collide>")
    endif()
    if(test_name STREQUAL "test_recommend_build_cli")
      add_dependencies(${test_name} adasdf_recommend_build adasdf_build_dense_sdf)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_RECOMMEND_BUILD="$<TARGET_FILE_NAME:adasdf_recommend_build>"
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>")
    endif()
    if(test_name STREQUAL "test_build_adaptive_sdf_cli" OR
        test_name STREQUAL "test_adaptive_block_sdfbin_roundtrip" OR
        test_name STREQUAL "test_adaptive_sdf_collision" OR
        test_name STREQUAL "test_adaptive_builder_hierarchical_sampling" OR
        test_name STREQUAL "test_compress_adaptive_sdf_cli")
      add_dependencies(${test_name}
        adasdf_build_adaptive_sdf
        adasdf_compress_adaptive_sdf
        adasdf_info
        adasdf_query
        adasdf_collide)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_ADAPTIVE_SDF="$<TARGET_FILE_NAME:adasdf_build_adaptive_sdf>"
        ADASDF_CL_COMPRESS_ADAPTIVE_SDF="$<TARGET_FILE_NAME:adasdf_compress_adaptive_sdf>"
        ADASDF_CL_INFO="$<TARGET_FILE_NAME:adasdf_info>"
        ADASDF_CL_QUERY="$<TARGET_FILE_NAME:adasdf_query>"
        ADASDF_CL_COLLIDE="$<TARGET_FILE_NAME:adasdf_collide>")
    endif()
    if(test_name STREQUAL "test_build_compressed_sdf_cli" OR
        test_name STREQUAL "test_compressed_sdf_builder_bvh_cli" OR
        test_name STREQUAL "test_compressed_builder_hierarchical_sampling_cli" OR
        test_name STREQUAL "test_compressed_block_sdfbin_roundtrip" OR
        test_name STREQUAL "test_compressed_sdf_collision")
      add_dependencies(${test_name}
        adasdf_build_compressed_sdf
        adasdf_info
        adasdf_query
        adasdf_collide)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_COMPRESSED_SDF="$<TARGET_FILE_NAME:adasdf_build_compressed_sdf>"
        ADASDF_CL_INFO="$<TARGET_FILE_NAME:adasdf_info>"
        ADASDF_CL_QUERY="$<TARGET_FILE_NAME:adasdf_query>"
        ADASDF_CL_COLLIDE="$<TARGET_FILE_NAME:adasdf_collide>")
    endif()
    if(test_name STREQUAL "test_build_adaptive_sdf_preview_cli" OR
        test_name STREQUAL "test_adaptive_builder_preview_updated" OR
        test_name STREQUAL "test_adaptive_builder_preview_v1_7" OR
        test_name STREQUAL "test_adaptive_builder_preview_v1_8")
      add_dependencies(${test_name} adasdf_build_adaptive_sdf_preview)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_ADAPTIVE_PREVIEW="$<TARGET_FILE_NAME:adasdf_build_adaptive_sdf_preview>")
    endif()
    if(test_name STREQUAL "test_capability_walkthrough_example" AND
        TARGET adasdf_capability_walkthrough)
      add_dependencies(${test_name} adasdf_capability_walkthrough)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_CAPABILITY_WALKTHROUGH="$<TARGET_FILE_NAME:adasdf_capability_walkthrough>")
    endif()
    if(test_name STREQUAL "test_benchmark_timing_fields" OR
        test_name STREQUAL "test_benchmark_cli_modes" OR
        test_name STREQUAL "test_benchmark_output_modes" OR
        test_name STREQUAL "test_cuda_device_only_benchmark" OR
        test_name STREQUAL "test_benchmark_workspace_reuse")
      if(TARGET adasdf_benchmark_batch_query)
        add_dependencies(${test_name} adasdf_benchmark_batch_query)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_BATCH_QUERY="$<TARGET_FILE_NAME:adasdf_benchmark_batch_query>")
      endif()
    endif()
    if(test_name STREQUAL "test_sparse_query_cli")
      add_dependencies(${test_name} adasdf_build_dense_sdf adasdf_sparse_query)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>"
        ADASDF_CL_SPARSE_QUERY="$<TARGET_FILE_NAME:adasdf_sparse_query>")
    endif()
    if(test_name STREQUAL "test_sparse_collide_cli")
      add_dependencies(${test_name} adasdf_build_dense_sdf adasdf_sparse_collide)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>"
        ADASDF_CL_SPARSE_COLLIDE="$<TARGET_FILE_NAME:adasdf_sparse_collide>")
    endif()
    if(test_name STREQUAL "test_contact_candidates_cli")
      add_dependencies(${test_name} adasdf_build_dense_sdf adasdf_contact_candidates)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>"
        ADASDF_CL_CONTACT_CANDIDATES="$<TARGET_FILE_NAME:adasdf_contact_candidates>")
    endif()
    if(test_name STREQUAL "test_stabilize_contacts_cli")
      add_dependencies(${test_name} adasdf_stabilize_contacts)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_STABILIZE_CONTACTS="$<TARGET_FILE_NAME:adasdf_stabilize_contacts>")
    endif()
    if(test_name STREQUAL "test_solver_contact_candidates_cli")
      add_dependencies(${test_name}
        adasdf_build_dense_sdf
        adasdf_solver_contact_candidates)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>"
        ADASDF_CL_SOLVER_CONTACT_CANDIDATES="$<TARGET_FILE_NAME:adasdf_solver_contact_candidates>")
    endif()
    if(test_name STREQUAL "test_world_broadphase_cli" OR
        test_name STREQUAL "test_world_sparse_collide_cli" OR
        test_name STREQUAL "test_world_solver_contacts_cli")
      add_dependencies(${test_name}
        adasdf_make_demo_box
        adasdf_world_broadphase
        adasdf_world_sparse_collide
        adasdf_world_solver_contacts)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_MAKE_DEMO_BOX="$<TARGET_FILE_NAME:adasdf_make_demo_box>"
        ADASDF_CL_WORLD_BROADPHASE="$<TARGET_FILE_NAME:adasdf_world_broadphase>"
        ADASDF_CL_WORLD_SPARSE_COLLIDE="$<TARGET_FILE_NAME:adasdf_world_sparse_collide>"
        ADASDF_CL_WORLD_SOLVER_CONTACTS="$<TARGET_FILE_NAME:adasdf_world_solver_contacts>")
    endif()
    if(test_name STREQUAL "test_benchmark_sparse_query_cli")
      add_dependencies(${test_name} adasdf_build_dense_sdf)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>")
      if(TARGET adasdf_benchmark_sparse_query)
        add_dependencies(${test_name} adasdf_benchmark_sparse_query)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_SPARSE_QUERY="$<TARGET_FILE_NAME:adasdf_benchmark_sparse_query>")
      endif()
    endif()
    if(test_name STREQUAL "test_select_active_blocks_cli" OR
        test_name STREQUAL "test_active_block_query_cli")
      add_dependencies(${test_name}
        adasdf_build_compressed_sdf
        adasdf_select_active_blocks
        adasdf_active_block_query)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_COMPRESSED_SDF="$<TARGET_FILE_NAME:adasdf_build_compressed_sdf>"
        ADASDF_CL_SELECT_ACTIVE_BLOCKS="$<TARGET_FILE_NAME:adasdf_select_active_blocks>"
        ADASDF_CL_ACTIVE_BLOCK_QUERY="$<TARGET_FILE_NAME:adasdf_active_block_query>")
    endif()
    if(test_name STREQUAL "test_cuda_active_block_query_unavailable" OR
        test_name STREQUAL "test_cuda_active_block_cli")
      add_dependencies(${test_name}
        adasdf_build_compressed_sdf
        adasdf_cuda_active_block_query)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_COMPRESSED_SDF="$<TARGET_FILE_NAME:adasdf_build_compressed_sdf>"
        ADASDF_CL_CUDA_ACTIVE_BLOCK_QUERY="$<TARGET_FILE_NAME:adasdf_cuda_active_block_query>")
    endif()
    if(test_name STREQUAL "test_benchmark_block_cache_cli")
      add_dependencies(${test_name} adasdf_build_compressed_sdf)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_COMPRESSED_SDF="$<TARGET_FILE_NAME:adasdf_build_compressed_sdf>")
      if(TARGET adasdf_benchmark_block_cache)
        add_dependencies(${test_name} adasdf_benchmark_block_cache)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_BLOCK_CACHE="$<TARGET_FILE_NAME:adasdf_benchmark_block_cache>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_builder_acceleration_cli")
      if(TARGET adasdf_benchmark_builder_acceleration)
        add_dependencies(${test_name} adasdf_benchmark_builder_acceleration)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_BUILDER_ACCELERATION="$<TARGET_FILE_NAME:adasdf_benchmark_builder_acceleration>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_contact_reduction_cli")
      add_dependencies(${test_name} adasdf_build_dense_sdf)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_DENSE_SDF="$<TARGET_FILE_NAME:adasdf_build_dense_sdf>")
      if(TARGET adasdf_benchmark_contact_reduction)
        add_dependencies(${test_name} adasdf_benchmark_contact_reduction)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_CONTACT_REDUCTION="$<TARGET_FILE_NAME:adasdf_benchmark_contact_reduction>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_collision_world_cli")
      add_dependencies(${test_name} adasdf_make_demo_box)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_MAKE_DEMO_BOX="$<TARGET_FILE_NAME:adasdf_make_demo_box>")
      if(TARGET adasdf_benchmark_collision_world)
        add_dependencies(${test_name}
          adasdf_world_broadphase
          adasdf_world_sparse_collide
          adasdf_benchmark_collision_world)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_WORLD_BROADPHASE="$<TARGET_FILE_NAME:adasdf_world_broadphase>"
          ADASDF_CL_BENCHMARK_COLLISION_WORLD="$<TARGET_FILE_NAME:adasdf_benchmark_collision_world>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_cuda_block_cache_cli")
      add_dependencies(${test_name} adasdf_build_compressed_sdf)
      target_compile_definitions(${test_name} PRIVATE
        ADASDF_CL_BUILD_COMPRESSED_SDF="$<TARGET_FILE_NAME:adasdf_build_compressed_sdf>")
      if(TARGET adasdf_benchmark_cuda_block_cache)
        add_dependencies(${test_name} adasdf_benchmark_cuda_block_cache)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_CUDA_BLOCK_CACHE="$<TARGET_FILE_NAME:adasdf_benchmark_cuda_block_cache>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_hierarchical_sampling_cli")
      if(TARGET adasdf_benchmark_hierarchical_sampling)
        add_dependencies(${test_name} adasdf_benchmark_hierarchical_sampling)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_HIERARCHICAL_SAMPLING="$<TARGET_FILE_NAME:adasdf_benchmark_hierarchical_sampling>")
      endif()
    endif()
    if(test_name STREQUAL "test_contact_band_benchmark_cli")
      if(TARGET adasdf_benchmark_contact_band_sampling)
        add_dependencies(${test_name} adasdf_benchmark_contact_band_sampling)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_BENCHMARK_CONTACT_BAND_SAMPLING="$<TARGET_FILE_NAME:adasdf_benchmark_contact_band_sampling>")
      endif()
    endif()
    if(test_name STREQUAL "test_contact_band_marker_sweep_cli")
      if(TARGET adasdf_sweep_contact_band_sampling)
        add_dependencies(${test_name} adasdf_sweep_contact_band_sampling)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_SWEEP_CONTACT_BAND_SAMPLING="$<TARGET_FILE_NAME:adasdf_sweep_contact_band_sampling>")
      endif()
    endif()
    if(test_name STREQUAL "test_sweep_hierarchical_sampling_cli")
      if(TARGET adasdf_sweep_hierarchical_sampling)
        add_dependencies(${test_name} adasdf_sweep_hierarchical_sampling)
        target_compile_definitions(${test_name} PRIVATE
          ADASDF_CL_SWEEP_HIERARCHICAL_SAMPLING="$<TARGET_FILE_NAME:adasdf_sweep_hierarchical_sampling>")
      endif()
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
    if(test_name STREQUAL "test_make_demo_box_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_make_demo_box>")
    endif()
    if(test_name STREQUAL "test_query_mode_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_query_mode_demo>")
    endif()
    if(test_name STREQUAL "test_expansion_quality_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_expansion_quality>")
    endif()
    if(test_name STREQUAL "test_capabilities_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_capabilities>")
    endif()
    if(test_name STREQUAL "test_write_manifest_cli" OR
        test_name STREQUAL "test_validate_report_cli" OR
        test_name STREQUAL "test_collect_run_summary_cli" OR
        test_name STREQUAL "test_strict_json_core_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_write_manifest>")
    endif()
    if(test_name STREQUAL "test_mesh_check_cli" OR
        test_name STREQUAL "test_mesh_check_readiness_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_mesh_check>")
    endif()
    if(test_name STREQUAL "test_mesh_clean_cli" OR
        test_name STREQUAL "test_mesh_cleanup_roundtrip")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_mesh_clean>")
    endif()
    if(test_name STREQUAL "test_build_dense_sdf_cli" OR
        test_name STREQUAL "test_dense_sdfbin_roundtrip" OR
        test_name STREQUAL "test_dense_sdf_collision")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_build_dense_sdf>")
    endif()
    if(test_name STREQUAL "test_recommend_build_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_recommend_build>")
    endif()
    if(test_name STREQUAL "test_build_adaptive_sdf_cli" OR
        test_name STREQUAL "test_adaptive_block_sdfbin_roundtrip" OR
        test_name STREQUAL "test_adaptive_sdf_collision" OR
        test_name STREQUAL "test_adaptive_builder_hierarchical_sampling" OR
        test_name STREQUAL "test_compress_adaptive_sdf_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_build_adaptive_sdf>")
    endif()
    if(test_name STREQUAL "test_build_compressed_sdf_cli" OR
        test_name STREQUAL "test_compressed_sdf_builder_bvh_cli" OR
        test_name STREQUAL "test_compressed_builder_hierarchical_sampling_cli" OR
        test_name STREQUAL "test_compressed_block_sdfbin_roundtrip" OR
        test_name STREQUAL "test_compressed_sdf_collision")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_build_compressed_sdf>")
    endif()
    if(test_name STREQUAL "test_build_adaptive_sdf_preview_cli" OR
        test_name STREQUAL "test_adaptive_builder_preview_updated" OR
        test_name STREQUAL "test_adaptive_builder_preview_v1_7" OR
        test_name STREQUAL "test_adaptive_builder_preview_v1_8")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_build_adaptive_sdf_preview>")
    endif()
    if(test_name STREQUAL "test_capability_walkthrough_example" AND
        TARGET adasdf_capability_walkthrough)
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_capability_walkthrough>")
    endif()
    if(test_name STREQUAL "test_benchmark_timing_fields" OR
        test_name STREQUAL "test_benchmark_cli_modes" OR
        test_name STREQUAL "test_benchmark_output_modes" OR
        test_name STREQUAL "test_cuda_device_only_benchmark" OR
        test_name STREQUAL "test_benchmark_workspace_reuse")
      if(TARGET adasdf_benchmark_batch_query)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_batch_query>")
      endif()
    endif()
    if(test_name STREQUAL "test_sparse_query_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_sparse_query>")
    endif()
    if(test_name STREQUAL "test_sparse_collide_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_sparse_collide>")
    endif()
    if(test_name STREQUAL "test_contact_candidates_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_contact_candidates>")
    endif()
    if(test_name STREQUAL "test_stabilize_contacts_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_stabilize_contacts>")
    endif()
    if(test_name STREQUAL "test_solver_contact_candidates_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_solver_contact_candidates>")
    endif()
    if(test_name STREQUAL "test_world_broadphase_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_world_broadphase>")
    endif()
    if(test_name STREQUAL "test_world_sparse_collide_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_world_sparse_collide>")
    endif()
    if(test_name STREQUAL "test_world_solver_contacts_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_world_solver_contacts>")
    endif()
    if(test_name STREQUAL "test_benchmark_sparse_query_cli")
      if(TARGET adasdf_benchmark_sparse_query)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_sparse_query>")
      endif()
    endif()
    if(test_name STREQUAL "test_select_active_blocks_cli" OR
        test_name STREQUAL "test_active_block_query_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_select_active_blocks>")
    endif()
    if(test_name STREQUAL "test_cuda_active_block_query_unavailable" OR
        test_name STREQUAL "test_cuda_active_block_cli")
      set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_cuda_active_block_query>")
    endif()
    if(test_name STREQUAL "test_benchmark_block_cache_cli")
      if(TARGET adasdf_benchmark_block_cache)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_block_cache>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_builder_acceleration_cli")
      if(TARGET adasdf_benchmark_builder_acceleration)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_builder_acceleration>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_contact_reduction_cli")
      if(TARGET adasdf_benchmark_contact_reduction)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_contact_reduction>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_collision_world_cli")
      if(TARGET adasdf_benchmark_collision_world)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_collision_world>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_cuda_block_cache_cli")
      if(TARGET adasdf_benchmark_cuda_block_cache)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_cuda_block_cache>")
      endif()
    endif()
    if(test_name STREQUAL "test_benchmark_hierarchical_sampling_cli")
      if(TARGET adasdf_benchmark_hierarchical_sampling)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_hierarchical_sampling>")
      endif()
    endif()
    if(test_name STREQUAL "test_contact_band_benchmark_cli")
      if(TARGET adasdf_benchmark_contact_band_sampling)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_benchmark_contact_band_sampling>")
      endif()
    endif()
    if(test_name STREQUAL "test_contact_band_marker_sweep_cli")
      if(TARGET adasdf_sweep_contact_band_sampling)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_sweep_contact_band_sampling>")
      endif()
    endif()
    if(test_name STREQUAL "test_sweep_hierarchical_sampling_cli")
      if(TARGET adasdf_sweep_hierarchical_sampling)
        set_tests_properties(${test_name} PROPERTIES
          WORKING_DIRECTORY "$<TARGET_FILE_DIR:adasdf_sweep_hierarchical_sampling>")
      endif()
    endif()
  endforeach()
endif()

set_target_properties(adasdf_cl PROPERTIES EXPORT_NAME adasdf_cl_headers)
set_target_properties(adasdf_cl_runtime PROPERTIES EXPORT_NAME adasdf_cl)
set(ADASDF_CL_INSTALL_TARGETS adasdf_cl adasdf_cl_runtime)
if(ADASDF_CL_HAS_EXISTING_CORE)
  set_target_properties(adasdf_existing_core PROPERTIES EXPORT_NAME adasdf_existing_core)
  list(APPEND ADASDF_CL_INSTALL_TARGETS adasdf_existing_core)
endif()
set(ADASDF_CL_INSTALL_RUNTIME_TARGET TRUE)
set(ADASDF_CL_INSTALL_HAS_CUDA_BACKEND ${ADASDF_CL_HAS_CUDA_BACKEND})
set(ADASDF_CL_INSTALL_TOOL_TARGETS
  adasdf_build
  adasdf_info
  adasdf_query
  adasdf_collide
  adasdf_make_demo_box
  adasdf_recommend_demo
  adasdf_recommend_build
  adasdf_build_demo_adaptive
  adasdf_collide_boxes_demo
  adasdf_query_mode_demo
  adasdf_expansion_quality
  adasdf_capabilities
  adasdf_write_manifest
  adasdf_validate_report
  adasdf_collect_run_summary
  adasdf_mesh_check
  adasdf_mesh_clean
  adasdf_build_dense_sdf
  adasdf_build_adaptive_sdf
  adasdf_compress_adaptive_sdf
  adasdf_build_compressed_sdf
  adasdf_build_adaptive_sdf_preview
  adasdf_sparse_query
  adasdf_sparse_collide
  adasdf_contact_candidates
  adasdf_stabilize_contacts
  adasdf_solver_contact_candidates
  adasdf_world_broadphase
  adasdf_world_sparse_collide
  adasdf_world_solver_contacts
  adasdf_select_active_blocks
  adasdf_active_block_query
  adasdf_cuda_active_block_query)
if(ADASDF_CL_BUILD_BENCHMARKS)
  list(APPEND ADASDF_CL_INSTALL_TOOL_TARGETS
    adasdf_benchmark_batch_query
    adasdf_benchmark_sparse_query
    adasdf_benchmark_block_cache
    adasdf_benchmark_builder_acceleration
    adasdf_benchmark_contact_reduction
    adasdf_benchmark_collision_world
    adasdf_benchmark_cuda_block_cache
    adasdf_benchmark_hierarchical_sampling
    adasdf_benchmark_exact_hotpath
    adasdf_benchmark_contact_band_sampling
    adasdf_sweep_contact_band_sampling
    adasdf_sweep_hierarchical_sampling)
endif()

install(TARGETS ${ADASDF_CL_INSTALL_TARGETS}
  EXPORT AdaSDFCLTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${ADASDF_CL_INSTALL_TOOL_TARGETS}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES LICENSE README.md CHANGELOG.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY models/surrogate/v1_8
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/models/surrogate)

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

set(CPACK_PACKAGE_NAME "AdaSDF-CL")
set(CPACK_PACKAGE_VERSION "${ADASDF_CL_VERSION_STRING}")
set(CPACK_PACKAGE_VENDOR "AdaSDF-CL contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "Adaptive Signed Distance Field Collision Library")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "/\\\\.git/"
  "/build/"
  "/dist/"
  "/install/"
  "/__pycache__/"
  "/\\\\.cache/"
  "\\\\.sdfbin$"
  "\\\\.npz$"
  "\\\\.log$"
  "\\\\.svg$"
  "\\\\.zip$"
  "\\\\.tar\\\\.gz$"
  "/data/raw_stl/"
  "/data/quarantine/"
  "/datasets/build_results/"
  "/reports/experiments/")
include(CPack)
