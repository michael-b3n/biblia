#pragma once

#include "bibstd/util/non_owning_ptr.hpp"
#include "bibstd/util/uid.hpp"

#include <expected>

namespace bibstd::framework
{

///
/// Unique runtime process ID.
///
using process_id_type = util::uid<struct process_id_tag>;

///
/// Enum listing unexpected process results.
///
enum class process_result_unexpected
{
  failure,
  stopped
};

///
/// Helper type for process results.
///
template<typename T>
using process_result = std::expected<T, process_result_unexpected>;

///
/// Parameters container for processes.
///
template<typename ParamsType>
class process_params;

///
/// template specialization \see process_params
///
template<>
class process_params<void> final
{
  // Variables
  process_id_type process_id_;

public: // Typedefs
  using value_type = void;
  using id_type = process_id_type;

public: // Constructor
  process_params() = default;

public: // Accessors
  ///
  /// Access process ID.
  /// \return process ID
  ///
  [[nodiscard]] auto process_id() const -> id_type { return process_id_; }
};

///
/// template specialization \see process_params
///
template<typename ParamsType>
  requires(!std::is_void_v<ParamsType>)
class process_params<ParamsType> final
{
  // Variables
  process_id_type process_id_;
  ParamsType params_;

public: // Typedefs
  using value_type = ParamsType;
  using id_type = process_id_type;

public: // Constructor
  process_params(const ParamsType& params)
    : params_{params}
  {
  }
  process_params(ParamsType&& params)
    : params_{std::forward<ParamsType>(params)}
  {
  }

public: // Accessors
  ///
  /// Access process ID.
  /// \return process ID
  ///
  [[nodiscard]] auto process_id() const -> id_type { return process_id_; }

  ///
  /// Access process_params_start parameters.
  /// \return process_params_start parameters
  ///
  [[nodiscard]] auto operator->() const -> util::non_owning_ptr<const ParamsType> { return &params_; }
};

} // namespace bibstd::framework
