#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"

#include <algorithm>
#include <sstream>

namespace adasdf {

bool parseBrickLevelMap(
    const std::string& text,
    std::vector<BrickLevelMapRange>* ranges,
    std::string* error) {
  if (ranges == nullptr) {
    return false;
  }
  ranges->clear();
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (item.empty()) {
      continue;
    }
    const std::size_t colon = item.find(':');
    const std::size_t dash = item.find('-');
    if (colon == std::string::npos || dash == std::string::npos ||
        dash > colon) {
      if (error != nullptr) {
        *error = "invalid brick-level-map item: " + item;
      }
      return false;
    }
    BrickLevelMapRange range;
    range.sampling_level_min = std::stoi(item.substr(0, dash));
    range.sampling_level_max =
        std::stoi(item.substr(dash + 1, colon - dash - 1));
    range.brick_level = std::stoi(item.substr(colon + 1));
    if (range.sampling_level_min > range.sampling_level_max ||
        range.sampling_level_min < 0 || range.brick_level < 0) {
      if (error != nullptr) {
        *error = "invalid brick-level-map range: " + item;
      }
      return false;
    }
    ranges->push_back(range);
  }
  if (ranges->empty()) {
    if (error != nullptr) {
      *error = "brick-level-map produced no ranges";
    }
    return false;
  }
  std::sort(ranges->begin(), ranges->end(), [](const auto& a, const auto& b) {
    if (a.sampling_level_min != b.sampling_level_min) {
      return a.sampling_level_min < b.sampling_level_min;
    }
    return a.sampling_level_max < b.sampling_level_max;
  });
  return true;
}

int brickLevelForSamplingLevel(
    const std::vector<BrickLevelMapRange>& ranges,
    int sampling_level) {
  int fallback = 0;
  for (const BrickLevelMapRange& range : ranges) {
    fallback = range.brick_level;
    if (sampling_level >= range.sampling_level_min &&
        sampling_level <= range.sampling_level_max) {
      return range.brick_level;
    }
  }
  return fallback;
}

const char* toString(NarrowBandTensorFillMode mode) {
  switch (mode) {
    case NarrowBandTensorFillMode::ExactAll:
      return "exact-all";
    case NarrowBandTensorFillMode::ContactExactFarInterp:
      return "contact-exact-far-interp";
    case NarrowBandTensorFillMode::CoarseProlongation:
      return "coarse-prolongation";
  }
  return "contact-exact-far-interp";
}

bool parseNarrowBandTensorFillMode(
    const std::string& text,
    NarrowBandTensorFillMode* mode) {
  if (mode == nullptr) {
    return false;
  }
  if (text == "exact-all") {
    *mode = NarrowBandTensorFillMode::ExactAll;
    return true;
  }
  if (text == "contact-exact-far-interp") {
    *mode = NarrowBandTensorFillMode::ContactExactFarInterp;
    return true;
  }
  if (text == "coarse-prolongation") {
    *mode = NarrowBandTensorFillMode::CoarseProlongation;
    return true;
  }
  return false;
}

const char* toString(NarrowBandCompressionMode mode) {
  switch (mode) {
    case NarrowBandCompressionMode::WeightedLowRank:
      return "weighted-low-rank";
    case NarrowBandCompressionMode::Dense:
      return "dense";
    case NarrowBandCompressionMode::ExistingLowRank:
      return "existing-low-rank";
  }
  return "weighted-low-rank";
}

bool parseNarrowBandCompressionMode(
    const std::string& text,
    NarrowBandCompressionMode* mode) {
  if (mode == nullptr) {
    return false;
  }
  if (text == "weighted-low-rank") {
    *mode = NarrowBandCompressionMode::WeightedLowRank;
    return true;
  }
  if (text == "dense") {
    *mode = NarrowBandCompressionMode::Dense;
    return true;
  }
  if (text == "existing-low-rank") {
    *mode = NarrowBandCompressionMode::ExistingLowRank;
    return true;
  }
  return false;
}

}  // namespace adasdf
