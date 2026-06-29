#include "adasdf/visualization/CollisionSVGWriter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace adasdf {
namespace {

struct Bounds2 {
  double min_x = 0.0;
  double max_x = 1.0;
  double min_y = 0.0;
  double max_y = 1.0;
};

bool validAABB(const AABB& aabb) {
  return aabb.valid && aabb.min.allFinite() && aabb.max.allFinite() &&
      aabb.min.x <= aabb.max.x && aabb.min.y <= aabb.max.y;
}

Bounds2 sceneBounds(const CollisionSVGScene& scene) {
  if (!validAABB(scene.box_a) || !validAABB(scene.box_b)) {
    return {};
  }
  Bounds2 bounds;
  bounds.min_x = std::min(scene.box_a.min.x, scene.box_b.min.x);
  bounds.max_x = std::max(scene.box_a.max.x, scene.box_b.max.x);
  bounds.min_y = std::min(scene.box_a.min.y, scene.box_b.min.y);
  bounds.max_y = std::max(scene.box_a.max.y, scene.box_b.max.y);
  for (const Contact& contact : scene.result.contacts()) {
    bounds.min_x = std::min(bounds.min_x, contact.point.x);
    bounds.max_x = std::max(bounds.max_x, contact.point.x);
    bounds.min_y = std::min(bounds.min_y, contact.point.y);
    bounds.max_y = std::max(bounds.max_y, contact.point.y);
  }
  const double margin_x = std::max(0.1, (bounds.max_x - bounds.min_x) * 0.15);
  const double margin_y = std::max(0.1, (bounds.max_y - bounds.min_y) * 0.15);
  bounds.min_x -= margin_x;
  bounds.max_x += margin_x;
  bounds.min_y -= margin_y;
  bounds.max_y += margin_y;
  return bounds;
}

double mapX(double x, const Bounds2& bounds, double width) {
  return 40.0 + (x - bounds.min_x) /
      std::max(1.0e-9, bounds.max_x - bounds.min_x) * (width - 80.0);
}

double mapY(double y, const Bounds2& bounds, double height) {
  return height - 60.0 - (y - bounds.min_y) /
      std::max(1.0e-9, bounds.max_y - bounds.min_y) * (height - 120.0);
}

void writeBox(
    std::ostream& out,
    const AABB& box,
    const Bounds2& bounds,
    const char* css_class,
    double width,
    double height) {
  const double x0 = mapX(box.min.x, bounds, width);
  const double x1 = mapX(box.max.x, bounds, width);
  const double y0 = mapY(box.max.y, bounds, height);
  const double y1 = mapY(box.min.y, bounds, height);
  out << "<rect class=\"" << css_class << "\" x=\"" << x0
      << "\" y=\"" << y0 << "\" width=\"" << (x1 - x0)
      << "\" height=\"" << (y1 - y0) << "\" />\n";
}

}  // namespace

void CollisionSVGWriter::write(
    const std::filesystem::path& path,
    const CollisionSVGScene& scene) {
  if (path.empty()) {
    throw std::runtime_error("CollisionSVGWriter::write received an empty path.");
  }
  if (!validAABB(scene.box_a) || !validAABB(scene.box_b)) {
    throw std::runtime_error("CollisionSVGWriter requires valid AABBs.");
  }
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("Could not write collision SVG: " + path.string());
  }

  constexpr double width = 720.0;
  constexpr double height = 520.0;
  const Bounds2 bounds = sceneBounds(scene);
  out << std::fixed << std::setprecision(3);
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
      << "\" height=\"" << height << "\" viewBox=\"0 0 " << width
      << " " << height << "\">\n";
  out << "<style>"
      << ".box-a{fill:#6aa6ff33;stroke:#1f5fbf;stroke-width:3}"
      << ".box-b{fill:#f4c54244;stroke:#9b6a00;stroke-width:3}"
      << ".contact-marker{fill:#d12c2c;stroke:#7a1010;stroke-width:1.5}"
      << ".normal-arrow{stroke:#2f8f46;stroke-width:2.5;marker-end:url(#arrow)}"
      << ".label{font-family:Arial,sans-serif;font-size:14px;fill:#222}"
      << ".title{font-family:Arial,sans-serif;font-size:20px;font-weight:bold;fill:#111}"
      << "</style>\n";
  out << "<defs><marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"8\" refY=\"5\" "
      << "markerWidth=\"6\" markerHeight=\"6\" orient=\"auto-start-reverse\">"
      << "<path d=\"M 0 0 L 10 5 L 0 10 z\" fill=\"#2f8f46\"/>"
      << "</marker></defs>\n";
  out << "<rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\""
      << height << "\" fill=\"#ffffff\"/>\n";
  out << "<text class=\"title\" x=\"32\" y=\"34\">" << scene.title
      << "</text>\n";
  writeBox(out, scene.box_a, bounds, "box-a", width, height);
  writeBox(out, scene.box_b, bounds, "box-b", width, height);
  out << "<text class=\"label\" x=\"32\" y=\"466\">Colliding: "
      << (scene.result.isColliding() ? "true" : "false")
      << " | Minimum distance: " << scene.result.minimumDistance()
      << " | Requested max contacts: " << scene.requested_max_contacts
      << " | Returned contacts: " << scene.result.contacts().size()
      << "</text>\n";
  out << "<text class=\"label\" x=\"32\" y=\"488\">Backend: "
      << (scene.backend.empty() ? scene.result.backendInfo() : scene.backend)
      << "</text>\n";
  out << "<text class=\"label\" x=\"32\" y=\"508\">Method: "
      << (scene.method.empty() ? scene.result.methodInfo() : scene.method)
      << "</text>\n";

  int index = 0;
  for (const Contact& contact : scene.result.contacts()) {
    const double cx = mapX(contact.point.x, bounds, width);
    const double cy = mapY(contact.point.y, bounds, height);
    const double nx = cx + contact.normal.x * 34.0;
    const double ny = cy - contact.normal.y * 34.0;
    out << "<circle class=\"contact-marker\" data-contact=\"" << index
        << "\" cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"5\" />\n";
    out << "<line class=\"normal-arrow\" x1=\"" << cx << "\" y1=\""
        << cy << "\" x2=\"" << nx << "\" y2=\"" << ny << "\" />\n";
    ++index;
  }
  out << "</svg>\n";
}

}  // namespace adasdf
