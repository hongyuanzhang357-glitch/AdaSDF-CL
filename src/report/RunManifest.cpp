#include "adasdf/report/RunManifest.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <thread>
#include <vector>

#include "adasdf/version.h"

#ifndef ADASDF_CL_GIT_COMMIT
#define ADASDF_CL_GIT_COMMIT "unknown"
#endif

namespace adasdf {
namespace {

std::uint32_t rotr(std::uint32_t value, int shift) {
  return (value >> shift) | (value << (32 - shift));
}

std::uint32_t loadBigEndian(const unsigned char* data) {
  return (static_cast<std::uint32_t>(data[0]) << 24) |
      (static_cast<std::uint32_t>(data[1]) << 16) |
      (static_cast<std::uint32_t>(data[2]) << 8) |
      static_cast<std::uint32_t>(data[3]);
}

void storeBigEndian(std::uint64_t value, unsigned char* data) {
  for (int i = 7; i >= 0; --i) {
    data[7 - i] = static_cast<unsigned char>((value >> (i * 8)) & 0xffu);
  }
}

std::array<unsigned char, 32> sha256Bytes(const std::vector<unsigned char>& input) {
  static constexpr std::array<std::uint32_t, 64> k = {
      0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
      0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
      0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
      0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
      0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
      0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
      0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
      0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
      0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
      0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
      0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
      0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
      0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
      0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
      0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
      0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

  std::array<std::uint32_t, 8> h = {
      0x6a09e667u,
      0xbb67ae85u,
      0x3c6ef372u,
      0xa54ff53au,
      0x510e527fu,
      0x9b05688cu,
      0x1f83d9abu,
      0x5be0cd19u};

  std::vector<unsigned char> data = input;
  const std::uint64_t bit_length =
      static_cast<std::uint64_t>(data.size()) * 8u;
  data.push_back(0x80u);
  while ((data.size() % 64u) != 56u) {
    data.push_back(0u);
  }
  unsigned char length_bytes[8] = {};
  storeBigEndian(bit_length, length_bytes);
  data.insert(data.end(), length_bytes, length_bytes + 8);

  for (std::size_t chunk = 0; chunk < data.size(); chunk += 64) {
    std::array<std::uint32_t, 64> w = {};
    for (int i = 0; i < 16; ++i) {
      w[static_cast<std::size_t>(i)] =
          loadBigEndian(&data[chunk + static_cast<std::size_t>(i) * 4u]);
    }
    for (int i = 16; i < 64; ++i) {
      const std::uint32_t s0 =
          rotr(w[static_cast<std::size_t>(i - 15)], 7) ^
          rotr(w[static_cast<std::size_t>(i - 15)], 18) ^
          (w[static_cast<std::size_t>(i - 15)] >> 3);
      const std::uint32_t s1 =
          rotr(w[static_cast<std::size_t>(i - 2)], 17) ^
          rotr(w[static_cast<std::size_t>(i - 2)], 19) ^
          (w[static_cast<std::size_t>(i - 2)] >> 10);
      w[static_cast<std::size_t>(i)] =
          w[static_cast<std::size_t>(i - 16)] + s0 +
          w[static_cast<std::size_t>(i - 7)] + s1;
    }

    std::uint32_t a = h[0];
    std::uint32_t b = h[1];
    std::uint32_t c = h[2];
    std::uint32_t d = h[3];
    std::uint32_t e = h[4];
    std::uint32_t f = h[5];
    std::uint32_t g = h[6];
    std::uint32_t hh = h[7];
    for (int i = 0; i < 64; ++i) {
      const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const std::uint32_t ch = (e & f) ^ ((~e) & g);
      const std::uint32_t temp1 =
          hh + s1 + ch + k[static_cast<std::size_t>(i)] +
          w[static_cast<std::size_t>(i)];
      const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t temp2 = s0 + maj;
      hh = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
  }

  std::array<unsigned char, 32> digest = {};
  for (std::size_t i = 0; i < h.size(); ++i) {
    digest[i * 4u + 0u] = static_cast<unsigned char>((h[i] >> 24) & 0xffu);
    digest[i * 4u + 1u] = static_cast<unsigned char>((h[i] >> 16) & 0xffu);
    digest[i * 4u + 2u] = static_cast<unsigned char>((h[i] >> 8) & 0xffu);
    digest[i * 4u + 3u] = static_cast<unsigned char>(h[i] & 0xffu);
  }
  return digest;
}

std::string hexDigest(const std::array<unsigned char, 32>& digest) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const unsigned char byte : digest) {
    out << std::setw(2) << static_cast<unsigned int>(byte);
  }
  return out.str();
}

}  // namespace

std::string utcTimestamp(std::chrono::system_clock::time_point time) {
  const std::time_t raw = std::chrono::system_clock::to_time_t(time);
  std::tm utc = {};
#if defined(_WIN32)
  gmtime_s(&utc, &raw);
#else
  gmtime_r(&raw, &utc);
#endif
  std::ostringstream out;
  out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

std::string currentUtcTimestamp() {
  return utcTimestamp(std::chrono::system_clock::now());
}

std::string currentGitCommit() {
  return ADASDF_CL_GIT_COMMIT;
}

std::string currentPlatformName() {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "macos";
#elif defined(__linux__)
  return "linux";
#else
  return "unknown";
#endif
}

unsigned int currentCpuThreads() {
  const unsigned int threads = std::thread::hardware_concurrency();
  return threads == 0 ? 1 : threads;
}

bool buildCudaEnabled() {
#ifdef ADASDF_CL_ENABLE_CUDA
  return ADASDF_CL_ENABLE_CUDA != 0;
#else
  return false;
#endif
}

bool buildCudaAvailable() {
#ifdef ADASDF_CL_HAS_CUDA_BACKEND
  return ADASDF_CL_HAS_CUDA_BACKEND != 0;
#else
  return false;
#endif
}

std::string fileSha256(const std::filesystem::path& path) {
  if (path.empty() || !std::filesystem::is_regular_file(path)) {
    return "";
  }
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return "";
  }
  std::vector<unsigned char> data(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());
  return hexDigest(sha256Bytes(data));
}

std::string pathToReportString(const std::filesystem::path& path) {
  return path.u8string();
}

RunManifest makeRunManifest(
    const std::string& tool_name,
    const std::string& case_id,
    const std::filesystem::path& input_path,
    const std::filesystem::path& output_path) {
  RunManifest manifest;
  manifest.schema_version = strictReportSchemaVersion();
  manifest.adasdf_version = versionString();
  manifest.git_commit = currentGitCommit();
  manifest.tool_name = tool_name;
  manifest.case_id = case_id.empty() ? "default" : case_id;
  manifest.input_path = input_path;
  manifest.input_sha256 = fileSha256(input_path);
  manifest.output_path = output_path;
  manifest.output_sha256 = fileSha256(output_path);
  manifest.platform = currentPlatformName();
  manifest.cpu_threads = currentCpuThreads();
  manifest.cuda_enabled = buildCudaEnabled();
  manifest.cuda_available = buildCudaAvailable();
  return manifest;
}

StrictReportRecord toStrictReportRecord(const RunManifest& manifest) {
  StrictReportRecord record;
  record.fields["schema_version"] = manifest.schema_version;
  record.fields["adasdf_version"] = manifest.adasdf_version;
  record.fields["git_commit"] = manifest.git_commit;
  record.fields["tool_name"] = manifest.tool_name;
  record.fields["case_id"] = manifest.case_id;
  record.fields["input_path"] = pathToReportString(manifest.input_path);
  record.fields["input_sha256"] = manifest.input_sha256;
  record.fields["output_path"] = pathToReportString(manifest.output_path);
  record.fields["output_sha256"] = manifest.output_sha256;
  record.fields["status"] = manifest.status;
  record.fields["success"] = manifest.success ? "true" : "false";
  record.fields["failure_reason"] = manifest.failure_reason;
  record.fields["start_time_utc"] = manifest.start_time_utc;
  record.fields["end_time_utc"] = manifest.end_time_utc;
  {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<double>::max_digits10)
        << manifest.elapsed_ms;
    record.fields["elapsed_ms"] = out.str();
  }
  record.fields["platform"] = manifest.platform;
  record.fields["cpu_threads"] = std::to_string(manifest.cpu_threads);
  record.fields["cuda_enabled"] = manifest.cuda_enabled ? "true" : "false";
  record.fields["cuda_available"] = manifest.cuda_available ? "true" : "false";
  for (const auto& metric : manifest.metrics) {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<double>::max_digits10)
        << metric.second;
    record.metrics[metric.first] = out.str();
  }
  return record;
}

}  // namespace adasdf
