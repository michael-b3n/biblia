#include "arcqml/style/Metrics.hpp"

namespace arcqml
{

///
///
Metrics::Metrics(arcstd::util::non_owning_ptr<QObject> parent)
  : QObject{parent}
{
}

///
///
Metrics::~Metrics() noexcept = default;

} // namespace arcqml
