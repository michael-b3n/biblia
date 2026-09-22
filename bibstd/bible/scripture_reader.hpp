#pragma once

#include "bibstd/bible/scripture.hpp"

#include <memory>
#include <string>

// Forward declarations
namespace bibstd::io
{
class zip_file_reader;
} // namespace bibstd::io

namespace bibstd::bible
{

///
/// Reads one scripture format out of an archive. Which archive a scripture ships in and which format
/// it holds are two separate questions: the archive is opened by its file extension, the format is
/// found by asking every reader whether it recognizes the opened archive.
///
class scripture_reader
{
public: // Typedefs
  using name_type = std::string;

public: // Constants
  ///
  /// Substituted for information the scripture does not carry, so that a scripture that was read
  /// always has a complete set of it.
  ///
  static constexpr auto unknown_name = "Unknown Scripture";
  static constexpr auto unknown_abbreviation = "Unknown Abbreviation";
  static constexpr auto unknown_language = "Unknown Language";

public: // Structors
  virtual ~scripture_reader() noexcept = default;

public: // Accessors
  ///
  /// Access to the scripture format name, for diagnostics.
  /// \return format name
  ///
  virtual auto name() const -> name_type = 0;

  ///
  /// Cheap look at the archive: does it hold this reader's format at all? Nothing is logged, an
  /// archive this reader does not recognize is simply another reader's.
  /// \return true if the archive holds this reader's format, false otherwise
  ///
  virtual auto recognizes(const io::zip_file_reader& archive) const -> bool = 0;

  ///
  /// Read the scripture out of the archive. Only called once recognizes said yes, so a nullptr
  /// result means a damaged file of a format that was identified, not a wrong guess.
  /// \return The scripture, or nullptr if the archive holds this format but cannot be read
  ///
  virtual auto read(const io::zip_file_reader& archive) const -> std::unique_ptr<scripture> = 0;
};

} // namespace bibstd::bible
