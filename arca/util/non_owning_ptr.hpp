#pragma once

#include <type_traits>

namespace bibstd::util
{

///
/// Pointer type wrapper to indicate, that the pointer is not owned.
///
template<typename T>
  requires(!std::is_pointer_v<T>)
using non_owning_ptr = T*;

} // namespace bibstd::util
