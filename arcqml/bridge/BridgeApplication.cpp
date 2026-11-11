#include "arcqml/bridge/BridgeApplication.hpp"

#include <QMetaObject>

namespace arcqml
{

///
///
BridgeApplication::BridgeApplication(const arcstd::util::non_owning_ptr<QObject> parent)
  : QObject{parent}
{
}

///
///
BridgeApplication::~BridgeApplication() noexcept = default;

///
///
bool BridgeApplication::updateAvailable() const
{
  return updateAvailable_;
}

///
///
BridgeApplication::UpdateCheck BridgeApplication::updateCheck() const
{
  return updateCheck_;
}

///
///
void BridgeApplication::requestShowWindow()
{
  QMetaObject::invokeMethod(this, [this]() { emit showWindowRequested(); }, Qt::QueuedConnection);
}

///
///
void BridgeApplication::notifyUpdateAvailable()
{
  QMetaObject::invokeMethod(
    this,
    [this]()
    {
      if(!updateAvailable_)
      {
        updateAvailable_ = true;
        emit updateAvailableChanged();
      }
    },
    Qt::QueuedConnection
  );
}

///
///
void BridgeApplication::notifyUpdateCheck(const UpdateCheck state)
{
  QMetaObject::invokeMethod(
    this,
    [this, state]()
    {
      if(updateCheck_ != state)
      {
        updateCheck_ = state;
        emit updateCheckChanged();
      }
    },
    Qt::QueuedConnection
  );
}

///
///
void BridgeApplication::requestUpdateCheck()
{
  emit updateCheckRequested();
}

///
///
void BridgeApplication::requestUpdate()
{
  emit updateRequested();
}

} // namespace arcqml
