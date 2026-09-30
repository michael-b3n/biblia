#pragma once

#include "bibstd/util/non_owning_ptr.hpp"

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace bibstd::core
{

///
/// Core cache. Keeps text values by key in a SQLite database file, so they last
/// beyond the run of the app and several processes may use the file at once. A
/// cache is not worth a failing script: a database that can not be opened, read
/// or written is logged, the cache then holds nothing and keeps nothing.
///
class core_cache final
{
  // Typedefs
  struct database_closer final
  {
    auto operator()(util::non_owning_ptr<sqlite3> database) const noexcept -> void;
  };

  struct statement_closer final
  {
    auto operator()(util::non_owning_ptr<sqlite3_stmt> statement) const noexcept -> void;
  };

  using database_ptr = std::unique_ptr<sqlite3, database_closer>;
  using statement_ptr = std::unique_ptr<sqlite3_stmt, statement_closer>;

  // Variables
  const std::filesystem::path file_;
  mutable std::mutex mtx_;
  database_ptr database_;
  statement_ptr get_;
  statement_ptr set_;
  statement_ptr remove_;

public: // Structors
  explicit core_cache(std::filesystem::path file);
  ~core_cache() noexcept = default;
  core_cache(const core_cache&) = delete;
  core_cache(core_cache&&) = delete;

public: // Operators
  auto operator=(const core_cache&) -> core_cache& = delete;
  auto operator=(core_cache&&) -> core_cache& = delete;

public: // Accessors
  ///
  /// \return the value of \p key, none if it is not cached
  ///
  [[nodiscard]] auto get(const std::string& key) const -> std::optional<std::string>;

public: // Modifiers
  ///
  /// Cache \p value for \p key, none removes it.
  ///
  auto set(const std::string& key, std::optional<std::string> value) -> void;

private: // Implementation
  [[nodiscard]] auto open() const -> database_ptr;
  [[nodiscard]] auto prepare(std::string_view sql) const -> statement_ptr;
  [[nodiscard]] auto succeeded(int status, std::string_view action) const -> bool;
};

} // namespace bibstd::core
