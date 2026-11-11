#include "arcqml/bridge/BridgeLogger.hpp"

#include <arcstd/util/log.hpp>

namespace bibqml
{

///
///
BridgeLogger::BridgeLogger(const bibstd::util::non_owning_ptr<QObject> parent)
  : QObject{parent}
{
}

///
///
void BridgeLogger::debug(const QString& message) const
{
  LOG_DEBUG("QML: {}", message.toStdString());
}

///
///
void BridgeLogger::info(const QString& message) const
{
  LOG_INFO("QML: {}", message.toStdString());
}

///
///
///
void BridgeLogger::warning(const QString& message) const
{
  LOG_WARN("QML: {}", message.toStdString());
}

///
///
void BridgeLogger::error(const QString& message) const
{
  LOG_ERROR("QML: {}", message.toStdString());
}

} // namespace bibqml
