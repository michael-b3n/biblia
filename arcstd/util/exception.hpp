#pragma once

#include <format>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <string>

namespace arcstd::util
{

///
/// Exception base class.
///
class exception : public std::runtime_error
{
public: // Constructor and destructor
  exception(
    const std::string& error,
    const std::source_location& loc = std::source_location::current(),
    const std::stacktrace& stacktrace = std::stacktrace::current()
  );

public: // Accessors
  [[nodiscard]] inline auto where() const -> const std::source_location& { return location_; }
  [[nodiscard]] inline auto stack() const -> const std::stacktrace& { return stacktrace_; }

protected: // Variables
  const std::source_location location_;
  const std::stacktrace stacktrace_;
};

///
/// Generate a report string for the current exception,
/// including type, message, location, and stacktrace if available.
/// \return A formatted string with exception details
///
[[nodiscard]] auto exception_report() -> std::string;

} // namespace arcstd::util

///
///
template<>
struct std::formatter<arcstd::util::exception> : std::formatter<std::string>
{
  auto format(const arcstd::util::exception& ex, std::format_context& ctx) const
  {
    const auto* msg = ex.what();
    decltype(auto) loc = ex.where();
    decltype(auto) stack = ex.stack();
    return formatter<std::string>::format(
      std::format(
        "{}\n  file: {}:{}:{}\n  function: {}\n  stack: {}",
        msg,
        loc.file_name(),
        loc.line(),
        loc.column(),
        loc.function_name(),
        stack
      ),
      ctx
    );
  }
};
