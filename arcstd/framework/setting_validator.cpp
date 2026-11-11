#include "arcstd/framework/setting_validator.hpp"

namespace arcstd::framework::detail
{

///
///
auto setting_validator_connector::connect_on_changed(std::function<void()> on_changed) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  on_changed_ = std::move(on_changed);
}

///
///
auto setting_validator_connector::notify_changed() -> void
{
  const auto on_changed = [&]
  {
    const auto lock = std::scoped_lock{mtx_};
    return on_changed_;
  }();
  if(on_changed)
  {
    on_changed();
  }
}

} // namespace arcstd::framework::detail
