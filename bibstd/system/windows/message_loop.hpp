#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <latch>
#include <thread>

namespace bibstd::system
{

///
/// Thread running a Windows message loop, for what Windows binds to a thread: a registered
/// hotkey, the window of a tray icon. Work is handed to that thread as messages to its window.
///
class message_loop final
{
  // Typedefs
  struct access;

  // Variables
  std::move_only_function<void(unsigned int, std::uintptr_t)> handler_;
  std::latch ready_{1};
  std::atomic<void*> window_{nullptr};
  std::jthread thread_; // last, it uses the members above and joins first

public: // Typedefs
  using handler_type = decltype(handler_);
  using task_type = std::move_only_function<void()>;

public: // Structors
  ///
  /// \param handler called on the loop thread with message and WPARAM of what is sent to window()
  ///
  explicit message_loop(handler_type&& handler = {});

  ///
  /// Ends the loop once the tasks posted so far have run.
  ///
  ~message_loop() noexcept;

public: // Accessors
  ///
  /// Get the message-only window of the loop.
  /// \return HWND of the window
  ///
  [[nodiscard]] auto window() const noexcept -> void*;

public: // Modifiers
  ///
  /// Run \p task on the loop thread, after the tasks posted before. An exception it throws is logged.
  ///
  auto post(task_type&& task) -> void;

  ///
  /// Run \p task on the loop thread and wait for it. An exception it throws is thrown here.
  ///
  auto call(const std::function<void()>& task) -> void;

private: // Implementation
  auto run() noexcept -> void;
};

} // namespace bibstd::system
