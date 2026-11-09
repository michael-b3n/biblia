#include "arca/util/scope_guard.hpp"
#include "arca/util/exception.hpp"
#include "arca/util/log.hpp"

namespace bibstd::util
{

///
///
scope_guard::scope_guard(std::move_only_function<void()>&& on_destruction)
  : on_destruction_{std::forward<decltype(on_destruction)>(on_destruction)}
{
}

///
///
scope_guard::~scope_guard() noexcept
{
  destruct();
}

///
///
scope_guard::scope_guard(scope_guard&& rhs) noexcept
  : on_destruction_{std::move(rhs.on_destruction_)}
{
  rhs.on_destruction_ = nullptr;
}

///
///
auto scope_guard::operator=(scope_guard&& rhs) & noexcept -> scope_guard&
{
  destruct();
  on_destruction_ = std::move(rhs.on_destruction_);
  rhs.on_destruction_ = nullptr;
  return *this;
}

///
///
auto scope_guard::reset() -> void
{
  destruct();
}

///
///
auto scope_guard::destruct() -> void
{
  try
  {
    if(on_destruction_)
    {
      on_destruction_();
      on_destruction_ = nullptr;
    }
  }
  catch(...)
  {
    LOG_ERROR("scoped guard destruction exception: {}", util::exception_report());
  }
}

///
///
auto shared_scope_guard::creator::create([[maybe_unused]] scope_guard::on_destruction_type&& on_destruction)
  -> shared_scope_guard
{
  auto is_initial_instance = false;
  auto instance = instance_.lock();
  if(!instance)
  {
    instance = std::make_shared<scope_guard>(std::move(on_destruction));
    instance_ = instance;
    is_initial_instance = true;
  }
  return {scope_guard([instance]() mutable { instance.reset(); }), is_initial_instance};
}

///
///
auto shared_scope_guard::is_initial_instance() const -> bool
{
  return is_initial_instance_;
}

///
///
auto shared_scope_guard::reset() -> void
{
  instance_guard_.reset();
}

///
///
shared_scope_guard::shared_scope_guard(scope_guard instance_guard, const bool is_initial_instance)
  : is_initial_instance_{is_initial_instance}
  , instance_guard_{std::move(instance_guard)}
{
}

} // namespace bibstd::util
