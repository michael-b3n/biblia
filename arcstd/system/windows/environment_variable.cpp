#include "arcstd/system/environment_variable.hpp"
#include "arcstd/util/log.hpp"

#include "arcstd/system/windows/win.hpp"

namespace bibstd::system
{

///
///
auto environment_variable::set(const std::string& name, const std::string& value) -> bool
{
  if(name.empty())
  {
    return false;
  }
  const bool result = SetEnvironmentVariableA(name.c_str(), value.empty() ? nullptr : value.c_str()) != 0;
  LOG_INFO("set environment variable {}: name=\"{}\", value=\"{}\"", result ? "succeeded" : "failed", name, value);
  return result;
}

} // namespace bibstd::system
