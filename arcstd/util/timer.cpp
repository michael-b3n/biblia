#include "arcstd/util/timer.hpp"
#include "arcstd/util/log.hpp"
#include "arcstd/util/source_location_helpers.hpp"

namespace bibstd::util
{

///
///
timer::timer()
  : time_point_{std::chrono::system_clock::now()}
{
}

///
///
auto timer::current_duration() const -> std::chrono::milliseconds
{
  const auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now - time_point_);
}

///
///
scoped_timer_logger::scoped_timer_logger(std::source_location&& source_location)
  : source_location_{source_location}
{
}

///
///
scoped_timer_logger::~scoped_timer_logger()
{
  const auto duration = timer_.current_duration();
  LOG_DEBUG("measured scoped time duration: {} ms | {}", duration.count(), util::filter_function_name(source_location_));
}

} // namespace bibstd::util
