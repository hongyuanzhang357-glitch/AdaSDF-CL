#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "adasdf/query/Contact.h"

namespace adasdf {

class DistanceResult {
 public:
  void clear();

  bool hasResult() const {
    return has_result_;
  }

  void setMinimumDistance(Scalar distance) {
    minimum_distance_ = distance;
    has_result_ = true;
  }

  Scalar minimumDistance() const {
    return minimum_distance_;
  }

  void setNearestPoints(const Vector3& point_a, const Vector3& point_b) {
    nearest_point_a_ = point_a;
    nearest_point_b_ = point_b;
    has_result_ = true;
  }

  const Vector3& nearestPointA() const {
    return nearest_point_a_;
  }

  const Vector3& nearestPointB() const {
    return nearest_point_b_;
  }

  void setNormal(const Vector3& normal) {
    normal_ = normal;
  }

  const Vector3& normal() const {
    return normal_;
  }

  void setNumSDFQueries(std::size_t count) {
    sdf_query_count_ = count;
  }

  std::size_t numSDFQueries() const {
    return sdf_query_count_;
  }

  void setClosestContact(const Contact& contact) {
    closest_contact_ = contact;
  }

  const Contact& closestContact() const {
    return closest_contact_;
  }

  void setNumCandidatePoints(std::size_t count) {
    candidate_point_count_ = count;
  }

  std::size_t numCandidatePoints() const {
    return candidate_point_count_;
  }

  void setNumRawContacts(std::size_t count) {
    raw_contact_count_ = count;
  }

  std::size_t numRawContacts() const {
    return raw_contact_count_;
  }

  void setNumReducedContacts(std::size_t count) {
    reduced_contact_count_ = count;
  }

  std::size_t numReducedContacts() const {
    return reduced_contact_count_;
  }

  void setBackendInfo(std::string backend_info) {
    backend_info_ = std::move(backend_info);
  }

  const std::string& backendInfo() const {
    return backend_info_;
  }

  void setMethodInfo(std::string method_info) {
    method_info_ = std::move(method_info);
  }

  const std::string& methodInfo() const {
    return method_info_;
  }

 private:
  bool has_result_ = false;
  Scalar minimum_distance_ = 0.0;
  Vector3 nearest_point_a_;
  Vector3 nearest_point_b_;
  Vector3 normal_;
  Contact closest_contact_;
  std::size_t sdf_query_count_ = 0;
  std::size_t candidate_point_count_ = 0;
  std::size_t raw_contact_count_ = 0;
  std::size_t reduced_contact_count_ = 0;
  std::string backend_info_;
  std::string method_info_;
};

}  // namespace adasdf
