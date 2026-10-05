#include "bibstd/util/log.hpp"
#include "bibstd/system/filesystem.hpp"
#include "bibstd/util/date.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <print>
#include <ranges>
#include <utility>
#include <vector>

namespace bibstd::util
{
namespace
{

///
/// Constants
///
const auto logger_name = std::string{"main"};
const auto latest_log_file_name = std::string{"latest.log"};

///
/// Convert u8 string view to normal string.
/// \warning Normal strings are encoded in utf-8.
/// \return string with content of string
///
inline auto to_string(const std::u8string& u8string) -> std::string
{
  return std::string{u8string.begin(), u8string.end()};
}

///
/// Lock logger mutex.
///
inline auto lock_logger() -> std::scoped_lock<std::mutex>
{
  static std::mutex mtx;
  return std::scoped_lock{mtx};
}

///
/// Convert logger level to spdlog level.
///
constexpr auto to_spdlog_level(const logger_level level) -> spdlog::level::level_enum
{
  switch(level)
  {
  case logger_level::debug: return spdlog::level::debug;
  case logger_level::info: return spdlog::level::info;
  case logger_level::warning: return spdlog::level::warn;
  case logger_level::error: return spdlog::level::err;
  }
  return spdlog::level::debug;
}

///
/// Delete the oldest session log files in \p log_directory, such that at most \p max_count remain.
/// \return number of deleted files
///
inline auto prune_log_files(const std::filesystem::path& log_directory, const std::size_t max_count) -> std::size_t
{
  auto ec = std::error_code{};
  const auto is_session_log = [&](const auto& entry)
  {
    return entry.is_regular_file(ec) && entry.path().extension() == ".log" && entry.path().filename() != latest_log_file_name;
  };
  auto files = std::filesystem::directory_iterator{log_directory, ec} | std::views::filter(is_session_log) |
               std::views::transform([&](const auto& entry) { return std::pair{entry.last_write_time(ec), entry.path()}; }) |
               std::ranges::to<std::vector>();
  if(files.size() <= max_count)
  {
    return 0;
  }

  std::ranges::sort(files, std::ranges::greater{}, &decltype(files)::value_type::first);
  // A file that cannot be deleted is retried on the next startup.
  return static_cast<std::size_t>(std::ranges::count_if(
    files | std::views::drop(max_count), [&](const auto& file) { return std::filesystem::remove(file.second, ec); }
  ));
}

///
/// Setup logger.
///
inline auto init_log(const std::optional<std::string_view> folder_name, std::string header) -> void
{
  static constexpr auto log_dir = std::string_view("logs");
  static constexpr auto log_pattern = std::string_view("[%Y-%m-%d %H:%M:%S] [%L] [%t] %v");
  static constexpr auto max_file_size = std::size_t{5} * 1024 * 1024;
  // A session keeps its file plus this many rotated ones, e.g. `x.log`, `x.1.log`, `x.2.log`.
  static constexpr auto max_rotated_files = std::size_t{2};
  static constexpr auto max_log_files = std::size_t{20};

  const auto log_directory = system::filesystem::local_data_folder(folder_name) / log_dir;
  std::filesystem::create_directories(log_directory);
  const auto deleted_files = prune_log_files(log_directory, max_log_files - 1);

  const auto log_file = log_directory / (format_current_time_cet() + std::string{".log"});
  const auto log_file_str = to_string(log_file.u8string());

  // Rotation reopens the file, so every file starts with the header.
  auto handlers = spdlog::file_event_handlers{};
  handlers.after_open = [header = std::move(header)](const spdlog::filename_t&, std::FILE* file)
  {
    std::print(file, "{}", header);
    std::fflush(file);
  };

  const auto file_sink =
    std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file_str, max_file_size, max_rotated_files, false, handlers);
  file_sink->set_pattern(std::string{log_pattern});

  // Mirrors the session file: truncated where it rotates, cleared of the previous session on open.
  const auto latest_file = log_directory / latest_log_file_name;
  const auto latest_sink =
    std::make_shared<spdlog::sinks::rotating_file_sink_mt>(to_string(latest_file.u8string()), max_file_size, 0, true, handlers);
  latest_sink->set_pattern(std::string{log_pattern});

  const auto sinks = std::vector<spdlog::sink_ptr>{file_sink, latest_sink};
  auto logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
  logger->set_level(to_spdlog_level(global_log_level()));
  logger->flush_on(spdlog::level::info);
  spdlog::register_logger(logger);

  log_debug(std::format("Init logger: file={}, deleted_old_files={}.", log_file_str, deleted_files));
}

///
/// Access logger.
///
inline auto get_logger() -> std::shared_ptr<spdlog::logger>
{
  return spdlog::get(logger_name);
}

} // namespace

///
///
auto log_debug(std::string_view msg) -> void
{
  const auto lock = lock_logger();
  if(const auto logger = get_logger())
  {
    logger->debug(msg);
  }
}

///
///
auto log_info(std::string_view msg) -> void
{
  const auto lock = lock_logger();
  if(const auto logger = get_logger())
  {
    logger->info(msg);
  }
}

///
///
auto log_warn(std::string_view msg) -> void
{
  const auto lock = lock_logger();
  if(const auto logger = get_logger())
  {
    logger->warn(msg);
  }
}

///
///
auto log_error(std::string_view msg) -> void
{
  const auto lock = lock_logger();
  if(const auto logger = get_logger())
  {
    logger->error(msg);
  }
}

///
///
logger::logger(const std::optional<std::string_view> folder_name, std::string header)
{
  init_log(folder_name, std::move(header));
}

///
///
logger::~logger() noexcept
{
  spdlog::shutdown();
}

} // namespace bibstd::util
