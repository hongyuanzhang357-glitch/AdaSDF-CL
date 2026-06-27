#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  static_assert(adasdf::versionMajor() == 0, "unexpected major version");
  static_assert(adasdf::versionMinor() >= 7, "package test expects v0.7 or newer");

  adasdf::CollisionRequest request;
  request.backend = adasdf::BackendType::CPU;
  request.query_mode = adasdf::QueryMode::Balanced;

  const adasdf::Transform transform = adasdf::Transform::Identity();
  const adasdf::Vector3 point = transform.applyPoint({1.0, 2.0, 3.0});
  const auto backend = adasdf::makeBackend(adasdf::BackendType::CPU);

  std::cout << "AdaSDF-CL package consumer\n";
  std::cout << "Version: " << adasdf::versionString() << "\n";
  std::cout << "Point: " << point << "\n";
  std::cout << "CPU backend available: "
            << (backend && backend->available() ? "true" : "false") << "\n";
  return backend ? 0 : 1;
}
