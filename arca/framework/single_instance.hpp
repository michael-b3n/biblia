#pragma once

#include <boost/interprocess/sync/file_lock.hpp>

#include <optional>
#include <string>

namespace bibstd::framework
{

///
/// The single instance guard holds an exclusive lock on a file in the local data folder.
/// That folder belongs to the current user, so the guard scopes an instance to a logon
/// session only. The lock is held by the OS and released when the process ends.
///
class [[nodiscard]] single_instance final
{
  // Variables
  std::optional<boost::interprocess::file_lock> lock_;
  std::optional<std::string> error_;
  bool owner_{false};

public: // Constructors
  ~single_instance() noexcept = default;
  single_instance(single_instance&&) = delete;
  single_instance(const single_instance&) = delete;

public: // Operators
  auto operator=(single_instance&&) -> single_instance& = delete;
  auto operator=(const single_instance&) -> single_instance& = delete;

public: // Static operations
  ///
  /// Claim the single instance of the application \p name, which also names its local data folder.
  /// \return guard holding the claim while it lives
  ///
  [[nodiscard]] static auto claim(const std::string& name) -> single_instance;

public: // Accessors
  ///
  /// Check if this process is the only running instance.
  /// \return true if no other instance was running, false otherwise
  ///
  [[nodiscard]] auto is_owner() const -> bool;

  ///
  /// Access the reason why no lock could be taken. The application is started in that case,
  /// the caller shall log the reason as soon as logging is available.
  /// \return error description if the guard is inactive, nullopt otherwise
  ///
  [[nodiscard]] auto error() const -> const std::optional<std::string>&;

private: // Constructors
  explicit single_instance(const std::string& name);
};

} // namespace bibstd::framework
