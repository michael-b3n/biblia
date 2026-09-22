#pragma once

#define INCBIN_STYLE  INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX res_
extern "C"
{
#include "incbin.h"
}
#include <cstddef>
#include <span>

///
/// Usage: INC_RESOURCE(LABEL, FILE)
///
/// Symbols defined by INCBIN:
/// `const std::byte res_{LABEL}_data[];`
/// `const std::byte* const res_{LABEL}_end;`
/// `const unsigned int res_{LABEL}_size;`
///
#define INC_RESOURCE(LABEL, FILE) INCBIN(std::byte, LABEL, FILE)

namespace bibstd::util::incbin
{

///
/// Reinterpret data pointer and size with span of given type `T`.
/// \tparam T value_type of span
/// \return `std::span<T>` on data
///
template<typename T>
inline auto to_span(const std::byte* const data, const unsigned int size) -> std::span<const T>
{
  return std::span(reinterpret_cast<const T* const>(data), size / sizeof(T));
}

} // namespace bibstd::util::incbin
