#include "bibqml/style/Metrics.hpp"

namespace bibqml
{

///
///
Metrics::Metrics(bibstd::util::non_owning_ptr<QObject> parent)
  : QObject{parent}
{
}

///
///
Metrics::~Metrics() noexcept = default;

} // namespace bibqml
