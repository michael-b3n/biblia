#include "bibstd/core/core_cache.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/numeric_cast.hpp"

#include <sqlite3.h>

#include <chrono>
#include <cstddef>
#include <ranges>
#include <system_error>
#include <tuple>
#include <utility>

namespace bibstd::core
{
namespace
{

// How long a change waits for another process changing the file
constexpr auto busy_timeout = std::chrono::seconds{5};

// Write-ahead log: a change is appended without waiting for the disk, so a script may set many values one by one,
// and other processes read meanwhile. A power failure may lose the last changes, the file stays intact.
constexpr auto setup_sql = "PRAGMA journal_mode = WAL;"
                           "PRAGMA synchronous = NORMAL;"
                           "CREATE TABLE IF NOT EXISTS entries (key TEXT PRIMARY KEY, value TEXT NOT NULL) WITHOUT ROWID;";
constexpr auto get_sql = "SELECT value FROM entries WHERE key = ?1;";
constexpr auto set_sql = "INSERT INTO entries (key, value) VALUES (?1, ?2) "
                         "ON CONFLICT (key) DO UPDATE SET value = excluded.value;";
constexpr auto remove_sql = "DELETE FROM entries WHERE key = ?1;";

///
/// Bind \p text as parameter \p index of \p statement, it must live until the statement is reset.
/// \return the status of SQLite
///
[[nodiscard]] auto bind_text(const util::non_owning_ptr<sqlite3_stmt> statement, const int index, const std::string& text)
  -> int
{
  // With its size, so a text holds any character
  return sqlite3_bind_text(statement, index, text.data(), numeric_cast<int>(text.size()), SQLITE_STATIC);
}

///
/// Leave \p statement ready for its next use.
///
auto reset(const util::non_owning_ptr<sqlite3_stmt> statement) -> void
{
  sqlite3_reset(statement);
  sqlite3_clear_bindings(statement);
}

} // namespace

///
///
auto core_cache::database_closer::operator()(const util::non_owning_ptr<sqlite3> database) const noexcept -> void
{
  sqlite3_close(database);
}

///
///
auto core_cache::statement_closer::operator()(const util::non_owning_ptr<sqlite3_stmt> statement) const noexcept -> void
{
  sqlite3_finalize(statement);
}

///
///
core_cache::core_cache(std::filesystem::path file)
  : file_{std::move(file)}
  , database_{open()}
  , get_{prepare(get_sql)}
  , set_{prepare(set_sql)}
  , remove_{prepare(remove_sql)}
{
}

///
///
auto core_cache::get(const std::string& key) const -> std::optional<std::string>
{
  const auto lock = std::scoped_lock{mtx_};
  if(!get_ || !succeeded(bind_text(get_.get(), 1, key), "bind"))
  {
    return std::nullopt;
  }
  auto value = std::optional<std::string>{};
  if(const auto status = sqlite3_step(get_.get()); status == SQLITE_ROW)
  {
    // The bytes as they were set, the size is only valid once they are read
    const auto text = static_cast<util::non_owning_ptr<const char>>(sqlite3_column_blob(get_.get(), 0));
    const auto size = numeric_cast<std::size_t>(sqlite3_column_bytes(get_.get(), 0));
    value = text != nullptr ? std::string{text, size} : std::string{};
  }
  else
  {
    std::ignore = succeeded(status, "read");
  }
  reset(get_.get());
  return value;
}

///
///
auto core_cache::set(const std::string& key, std::optional<std::string> value) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  const auto& statement = value ? set_ : remove_;
  if(!statement)
  {
    return;
  }
  if(
    succeeded(bind_text(statement.get(), 1, key), "bind") &&
    (!value || succeeded(bind_text(statement.get(), 2, *value), "bind"))
  )
  {
    std::ignore = succeeded(sqlite3_step(statement.get()), "write");
  }
  reset(statement.get());
}

///
///
auto core_cache::open() const -> database_ptr
{
  auto error = std::error_code{};
  std::filesystem::create_directories(file_.parent_path(), error);
  // SQLite takes the path as UTF-8
  const auto path = file_.u8string();
  auto opened = util::non_owning_ptr<sqlite3>{nullptr};
  const auto status = sqlite3_open_v2(
    std::string{std::ranges::begin(path), std::ranges::end(path)}.c_str(),
    &opened,
    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
    nullptr
  );
  // Owned also if opening failed, SQLite wants it closed
  auto database = database_ptr{opened};
  if(status != SQLITE_OK)
  {
    LOG_ERROR("failed to open cache: file=\"{}\", error={}", file_.generic_string(), sqlite3_errstr(status));
    return nullptr;
  }
  sqlite3_busy_timeout(database.get(), numeric_cast<int>(std::chrono::milliseconds{busy_timeout}.count()));
  if(const auto setup = sqlite3_exec(database.get(), setup_sql, nullptr, nullptr, nullptr); setup != SQLITE_OK)
  {
    // E.g. a file that is no database
    LOG_ERROR("failed to set up cache: file=\"{}\", error={}", file_.generic_string(), sqlite3_errmsg(database.get()));
    return nullptr;
  }
  return database;
}

///
///
auto core_cache::prepare(const std::string_view sql) const -> statement_ptr
{
  if(!database_)
  {
    return nullptr;
  }
  auto prepared = util::non_owning_ptr<sqlite3_stmt>{nullptr};
  const auto status = sqlite3_prepare_v2(database_.get(), sql.data(), numeric_cast<int>(sql.size()), &prepared, nullptr);
  auto statement = statement_ptr{prepared};
  return succeeded(status, "prepare") ? std::move(statement) : nullptr;
}

///
///
auto core_cache::succeeded(const int status, const std::string_view action) const -> bool
{
  if(status == SQLITE_OK || status == SQLITE_DONE || status == SQLITE_ROW)
  {
    return true;
  }
  LOG_ERROR("cache failed: file=\"{}\", action={}, error={}", file_.generic_string(), action, sqlite3_errmsg(database_.get()));
  return false;
}

} // namespace bibstd::core
