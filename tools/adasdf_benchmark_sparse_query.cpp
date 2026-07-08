#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "ModelJsonHelpers.h"

namespace {

enum class Mode {
  PhiOnly,
  PhiNormal,
  CollisionOnly,
  Clearance,
  Candidates
};

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_sparse_query model.sdfbin samples.csv "
         "[--repeat N] [--warmup N] "
         "[--mode phi-only|phi-normal|collision-only|clearance|candidates] "
         "[--threshold value] [--top-k N] [--early-exit] [--with-normal] "
         "[--no-radius] [--report benchmark.md] [--json [benchmark.json]] "
         "[--csv benchmark.csv]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

Mode parseMode(const std::string& text) {
  if (text == "phi-only") {
    return Mode::PhiOnly;
  }
  if (text == "phi-normal" || text == "phi+normal") {
    return Mode::PhiNormal;
  }
  if (text == "collision-only") {
    return Mode::CollisionOnly;
  }
  if (text == "clearance") {
    return Mode::Clearance;
  }
  if (text == "candidates") {
    return Mode::Candidates;
  }
  throw std::runtime_error("unsupported sparse benchmark mode: " + text);
}

const char* toString(Mode mode) {
  switch (mode) {
    case Mode::PhiOnly:
      return "phi-only";
    case Mode::PhiNormal:
      return "phi-normal";
    case Mode::CollisionOnly:
      return "collision-only";
    case Mode::Clearance:
      return "clearance";
    case Mode::Candidates:
      return "candidates";
  }
  return "unknown";
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << text;
  return true;
}

struct Metrics {
  std::size_t sample_count = 0;
  int repeat = 0;
  int warmup = 0;
  double total_ms = 0.0;
  double avg_ms = 0.0;
  double avg_us = 0.0;
  double avg_ns_per_sample = 0.0;
  double queried_samples_avg = 0.0;
  double early_exit_rate = 0.0;
  std::size_t candidate_count = 0;
};

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    if (argc < 3) {
      usage();
      return 1;
    }

    const std::filesystem::path model_path = argv[1];
    const std::filesystem::path samples_path = argv[2];
    int repeat = 10;
    int warmup = 1;
    Mode mode = Mode::PhiOnly;
    double threshold = 0.0;
    int top_k = 8;
    bool early_exit = false;
    bool with_normal = false;
    bool use_radius = true;
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    std::filesystem::path csv_path;
    bool json_stdout = false;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--repeat" && hasValue(i, argc)) {
        repeat = std::stoi(argv[++i]);
      } else if (arg == "--warmup" && hasValue(i, argc)) {
        warmup = std::stoi(argv[++i]);
      } else if (arg == "--mode" && hasValue(i, argc)) {
        mode = parseMode(argv[++i]);
      } else if (arg == "--threshold" && hasValue(i, argc)) {
        threshold = std::stod(argv[++i]);
      } else if (arg == "--top-k" && hasValue(i, argc)) {
        top_k = std::stoi(argv[++i]);
      } else if (arg == "--early-exit") {
        early_exit = true;
      } else if (arg == "--with-normal") {
        with_normal = true;
      } else if (arg == "--no-radius") {
        use_radius = false;
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json") {
        if (hasValue(i, argc) && std::string(argv[i + 1]).rfind("-", 0) != 0) {
          json_path = argv[++i];
        } else {
          json_stdout = true;
        }
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }

    repeat = std::max(1, repeat);
    warmup = std::max(0, warmup);

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "adasdf_benchmark_sparse_query: failed to load queryable model\n";
      return 1;
    }
    const auto samples = adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_benchmark_sparse_query: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    Metrics metrics;
    metrics.sample_count = samples.sample_set.size();
    metrics.repeat = repeat;
    metrics.warmup = warmup;

    auto run_once = [&]() {
      if (mode == Mode::CollisionOnly || mode == Mode::Clearance) {
        adasdf::SparseCollisionQueryOptions options;
        options.mode = mode == Mode::CollisionOnly
            ? adasdf::SparseCollisionMode::CollisionOnly
            : adasdf::SparseCollisionMode::Clearance;
        options.threshold = threshold;
        options.early_exit = mode == Mode::CollisionOnly && early_exit;
        options.compute_normals = with_normal;
        options.use_sample_radius = use_radius;
        const auto result =
            adasdf::SparseCollisionQuery::check(*model, samples.sample_set, options);
        metrics.queried_samples_avg += static_cast<double>(result.queried_count);
        metrics.early_exit_rate += result.early_exit_triggered ? 1.0 : 0.0;
        if (!result.success) {
          throw std::runtime_error(result.error_message);
        }
        return;
      }

      adasdf::SparseSDFQueryOptions query_options;
      query_options.threshold = threshold;
      query_options.use_sample_radius = use_radius;
      query_options.early_exit = false;
      query_options.include_non_colliding_samples = true;
      query_options.compute_normals = with_normal || mode == Mode::PhiNormal ||
          mode == Mode::Candidates;
      query_options.output_mode = query_options.compute_normals
          ? adasdf::SparseQueryOutputMode::PhiAndNormal
          : adasdf::SparseQueryOutputMode::PhiOnly;
      const auto query =
          adasdf::SparseSDFQuery::query(*model, samples.sample_set, query_options);
      if (!query.success) {
        throw std::runtime_error(query.error_message);
      }
      metrics.queried_samples_avg += static_cast<double>(query.stats.queried_count);
      if (mode == Mode::Candidates) {
        adasdf::ContactCandidateOptions candidate_options;
        candidate_options.top_k = top_k;
        candidate_options.candidate_threshold = threshold;
        candidate_options.compute_normals = query_options.compute_normals;
        const auto reduced =
            adasdf::ContactCandidateReducer::reduce(query.samples, candidate_options);
        metrics.candidate_count += reduced.reduced_count;
      }
    };

    for (int i = 0; i < warmup; ++i) {
      run_once();
    }
    metrics.queried_samples_avg = 0.0;
    metrics.early_exit_rate = 0.0;
    metrics.candidate_count = 0;

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < repeat; ++i) {
      run_once();
    }
    metrics.total_ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start)
            .count();
    metrics.avg_ms = metrics.total_ms / static_cast<double>(repeat);
    metrics.avg_us = metrics.avg_ms * 1000.0;
    metrics.queried_samples_avg /= static_cast<double>(repeat);
    metrics.early_exit_rate /= static_cast<double>(repeat);
    metrics.avg_ns_per_sample = metrics.queried_samples_avg > 0.0
        ? metrics.avg_ms * 1.0e6 / metrics.queried_samples_avg
        : 0.0;
    if (mode == Mode::Candidates) {
      metrics.candidate_count /= static_cast<std::size_t>(repeat);
    }

    if (json_stdout) {
      adasdf::BackendJsonContract contract = adasdf_tools::makeBaseContract(
          adasdf::SchemaIds::Benchmark,
          "adasdf_benchmark_sparse_query");
      contract.payload_fields.push_back(
          {"sample_count",
           adasdf::JsonContractWriter::integer(metrics.sample_count)});
      contract.payload_fields.push_back(
          {"query_count",
           adasdf::JsonContractWriter::number(metrics.queried_samples_avg)});
      contract.payload_fields.push_back(
          {"time_ms", adasdf::JsonContractWriter::number(metrics.avg_ms)});
      contract.payload_fields.push_back(
          {"total_time_ms",
           adasdf::JsonContractWriter::number(metrics.total_ms)});
      contract.payload_fields.push_back(
          {"ns_per_query",
           adasdf::JsonContractWriter::number(metrics.avg_ns_per_sample)});
      const double throughput = metrics.avg_ms > 0.0
          ? metrics.queried_samples_avg / (metrics.avg_ms / 1000.0)
          : 0.0;
      contract.payload_fields.push_back(
          {"throughput", adasdf::JsonContractWriter::number(throughput)});
      contract.payload_fields.push_back(
          {"backend", adasdf::JsonContractWriter::quote("cpu")});
      contract.payload_fields.push_back(
          {"model_type",
           adasdf::JsonContractWriter::quote(adasdf_tools::modelType(*model))});
      contract.payload_fields.push_back(
          {"mode", adasdf::JsonContractWriter::quote(toString(mode))});
      contract.payload_fields.push_back(
          {"repeat", adasdf::JsonContractWriter::integerSigned(repeat)});
      contract.payload_fields.push_back(
          {"warmup", adasdf::JsonContractWriter::integerSigned(warmup)});
      contract.payload_fields.push_back(
          {"phi_stats",
           "{\"threshold\":" + adasdf::JsonContractWriter::number(threshold) +
               ",\"with_normal\":" +
               adasdf::JsonContractWriter::boolean(
                   with_normal || mode == Mode::PhiNormal ||
                   mode == Mode::Candidates) +
               "}"});
      contract.payload_fields.push_back(
          {"collision_stats",
           "{\"early_exit_rate\":" +
               adasdf::JsonContractWriter::number(metrics.early_exit_rate) +
               ",\"candidate_count\":" +
               adasdf::JsonContractWriter::integer(metrics.candidate_count) +
               "}"});
      std::cout << adasdf::JsonContractWriter::writeObject(contract);
    } else {
      std::cout << "sample_count,repeat,warmup,total_ms,avg_ms,avg_us,"
                   "avg_ns_per_sample,queried_samples_avg,early_exit_rate,mode,"
                   "with_normal,threshold,top_k,status\n";
      std::cout << metrics.sample_count << ","
                << metrics.repeat << ","
                << metrics.warmup << ","
                << metrics.total_ms << ","
                << metrics.avg_ms << ","
                << metrics.avg_us << ","
                << metrics.avg_ns_per_sample << ","
                << metrics.queried_samples_avg << ","
                << metrics.early_exit_rate << ","
                << toString(mode) << ","
                << (with_normal || mode == Mode::PhiNormal ||
                    mode == Mode::Candidates ? "true" : "false")
                << ","
                << threshold << ","
                << top_k << ",ok\n";
      std::cout << "Sparse benchmark mode: " << toString(mode) << "\n";
      std::cout << "Average ns per sample: " << metrics.avg_ns_per_sample
                << "\n";
      std::cout << "Status: ok\n";
    }

    const std::string csv =
        "sample_count,repeat,warmup,total_ms,avg_ms,avg_us,avg_ns_per_sample,"
        "queried_samples_avg,early_exit_rate,mode,with_normal,threshold,top_k\n" +
        std::to_string(metrics.sample_count) + "," +
        std::to_string(metrics.repeat) + "," +
        std::to_string(metrics.warmup) + "," +
        std::to_string(metrics.total_ms) + "," +
        std::to_string(metrics.avg_ms) + "," +
        std::to_string(metrics.avg_us) + "," +
        std::to_string(metrics.avg_ns_per_sample) + "," +
        std::to_string(metrics.queried_samples_avg) + "," +
        std::to_string(metrics.early_exit_rate) + "," +
        toString(mode) + "," +
        ((with_normal || mode == Mode::PhiNormal || mode == Mode::Candidates)
             ? "true"
             : "false") +
        "," + std::to_string(threshold) + "," + std::to_string(top_k) + "\n";
    if (!csv_path.empty() && !writeText(csv_path, csv)) {
      std::cerr << "adasdf_benchmark_sparse_query: failed to write CSV\n";
      return 2;
    }
    if (!report_path.empty()) {
      const std::string md =
          "# Sparse Query Benchmark\n\n"
          "- Mode: " + std::string(toString(mode)) + "\n"
          "- Sample count: " + std::to_string(metrics.sample_count) + "\n"
          "- Repeat: " + std::to_string(repeat) + "\n"
          "- Warmup: " + std::to_string(warmup) + "\n"
          "- Average ns per sample: " +
          std::to_string(metrics.avg_ns_per_sample) + "\n";
      if (!writeText(report_path, md)) {
        std::cerr << "adasdf_benchmark_sparse_query: failed to write report\n";
        return 2;
      }
    }
    if (!json_path.empty()) {
      adasdf::BackendJsonContract contract = adasdf_tools::makeBaseContract(
          adasdf::SchemaIds::Benchmark,
          "adasdf_benchmark_sparse_query");
      contract.payload_fields.push_back(
          {"sample_count",
           adasdf::JsonContractWriter::integer(metrics.sample_count)});
      contract.payload_fields.push_back(
          {"query_count",
           adasdf::JsonContractWriter::number(metrics.queried_samples_avg)});
      contract.payload_fields.push_back(
          {"time_ms", adasdf::JsonContractWriter::number(metrics.avg_ms)});
      contract.payload_fields.push_back(
          {"ns_per_query",
           adasdf::JsonContractWriter::number(metrics.avg_ns_per_sample)});
      const double throughput = metrics.avg_ms > 0.0
          ? metrics.queried_samples_avg / (metrics.avg_ms / 1000.0)
          : 0.0;
      contract.payload_fields.push_back(
          {"throughput", adasdf::JsonContractWriter::number(throughput)});
      contract.payload_fields.push_back(
          {"backend", adasdf::JsonContractWriter::quote("cpu")});
      contract.payload_fields.push_back(
          {"model_type",
           adasdf::JsonContractWriter::quote(adasdf_tools::modelType(*model))});
      contract.payload_fields.push_back(
          {"mode", adasdf::JsonContractWriter::quote(toString(mode))});
      const std::string json =
          adasdf::JsonContractWriter::writeObject(contract);
      if (!writeText(json_path, json)) {
        std::cerr << "adasdf_benchmark_sparse_query: failed to write JSON report\n";
        return 2;
      }
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_sparse_query failed: " << exc.what() << "\n";
    return 2;
  }
}
