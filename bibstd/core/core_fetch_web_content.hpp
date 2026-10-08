#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace bibstd::core
{

///
/// Core web content fetcher. This class uses libcurl to fetch content from a webpage.
///
class core_fetch_web_content final
{
public: // Typedefs
  ///
  /// Error codes for web content fetching operations.
  ///
  enum class error_code
  {
    init_failed,    ///< Failed to initialize curl
    request_failed, ///< HTTP request failed
    not_found,      ///< HTTP 404 or 410, the server has no such page
    invalid_url,    ///< Invalid URL provided
    timeout,        ///< Request timed out
    unknown         ///< Unknown error occurred
  };

public: // Structors
  core_fetch_web_content() = default;
  ~core_fetch_web_content() noexcept = default;

public: // Operations
  ///
  /// \return Content of the webpage, or an error code
  ///
  static auto fetch(std::string_view url) -> std::expected<std::string, error_code>;
};

} // namespace bibstd::core
