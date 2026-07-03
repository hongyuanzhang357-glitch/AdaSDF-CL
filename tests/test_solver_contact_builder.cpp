#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

namespace {

adasdf::ContactCandidate candidate(int id, double depth) {
  adasdf::ContactCandidate c;
  c.sample_id = id;
  c.point = {1.0, 2.0, 3.0};
  c.normal = {0.0, 1.0, 0.0};
  c.has_normal = true;
  c.penetration_depth = depth;
  c.phi = -depth;
  c.effective_phi = -depth;
  c.object_id = 1;
  c.link_id = 2;
  c.group_id = 3;
  return c;
}

}  // namespace

int main() {
  std::vector<adasdf::ContactCandidate> candidates = {
      candidate(2, 0.1), candidate(1, 0.5)};
  auto patch = adasdf::ContactPatchBuilder::fromMembers(4, candidates);
  auto contacts =
      adasdf::SolverContactBuilder::fromCandidates(candidates, {patch});
  if (contacts.size() != 2 || contacts.contacts[0].contact_id != 0 ||
      contacts.contacts[1].contact_id != 1) {
    std::cerr << "solver contact ids failed\n";
    return 1;
  }
  if (contacts.contacts[0].sample_id != 1 ||
      contacts.contacts[0].patch_id != 4 ||
      contacts.contacts[0].stable_key.empty() ||
      contacts.contacts[0].stable_key.find("1:2:3:4:1") == std::string::npos ||
      !contacts.contacts[0].normal.allFinite()) {
    std::cerr << "solver contact fields failed\n";
    return 1;
  }
  std::cout << "solver contact builder passed\n";
  return 0;
}
