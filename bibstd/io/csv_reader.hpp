#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations
namespace rapidcsv
{
class Document;
} // namespace rapidcsv

namespace bibstd::io
{

///
/// Reader for CSV documents. Provides read-only access to the cells of a CSV document.
/// This class wraps rapidcsv for simple CSV read operations. All cells are provided as
/// strings, conversion to other types is up to the caller.
/// \note The complete document is held in memory.
///
class csv_reader final
{
  // Variables
  std::unique_ptr<rapidcsv::Document> document_;

public: // Typedefs
  ///
  /// Parameters describing the layout of the CSV document.
  ///
  struct params final
  {
    char separator{','};            // cell separator character
    bool header{true};              // if true, the first row contains the column names
    bool trim{false};               // if true, leading and trailing whitespace of a cell is removed
    bool skip_comment_lines{false}; // if true, lines starting with `comment_prefix` are ignored
    bool skip_empty_lines{false};   // if true, empty lines are ignored
  };

public: // Constants
  ///
  /// Prefix marking a comment line. \see params::skip_comment_lines
  ///
  static constexpr char comment_prefix = '#';

public: // Structors
  ///
  /// Construct a CSV reader from in-memory data. Intended for compiled in resources.
  /// Parameters describing the document layout must be provided.
  /// \throws util::exception if the data cannot be parsed
  ///
  csv_reader(std::span<const std::byte> data, const params& p);

  ///
  /// Construct a CSV reader for the specified file path with parameters describing the document layout.
  /// \throws util::exception if the file cannot be read or parsed
  ///
  csv_reader(const std::filesystem::path& csv_path, const params& p);

  ~csv_reader() noexcept;
  csv_reader(const csv_reader&) = delete;
  csv_reader(csv_reader&&) noexcept;

public: // Operators
  auto operator=(const csv_reader&) -> csv_reader& = delete;
  auto operator=(csv_reader&&) noexcept -> csv_reader&;

public: // Accessors
  ///
  /// Get the number of data rows. The header row is not counted.
  /// \return number of rows
  ///
  [[nodiscard]] auto row_count() const -> std::size_t;

  ///
  /// Get the number of columns.
  /// \return number of columns
  ///
  [[nodiscard]] auto column_count() const -> std::size_t;

  ///
  /// Get the names of all columns as defined by the header row.
  /// \return list of column names, empty if the document has no header row
  ///
  [[nodiscard]] auto column_names() const -> std::vector<std::string>;

  ///
  /// Get the index of the column with the specified name.
  /// \return column index if found, std::nullopt otherwise
  ///
  [[nodiscard]] auto column_index(std::string_view name) const -> std::optional<std::size_t>;

  ///
  /// Get the cell at the specified column and row.
  /// \return cell value if column and row exist, std::nullopt otherwise
  ///
  [[nodiscard]] auto cell(std::size_t column, std::size_t row) const -> std::optional<std::string>;

  ///
  /// Get the cell of the named column at the specified row.
  /// \return cell value if column and row exist, std::nullopt otherwise
  ///
  [[nodiscard]] auto cell(std::string_view column, std::size_t row) const -> std::optional<std::string>;

  ///
  /// Get all cells of the row at the specified index.
  /// \return cells of the row, empty if the row does not exist
  ///
  [[nodiscard]] auto row(std::size_t row) const -> std::vector<std::string>;

  ///
  /// Get all cells of the named column.
  /// \return cells of the column, empty if the column does not exist
  ///
  [[nodiscard]] auto column(std::string_view column) const -> std::vector<std::string>;
};

} // namespace bibstd::io
