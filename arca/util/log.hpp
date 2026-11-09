#pragma once

#include "arca/util/source_location_helpers.hpp"

#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>

namespace bibstd::util
{

///
/// List of all available logger levels.
///
enum class logger_level
{
  debug,
  info,
  warning,
  error
};

///
/// Get the global log level, debug messages are only logged in debug builds.
/// \return global log level
///
[[nodiscard]] constexpr auto global_log_level() -> logger_level
{
#ifdef NDEBUG
  return logger_level::info;
#else
  return logger_level::debug;
#endif
}

///
/// Log message with debug level.
///
auto log_debug(std::string_view msg) -> void;

///
/// Log message with info level.
///
auto log_info(std::string_view msg) -> void;

///
/// Log message with warning level.
///
auto log_warn(std::string_view msg) -> void;

///
/// Log message with error level.
///
auto log_error(std::string_view msg) -> void;

///
/// Init logger and shutdown logger with RAII.
/// \note Messages logged while no logger instance is alive are dropped.
///
struct logger final
{
  ///
  /// Init the logger, writing to the local data folder \p folder_name, see system::filesystem::local_data_folder.
  /// The \p header is written at the top of every log file, rotated files included.
  ///
  explicit logger(std::optional<std::string_view> folder_name = std::nullopt, std::string header = {});
  ~logger() noexcept;
};

} // namespace bibstd::util

///
/// Internal std formatted string helper macro.
///
// clang-format off
#define INT_LOG_INTERNAL_FMT_STR(LEVEL, FMT_STR, ...)                                                                                                                                                                   \
  {                                                                                                                                                                                                                     \
    if(LEVEL >= ::bibstd::util::global_log_level())                                                                                                                                                                     \
    {                                                                                                                                                                                                                   \
      static constexpr std::source_location __log_source_location = std::source_location::current();                                                                                                                    \
      static constexpr auto __log_folder_name  = ::bibstd::util::filter_folder_name(__log_source_location);                                                                                                             \
      static constexpr auto __log_file_name  = ::bibstd::util::filter_file_name(__log_source_location);                                                                                                                 \
      try                                                                                                                                                                                                               \
      {                                                                                                                                                                                                                 \
        const auto log_string = std::format("[{}::{}] " FMT_STR, __log_folder_name, __log_file_name __VA_OPT__(,) __VA_ARGS__);                                                                                         \
        if      constexpr(LEVEL == ::bibstd::util::logger_level::debug)   { ::bibstd::util::log_debug(log_string); }                                                                                                    \
        else if constexpr(LEVEL == ::bibstd::util::logger_level::info)    { ::bibstd::util::log_info(log_string);  }                                                                                                    \
        else if constexpr(LEVEL == ::bibstd::util::logger_level::warning) { ::bibstd::util::log_warn(log_string);  }                                                                                                    \
        else if constexpr(LEVEL == ::bibstd::util::logger_level::error)   { ::bibstd::util::log_error(log_string); }                                                                                                    \
      }                                                                                                                                                                                                                 \
      catch(std::format_error& exception)                                                                                                                                                                               \
      {                                                                                                                                                                                                                 \
        ::bibstd::util::log_error(std::format("[INTERNAL_FMT_STR] format error: {}.", exception.what()));                                                                                                               \
      }                                                                                                                                                                                                                 \
    }                                                                                                                                                                                                                   \
  }
// clang-format on

///
/// Default logging macros
///
// clang-format off
#define LOG_DEBUG(FMT_STR, ...) INT_LOG_INTERNAL_FMT_STR(::bibstd::util::logger_level::debug, FMT_STR __VA_OPT__(,) __VA_ARGS__);
#define LOG_INFO(FMT_STR, ...)  INT_LOG_INTERNAL_FMT_STR(::bibstd::util::logger_level::info, FMT_STR __VA_OPT__(,) __VA_ARGS__);
#define LOG_WARN(FMT_STR, ...)  INT_LOG_INTERNAL_FMT_STR(::bibstd::util::logger_level::warning, FMT_STR __VA_OPT__(,) __VA_ARGS__);
#define LOG_ERROR(FMT_STR, ...) INT_LOG_INTERNAL_FMT_STR(::bibstd::util::logger_level::error, FMT_STR __VA_OPT__(,) __VA_ARGS__);
// clang-format on
