#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

int main() {
  try {
    const std::filesystem::path source_dir = ADASDF_CL_SOURCE_DIR;
    const std::filesystem::path fixture =
        source_dir / "tests" / "data" / "mesh_diagnostics" /
        "closed_cube_ascii.stl";
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() /
        "adasdf_cl_adaptive_block_compression_demo.sdfbin";

    adasdf::AdaptiveBlockSDFBuildOptions build_options;
    build_options.max_octree_level = 2;
    build_options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport build_report;
    auto dense_model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        fixture.string(),
        build_options,
        &build_report);
    auto adaptive =
        std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(dense_model);
    if (!adaptive) {
      std::cerr << build_report.error_message << "\n";
      return 1;
    }

    adasdf::BlockLowRankCompressionOptions compression_options;
    compression_options.max_rank = 5;
    adasdf::BlockLowRankCompressionReport compression_report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            compression_options,
            &compression_report);
    auto compressed_model =
        std::make_shared<adasdf::CompressedAdaptiveBlockSDFModel>(
            std::move(compressed));
    if (!compressed_model->isValid()) {
      return 1;
    }
    adasdf::SDFBinWriter::write(output.string(), *compressed_model);
    auto reloaded = adasdf::SDFBinReader::read(output);
    adasdf::CollisionObject a(reloaded);
    adasdf::CollisionObject b(reloaded);
    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 4;
    adasdf::CollisionResult result;
    const bool hit = adasdf::collide(a, b, request, result);

    std::cout << "AdaSDF-CL adaptive block compression demo\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Adaptive blocks: " << build_report.block_count << "\n";
    std::cout << "Matrix-SVD blocks: "
              << compression_report.compressed_block_count << "\n";
    std::cout << "Dense fallback blocks: "
              << compression_report.dense_fallback_block_count << "\n";
    std::cout << "Compression ratio: "
              << compression_report.compression_ratio << "\n";
    std::cout << "Origin phi: " << reloaded->sampleDistance({0.0, 0.0, 0.0})
              << "\n";
    std::cout << "Outside phi: " << reloaded->sampleDistance({1.5, 0.5, 0.5})
              << "\n";
    std::cout << "Collision hit: " << (hit ? "true" : "false") << "\n";
    std::cout << "Contact count: " << result.contacts().size() << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adaptive block compression demo failed: " << exc.what()
              << "\n";
    return 1;
  }
}
