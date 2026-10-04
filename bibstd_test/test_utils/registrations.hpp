#pragma once

#include <bibstd/lua/registration.hpp>

#include <utility>
#include <vector>

namespace bibstd::test_utils
{

///
/// Keeps the registrations of a test, so what it registers lasts as long as this object.
///
class registrations final
{
  // Variables
  std::vector<lua::registration> kept_;

public: // Operators
  ///
  /// Keep \p registration.
  /// \return true if it registered something
  ///
  auto operator()(lua::registration registration) -> bool;
};

///
///
inline auto registrations::operator()(lua::registration registration) -> bool
{
  kept_.push_back(std::move(registration));
  return static_cast<bool>(kept_.back());
}

} // namespace bibstd::test_utils
