#pragma once

#include <memory>
#include <string>

#include "adasdf/config.h"

namespace adasdf {

class Backend {
 public:
  virtual ~Backend() = default;

  virtual BackendType type() const = 0;
  virtual bool available() const = 0;
  virtual std::string name() const = 0;
};

std::unique_ptr<Backend> makeBackend(BackendType type);

}  // namespace adasdf
