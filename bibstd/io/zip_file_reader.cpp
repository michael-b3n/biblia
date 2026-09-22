#include "bibstd/io/zip_file_reader.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/ranges.hpp"

#include <zip.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <utility>

namespace bibstd::io
{
namespace
{

// Flags for reading original state
constexpr zip_flags_t unchanged_flag = ZIP_FL_UNCHANGED;

///
/// Convert libzip compression method to our enum.
///
auto convert_compression_method(std::uint16_t libzip_method) -> zip_file_reader::compression_method
{
  using compression_method = zip_file_reader::compression_method;
  switch(libzip_method)
  {
  case ZIP_CM_STORE: return compression_method::store;
  case ZIP_CM_DEFLATE: return compression_method::deflate;
#ifdef ZIP_CM_BZIP2
  case ZIP_CM_BZIP2: return compression_method::bzip2;
#endif
#ifdef ZIP_CM_LZMA
  case ZIP_CM_LZMA: return compression_method::lzma;
#endif
#ifdef ZIP_CM_ZSTD
  case ZIP_CM_ZSTD: return compression_method::zstd;
#endif
#ifdef ZIP_CM_XZ
  case ZIP_CM_XZ: return compression_method::xz;
#endif
  default: return compression_method::default_method;
  }
}

///
/// Convert libzip encryption method to our enum.
///
auto convert_encryption_method(std::uint16_t libzip_method) -> zip_file_reader::encryption_method
{
  using encryption_method = zip_file_reader::encryption_method;
  switch(libzip_method)
  {
  case ZIP_EM_NONE: return encryption_method::none;
  case ZIP_EM_TRAD_PKWARE: return encryption_method::trad_pkware;
#ifdef ZIP_EM_AES_128
  case ZIP_EM_AES_128: return encryption_method::aes_128;
#endif
#ifdef ZIP_EM_AES_192
  case ZIP_EM_AES_192: return encryption_method::aes_192;
#endif
#ifdef ZIP_EM_AES_256
  case ZIP_EM_AES_256: return encryption_method::aes_256;
#endif
  default: return encryption_method::none;
  }
}

} // anonymous namespace

///
///
zip_file_reader::zip_file_reader(const std::filesystem::path& zip_path, const std::string_view password)
  : password_{password}
  , zip_handle_{nullptr}
{
  int error_code = 0;
#ifndef BIBSTD_DEBUG
  const int flags = ZIP_RDONLY;
#else
  const int flags = ZIP_RDONLY | ZIP_CHECKCONS;
#endif
  zip_handle_ = zip_open(zip_path.string().c_str(), flags, &error_code);
  if(zip_handle_ == nullptr)
  {
    const auto msg = [&]() -> std::string_view
    {
      switch(error_code)
      {
        // clang-format off
      case ZIP_ER_EXISTS: return "The file specified by path exists and ZIP_EXCL is set.";
      case ZIP_ER_INCONS: return "Inconsistencies were found in the file specified by path. This error is often caused by specifying ZIP_CHECKCONS but can also happen without it.";
      case ZIP_ER_INVAL: return "The path argument is NULL.";
      case ZIP_ER_MEMORY: return "Required memory could not be allocated.";
      case ZIP_ER_NOENT: return "The file specified by path does not exist and ZIP_CREATE is not set.";
      case ZIP_ER_NOZIP: return "The file specified by path is not a zip archive.";
      case ZIP_ER_OPEN: return "The file specified by path could not be opened.";
      case ZIP_ER_READ: return "A read error occurred; see errno for details.";
      case ZIP_ER_SEEK: return "The file specified by path does not allow seeks.";
      default: return "Unknown error.";
        // clang-format on
      }
    }();
    LOG_ERROR("failed to open zip archive: path=\"{}\", error=\"{}\"", zip_path.string(), msg);
  }
  else if(!password_.empty())
  {
    zip_set_default_password(zip_handle_, password_.c_str());
  }
}

///
///
zip_file_reader::zip_file_reader(const std::span<const std::byte> data, const std::string_view password)
  : password_{password}
  , zip_handle_{nullptr}
{
  zip_error_t error;
  auto* source = zip_source_buffer_create(data.data(), static_cast<zip_uint64_t>(data.size()), 0, &error);
  if(source == nullptr)
  {
    const auto* const msg = zip_error_strerror(&error);
    LOG_ERROR("failed to create zip source from memory buffer: error=\"{}\"", msg);
    zip_error_fini(&error);
    return;
  }
#ifndef BIBSTD_DEBUG
  const int flags = ZIP_RDONLY;
#else
  const int flags = ZIP_RDONLY | ZIP_CHECKCONS;
#endif
  zip_handle_ = zip_open_from_source(source, flags, &error); // takes ownership of source
  if(zip_handle_ == nullptr)
  {
    const auto* const msg = zip_error_strerror(&error);
    LOG_ERROR("failed to open zip archive from memory buffer: error=\"{}\"", msg);
    zip_error_fini(&error);
  }
  else if(!password_.empty())
  {
    zip_set_default_password(zip_handle_, password_.c_str());
  }
}

///
///
zip_file_reader::~zip_file_reader() noexcept
{
  if(zip_handle_ != nullptr)
  {
    zip_discard(zip_handle_);
    zip_handle_ = nullptr;
  }
}

///
///
auto zip_file_reader::is_open() const -> bool
{
  return zip_handle_ != nullptr;
}

///
///
auto zip_file_reader::entry_count() const -> std::size_t
{
  // The flag ZIP_FL_UNCHANGED is used since we guarantee
  // to open the archive in read only mode.
  return is_open() ? zip_get_num_entries(zip_handle_, unchanged_flag) : 0;
}

///
///
auto zip_file_reader::comment() const -> std::string
{
  if(is_open())
  {
    int length = 0;
    const auto* const c = zip_get_archive_comment(zip_handle_, &length, unchanged_flag);
    if((c != nullptr) && length > 0)
    {
      return std::string(c, static_cast<std::size_t>(length));
    }
  }
  return {};
}

///
///
auto zip_file_reader::entries() const -> std::vector<zip_entry>
{
  if(!is_open())
  {
    return {};
  }

  auto result = std::vector<zip_entry>(entry_count());
  zip_stat_t stat;
  zip_stat_init(&stat);

  std::ranges::for_each(
    util::ranges::index_view(result),
    [&](const auto i)
    {
      if(zip_stat_index(zip_handle_, static_cast<zip_uint64_t>(i), unchanged_flag, &stat) == 0)
      {
        result.at(i) = create_entry(stat);
      }
      else
      {
        LOG_ERROR("failed to get zip entry info: index={}, error=\"{}\"", i, error_message());
      }
    }
  );
  return result;
}

///
///
auto zip_file_reader::entry(const std::size_t index) const -> std::optional<zip_entry>
{
  return entry_by_index(index, unchanged_flag);
}

///
///
auto zip_file_reader::entry(const std::string& name, const query_flags_type flags) const -> std::optional<zip_entry>
{
  const auto index_optional = index_of_entry(name, flags);
  if(index_optional)
  {
    return entry_by_index(*index_optional, unchanged_flag);
  }
  return {};
}

///
///
auto zip_file_reader::read_entry(const zip_entry& entry) const -> std::vector<std::byte>
{
  if(!is_open() || !entry.index.has_value() || !entry.size.has_value())
  {
    return {};
  }

  if(auto* const file = zip_fopen_index(zip_handle_, static_cast<zip_uint64_t>(entry.index.value()), unchanged_flag))
  {
    const auto size = entry.size.value();
    std::vector<std::byte> buffer(size);
    if(size > 0)
    {
      const auto bytes_read = zip_fread(file, buffer.data(), size);
      if(bytes_read < 0 || std::cmp_not_equal(bytes_read, size))
      {
        LOG_WARN("file reading failed or incomplete: index={}, name=\"{}\"", *entry.index, entry.name.value_or("unknown"));
      }
    }
    zip_fclose(file);
    return buffer;
  }
  else
  {
    LOG_ERROR("failed to open zip file: index={}, error=\"{}\"", *entry.index, error_message());
  }
  return {};
}

///
///
auto zip_file_reader::read_entry_as_string(const zip_entry& entry) const -> std::string
{
  const auto data = read_entry(entry);
  if(data.empty())
  {
    return {};
  }
  return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

///
///
auto zip_file_reader::index_of_entry(const std::string& name, const query_flags_type flags) const -> std::optional<std::size_t>
{
  if(!is_open())
  {
    return {};
  }
  std::uint32_t zip_flags = unchanged_flag;
  if(flags.has(query_flag::exclude_directories))
  {
    zip_flags |= ZIP_FL_NODIR;
  }
  if(flags.has(query_flag::case_insensitive))
  {
    zip_flags |= ZIP_FL_NOCASE;
  }
  const auto n = std::string{name};
  const auto index = zip_name_locate(zip_handle_, n.c_str(), zip_flags);
  return index < 0 ? std::nullopt : std::make_optional(static_cast<std::size_t>(index));
}

///
///
auto zip_file_reader::entry_by_index(const std::size_t index, const std::uint32_t flags) const -> std::optional<zip_entry>
{
  auto result = std::optional<zip_entry>{};
  if(!is_open() || index >= entry_count())
  {
    return result;
  }

  zip_stat_t stat;
  zip_stat_init(&stat);
  if(zip_stat_index(zip_handle_, static_cast<zip_uint64_t>(index), flags, &stat) == 0)
  {
    result = create_entry(stat);
  }
  else
  {
    LOG_ERROR("failed to get zip entry info: index={}, error=\"{}\"", index, error_message());
  }
  return result;
}

///
///
auto zip_file_reader::create_entry(const zip_stat_t& stat) const -> zip_entry
{
  const auto valid = [&](const std::uint64_t flag) -> bool { return stat.valid & flag; };
  return zip_entry{
    .comment = [&]() -> std::string
    {
      if(is_open())
      {
        zip_uint32_t length = 0;
        const auto* const c = zip_file_get_comment(zip_handle_, stat.index, &length, unchanged_flag);
        if(c && length >= 0)
        {
          return std::string(c, static_cast<std::size_t>(length));
        }
        else
        {
          const auto* const name = valid(ZIP_STAT_NAME) && stat.name ? stat.name : "unknown";
          LOG_ERROR("failed to get comment for zip entry: name=\"{}\", error=\"{}\"", name, error_message());
        }
      }
      return {};
    }(),
    // clang-format off
    .name = valid(ZIP_STAT_NAME) ? ((stat.name != nullptr) ? std::string{stat.name} : "") : decltype(zip_entry::name){},
    .index = valid(ZIP_STAT_INDEX) ? static_cast<std::uint64_t>(stat.index) : decltype(zip_entry::index){},
    .compression = valid(ZIP_STAT_COMP_METHOD) ? convert_compression_method(stat.comp_method) : decltype(zip_entry::compression){},
    .encryption = valid(ZIP_STAT_ENCRYPTION_METHOD) ? convert_encryption_method(stat.encryption_method) : decltype(zip_entry::encryption){},
    .size = valid(ZIP_STAT_SIZE) ? static_cast<std::uint64_t>(stat.size) : decltype(zip_entry::size){},
    .compressed_size = valid(ZIP_STAT_COMP_SIZE) ? static_cast<std::uint64_t>(stat.comp_size) : decltype(zip_entry::compressed_size){},
    .crc = valid(ZIP_STAT_CRC) ? static_cast<std::uint32_t>(stat.crc) : decltype(zip_entry::crc){}
    // clang-format on
  };
}

///
///
auto zip_file_reader::error_message() const -> std::string
{
  if(!is_open())
  {
    return std::string{"archive not open"};
  }
  else if(auto* const error = zip_get_error(zip_handle_))
  {
    const auto* const strerror = zip_error_strerror(error);
    if(strerror != nullptr)
    {
      return std::string(strerror);
    }
  }
  return std::string{"unknown error"};
}

} // namespace bibstd::io
