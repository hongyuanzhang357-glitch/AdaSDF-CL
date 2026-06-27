#pragma once

#include <string>
#include <utility>

#include "adasdf/query/Contact.h"

namespace adasdf {

struct QueryTiming {
  double broadphase_seconds = 0.0;
  double narrowphase_seconds = 0.0;
  double sdf_query_seconds = 0.0;
  double total_seconds = 0.0;
};

class CollisionResult {
 public:
  void clear();
  void addContact(const Contact& contact);

  void setColliding(bool flag) {
    is_colliding_ = flag;
  }

  bool isColliding() const {
    return is_colliding_;
  }

  void setMinimumDistance(Scalar distance) {
    minimum_distance_ = distance;
  }

  Scalar minimumDistance() const {
    return minimum_distance_;
  }

  const ContactList& contacts() const {
    return contacts_;
  }

  const QueryTiming& timing() const {
    return timing_;
  }

  void setTiming(const QueryTiming& timing) {
    timing_ = timing;
  }

  void setNumSDFQueries(std::size_t count) {
    sdf_query_count_ = count;
  }

  std::size_t sdfQueryCount() const {
    return sdf_query_count_;
  }

  std::size_t numSDFQueries() const {
    return sdf_query_count_;
  }

  void setBackendInfo(std::string backend_info) {
    backend_info_ = std::move(backend_info);
  }

  const std::string& backendInfo() const {
    return backend_info_;
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

  void setMethodInfo(std::string method_info) {
    method_info_ = std::move(method_info);
  }

  const std::string& methodInfo() const {
    return method_info_;
  }

 private:
  bool is_colliding_ = false;
  Scalar minimum_distance_ = 0.0;
  ContactList contacts_;
  QueryTiming timing_;
  std::size_t sdf_query_count_ = 0;
  std::size_t candidate_point_count_ = 0;
  std::size_t raw_contact_count_ = 0;
  std::size_t reduced_contact_count_ = 0;
  std::string backend_info_;
  std::string method_info_;
};

}  // namespace adasdf
