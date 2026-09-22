#include "bibstd/io/csv_reader.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/ranges.hpp"

#include <algorithm>
#include <rapidcsv.h>

#include <format>
#include <sstream>

namespace bibstd::io
{
namespace
{

///
/// Create rapidcsv label params from the csv reader params.
/// \return rapidcsv label params
///
auto to_label_params(const csv_reader::params& p) -> rapidcsv::LabelParams
{
  // The second parameter denotes the column holding the row labels, which is not supported.
  return rapidcsv::LabelParams{p.header ? 0 : -1, -1};
}

///
/// Create rapidcsv separator params from the csv reader params.
/// \return rapidcsv separator params
///
auto to_separator_params(const csv_reader::params& p) -> rapidcsv::SeparatorParams
{
  return rapidcsv::SeparatorParams{p.separator, p.trim};
}

///
/// Create rapidcsv line reader params from the csv reader params.
/// \return rapidcsv line reader params
///
auto to_line_reader_params(const csv_reader::params& p) -> rapidcsv::LineReaderParams
{
  return rapidcsv::LineReaderParams{p.skip_comment_lines, csv_reader::comment_prefix, p.skip_empty_lines};
}

} // anonymous namespace

///
///
csv_reader::csv_reader(const std::span<const std::byte> data, const params& p)
{
  auto stream = std::istringstream{
    std::string{reinterpret_cast<const char*>(data.data()), data.size()},
    std::ios::binary
  };
  try
  {
    document_ = std::make_unique<rapidcsv::Document>(
      stream, to_label_params(p), to_separator_params(p), rapidcsv::ConverterParams{}, to_line_reader_params(p)
    );
  }
  catch(const std::exception& e)
  {
    throw util::exception{std::format("parse csv data failed: size={}, what=\"{}\"", data.size(), e.what())};
  }
}

///
///
csv_reader::csv_reader(const std::filesystem::path& csv_path, const params& p)
{
  try
  {
    document_ = std::make_unique<rapidcsv::Document>(
      csv_path.string(), to_label_params(p), to_separator_params(p), rapidcsv::ConverterParams{}, to_line_reader_params(p)
    );
  }
  catch(const std::exception& e)
  {
    throw util::exception{std::format(R"(read csv file failed: path="{}", what="{}")", csv_path.string(), e.what())};
  }
}

///
///
csv_reader::~csv_reader() noexcept = default;

///
///
csv_reader::csv_reader(csv_reader&&) noexcept = default;

///
///
auto csv_reader::operator=(csv_reader&&) noexcept -> csv_reader& = default;

///
///
auto csv_reader::row_count() const -> std::size_t
{
  return document_->GetRowCount();
}

///
///
auto csv_reader::column_count() const -> std::size_t
{
  return document_->GetColumnCount();
}

///
///
auto csv_reader::column_names() const -> std::vector<std::string>
{
  return document_->GetColumnNames();
}

///
///
auto csv_reader::column_index(const std::string_view name) const -> std::optional<std::size_t>
{
  const auto index = document_->GetColumnIdx(std::string{name});
  if(index < 0)
  {
    return std::nullopt;
  }
  return static_cast<std::size_t>(index);
}

///
///
auto csv_reader::cell(const std::size_t column, const std::size_t row) const -> std::optional<std::string>
{
  if(column >= column_count() || row >= row_count())
  {
    return std::nullopt;
  }
  try
  {
    return document_->GetCell<std::string>(column, row);
  }
  catch(...)
  {
    // The cell is not part of the document, e.g. if the row holds less cells than the header row.
    LOG_DEBUG("read csv cell failed: column={}, row={}, what=\"{}\"", column, row, util::exception_report());
    return std::nullopt;
  }
}

///
///
auto csv_reader::cell(const std::string_view column, const std::size_t row) const -> std::optional<std::string>
{
  const auto index = column_index(column);
  return index ? cell(*index, row) : std::nullopt;
}

///
///
auto csv_reader::row(const std::size_t row) const -> std::vector<std::string>
{
  if(row >= row_count())
  {
    return {};
  }
  const auto columns = column_count();
  auto result = std::vector<std::string>{};
  result.reserve(columns);
  std::ranges::for_each(
    util::ranges::index_view_to(columns),
    [this, &row, &result](const auto column) { result.emplace_back(cell(column, row).value_or(std::string{})); }
  );
  return result;
}

///
///
auto csv_reader::column(const std::string_view column) const -> std::vector<std::string>
{
  const auto index = column_index(column);
  if(!index)
  {
    return {};
  }
  const auto rows = row_count();
  auto result = std::vector<std::string>{};
  result.reserve(rows);
  std::ranges::for_each(
    util::ranges::index_view_to(rows),
    [this, &index, &result](const auto row) { result.emplace_back(cell(*index, row).value_or(std::string{})); }
  );
  return result;
}

} // namespace bibstd::io
