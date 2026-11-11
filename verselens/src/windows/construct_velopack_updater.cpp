#include "src/windows/construct_velopack_updater.hpp"
#include "src/qml_application.hpp"

#include <bibqml/bridge/BridgeApplication.hpp>

#include <QObject>

namespace verselens
{

///
///
auto construct_velopack_updater(QGuiApplication& app, bridge_instance& bridge, translations_instance& translations)
  -> std::unique_ptr<app_velopack_updater>
{
  auto instance = std::make_unique<app_velopack_updater>(*bridge.bridge_application);
  QObject::connect(
    bridge.bridge_application.get(),
    &bibqml::BridgeApplication::updateCheckRequested,
    &app,
    [updater = instance.get()]() { updater->check_now(); }
  );
  QObject::connect(
    bridge.bridge_application.get(),
    &bibqml::BridgeApplication::updateRequested,
    &app,
    [&app, &bridge, &translations, updater = instance.get()]()
    {
      if(updater->install_and_restart())
      {
        quit_application(app, bridge, translations);
      }
    }
  );
  return instance;
}

} // namespace verselens
