#pragma once

#include <boost/interprocess/sync/file_lock.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace bibstd::system
{

///
/// The single instance guard holds an exclusive lock on a file in the data folder of the application.
/// That folder belongs to the current user, so the guard scopes an instance to a logon
/// session only. The lock is held by the OS and released when the process ends.
///
class [[nodiscard]] single_instance final
{
  // Variables
  std::optional<boost::interprocess::file_lock> lock_;
  std::optional<std::string> error_;
  bool owner_{false};

public: // Structors
  ~single_instance() noexcept = default;
  single_instance(single_instance&&) = delete;
  single_instance(const single_instance&) = delete;

public: // Operators
  auto operator=(single_instance&&) -> single_instance& = delete;
  auto operator=(const single_instance&) -> single_instance& = delete;

public: // Static operations
  ///
  /// Claim the single instance of the application \p name, its lock file is in \p data_folder.
  /// \return guard holding the claim while it lives
  ///
  [[nodiscard]] static auto claim(const std::filesystem::path& data_folder, const std::string& name)
    -> single_instance;

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

private: // Structors
  single_instance(const std::filesystem::path& data_folder, const std::string& name);
};

} // namespace bibstd::system
