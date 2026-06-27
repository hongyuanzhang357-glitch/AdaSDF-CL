#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  if (adasdf::SurrogateRecommender::isAvailable()) {
    std::cerr << "Surrogate backend unexpectedly reports available in this build\n";
    return 1;
  }

  adasdf::BuildOptions options;
  const auto recommendations =
      adasdf::SurrogateRecommender::recommend("missing.stl", options, 3);
  if (recommendations.empty()) {
    std::cerr << "Unavailable surrogate should return an explanatory placeholder\n";
    return 1;
  }
  if (recommendations.front().credible) {
    std::cerr << "Unavailable surrogate returned a credible recommendation\n";
    return 1;
  }
  if (recommendations.front().note.find("not available") == std::string::npos) {
    std::cerr << "Unavailable surrogate note is not explicit\n";
    return 1;
  }
  return 0;
}
