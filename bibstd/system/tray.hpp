#pragma once

#include "bibstd/execution/scope.hpp"
#include "bibstd/system/windows/message_loop.hpp"
#include "bibstd/util/scope_guard.hpp"

#include <tray.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace bibstd::system
{

///
/// Tray class for windows implementation.
///
class tray final
{
  // Variables
  inline static std::mutex mtx_;
  inline static bool active_{false};
  inline static std::unique_ptr<message_loop> loop_{};
  inline static std::unique_ptr<execution::scope> scope_{};
  // The tray window belongs to the loop thread, the tray is only used there
  inline static std::unique_ptr<Tray::Tray> tray_{nullptr};

public: // Typedefs
  ///
  /// Icon file loaded into memory, the buffer views a `*.ico` file.
  ///
  struct icon_buffer final
  {
    std::span<const std::byte> buffer;
  };

  // clang-format off
  struct button final { std::string text; std::function<void()> callback; };
  struct label final { std::string text; };
  struct separator final {};
  struct toggle final { std::string text; bool state; std::function<std::function<void()>(bool)> callback; };
  // clang-format on

  using entry_type = std::variant<button, label, separator, toggle>;

public: // Static modifiers
  static auto init(icon_buffer icon, std::vector<entry_type>&& entries) -> util::shared_scope_guard;

  ///
  /// Change the text of an entry, e.g. after a language change. Ignored if no tray exists.
  /// \p index is the position in the list the tray was initialized with, \p text is UTF-8 encoded.
  ///
  static auto set_text(std::size_t index, std::string text) -> void;
};

} // namespace bibstd::system
