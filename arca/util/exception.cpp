#include "arca/util/exception.hpp"

namespace bibstd::util
{

///
///
auto exception_report() -> std::string
{
  decltype(auto) ex = std::current_exception();
  try
  {
    if(!ex)
    {
      throw exception{"exception_report called with no active exception"};
    }
    std::rethrow_exception(ex);
  }
  catch(const exception& e)
  {
    return std::format("{}", e);
  }
  catch(const std::exception& e)
  {
    return std::format("{}", e.what());
  }
  catch(...)
  {
    return std::string{"unknown exception"};
  }
}

///
///
exception::exception(const std::string& error, const std::source_location& loc, const std::stacktrace& stacktrace)
  : std::runtime_error{error}
  , location_{loc}
  , stacktrace_{stacktrace}
{
}

} // namespace bibstd::util
