#pragma once

#include "arcstd/meta/pack.hpp"

namespace arcstd::meta
{

///
/// Checks if pack like type P contains a type T
/// \tparam P pack like type
/// \tparam T generic type
/// \result true, if T is contained in P, false otherwise
///
template<packaged P, typename T>
constexpr bool contains_v = type_index_v<P, T> < pack_info<P>::size;

} // namespace arcstd::meta
