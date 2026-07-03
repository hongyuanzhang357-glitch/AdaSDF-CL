#include "adasdf/contact/SolverContact.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace adasdf {
namespace {

bool deeperFirst(const ContactCandidate& a, const ContactCandidate& b) {
  if (a.penetration_depth != b.penetration_depth) {
    return a.penetration_depth > b.penetration_depth;
  }
  return a.sample_id < b.sample_id;
}

bool normalFinite(const Vector3& normal) {
  return normal.allFinite();
}

std::string stableKey(const SolverContact& contact) {
  std::ostringstream out;
  out << contact.object_id << ":" << contact.link_id << ":"
      << contact.group_id << ":" << contact.patch_id << ":"
      << contact.sample_id;
  return out.str();
}

}  // namespace

std::size_t SolverContactSet::size() const {
  return contacts.size();
}

bool SolverContactSet::empty() const {
  return contacts.empty();
}

SolverContactSet SolverContactBuilder::fromCandidates(
    const std::vector<ContactCandidate>& candidates,
    const std::vector<ContactPatch>& patches) {
  std::map<int, int> sample_to_patch;
  for (const ContactPatch& patch : patches) {
    for (const ContactCandidate& member : patch.members) {
      sample_to_patch.emplace(member.sample_id, patch.patch_id);
    }
  }

  std::vector<ContactCandidate> sorted = candidates;
  std::sort(sorted.begin(), sorted.end(), deeperFirst);

  SolverContactSet set;
  int contact_id = 0;
  for (const ContactCandidate& candidate : sorted) {
    SolverContact contact;
    contact.contact_id = contact_id++;
    contact.sample_id = candidate.sample_id;
    const auto patch_iter = sample_to_patch.find(candidate.sample_id);
    contact.patch_id = patch_iter == sample_to_patch.end() ? -1 : patch_iter->second;
    contact.point = candidate.point;
    contact.normal =
        candidate.has_normal && normalFinite(candidate.normal) ? candidate.normal : Vector3{};
    contact.penetration_depth =
        std::isfinite(candidate.penetration_depth) ? candidate.penetration_depth : 0.0;
    contact.phi = std::isfinite(candidate.phi) ? candidate.phi : 0.0;
    contact.effective_phi =
        std::isfinite(candidate.effective_phi) ? candidate.effective_phi : contact.phi;
    contact.object_id = candidate.object_id;
    contact.link_id = candidate.link_id;
    contact.group_id = candidate.group_id;
    contact.label = candidate.label;
    contact.stable_key = stableKey(contact);
    set.contacts.push_back(contact);
  }
  return set;
}

}  // namespace adasdf
