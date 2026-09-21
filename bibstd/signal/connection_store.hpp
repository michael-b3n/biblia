#pragma once

#include "bibstd/signal/common.hpp"

#include <mutex>
#include <vector>

namespace bibstd::signal
{

///
/// Connection store that stores connections to signals and disconnects them when destroyed.
///
class connection_store final
{
  // Variables
  mutable std::mutex mtx_;
  std::vector<scoped_connection_type> connections_;

public: // Constructor
  connection_store() = default;
  ~connection_store() noexcept;

public: // Modifiers
  ///
  /// Add connection to connection store.
  ///
  auto store(scoped_connection_type con) -> void;

  ///
  /// Clear connection store and disconnect all connections.
  ///
  auto clear() -> void;
};

} // namespace bibstd::signal
