#pragma once

#include <boost/preprocessor/cat.hpp>

#include <chrono>
#include <source_location>

namespace arcstd::util
{

///
/// Timer class used for measuring time durations.
///
class timer final
{
  // Variables
  std::chrono::system_clock::time_point time_point_;

public: // Constructor
  timer();

public: // Accessors
  ///
  /// Get current duration from constructed timepoint to timepoint of the call of this function.
  /// \return current duration
  ///
  [[nodiscard]] auto current_duration() const -> std::chrono::milliseconds;
};

///
/// Scoped timer logging duration on destruction.
///
class scoped_timer_logger final
{
  // Variables
  const std::source_location source_location_;
  const timer timer_;

public: // Destructor
  scoped_timer_logger(std::source_location&& source_location = std::source_location::current());
  ~scoped_timer_logger();
};

} // namespace arcstd::util

///
/// Scoped timer logging macro.
///
#define SCOPED_TIMER_LOG()                                                                                                     \
  [[maybe_unused]] const auto BOOST_PP_CAT(__scoped_timer_logger_instance, __LINE__) = ::arcstd::util::scoped_timer_logger();
