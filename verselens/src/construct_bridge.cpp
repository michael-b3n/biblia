#include "src/construct_bridge.hpp"
#include "src/qml_application.hpp"

#include <bibstd/workflow/workflow_bible_ref_lookup.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr_auto.hpp>
#include <bibstd/workflow/workflow_hotkey.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>
#include <bibstd/workflow/workflow_settings.hpp>

#include <bibqml/bridge/BridgeApplication.hpp>
#include <bibqml/bridge/BridgeBibleRefLookup.hpp>
#include <bibqml/bridge/BridgeBibleRefOcr.hpp>
#include <bibqml/bridge/BridgeScripture.hpp>
#include <bibqml/bridge/BridgeSettings.hpp>
#include <bibqml/model/ScriptureListModel.hpp>
#include <bibqml/model/SettingsListModel.hpp>

#include <memory>

namespace verselens
{

///
///
bridge_instance::~bridge_instance() noexcept = default;

///
///
auto disconnect_bridge(bridge_instance& instance) -> void
{
  assert(instance.bridge_bible_ref_lookup);
  assert(instance.bridge_bible_ref_ocr);
  assert(instance.scripture_list_model);
  assert(instance.settings_list_model);
  assert(instance.bridge_scripture);
  instance.bridge_bible_ref_lookup->disconnect();
  instance.bridge_bible_ref_ocr->disconnect();
  instance.scripture_list_model->disconnect();
  instance.settings_list_model->disconnect();
  instance.bridge_scripture->disconnect();
}

///
///
auto construct_bridge([[maybe_unused]] QGuiApplication& /*app*/, backend_instance& backend) -> bridge_instance
{
  // clang-format off
  auto workflow_settings = backend.workflow_settings;
  auto workflow_bible_ref_ocr = std::static_pointer_cast<bibstd::workflow::workflow_bible_ref_ocr>(backend.workflow_bible_ref_ocr);
  auto workflow_bible_ref_ocr_auto = std::static_pointer_cast<bibstd::workflow::workflow_bible_ref_ocr_auto>(backend.workflow_bible_ref_ocr_auto);
  auto workflow_hotkey = std::static_pointer_cast<bibstd::workflow::workflow_hotkey>(backend.workflow_hotkey);
  auto workflow_bible_ref_lookup = std::static_pointer_cast<bibstd::workflow::workflow_bible_ref_lookup>(backend.workflow_bible_ref_lookup);
  auto workflow_scripture = std::static_pointer_cast<bibstd::workflow::workflow_scripture>(backend.workflow_scripture);
  return bridge_instance{
    .bridge_application{std::make_unique<bibqml::BridgeApplication>()},
    .bridge_settings{std::make_unique<bibqml::BridgeSettings>(workflow_settings)},
    .settings_list_model{std::make_unique<bibqml::SettingsListModel>(workflow_settings)},
    .bridge_bible_ref_ocr{std::make_unique<bibqml::BridgeBibleRefOcr>(workflow_bible_ref_ocr, workflow_bible_ref_ocr_auto, workflow_hotkey, workflow_settings)},
    .bridge_bible_ref_lookup{std::make_unique<bibqml::BridgeBibleRefLookup>(workflow_bible_ref_lookup, workflow_scripture)},
    .scripture_list_model{std::make_unique<bibqml::ScriptureListModel>(workflow_scripture)},
    .bridge_scripture{std::make_unique<bibqml::BridgeScripture>(workflow_scripture)}
  };
  // clang-format on
}

///
///
auto connect_bridge(bridge_instance& instance) -> void
{
  // Connect bridge signal to passage model
  QObject::connect(
    instance.bridge_bible_ref_ocr.get(),
    &bibqml::BridgeBibleRefOcr::referenceFound,
    instance.scripture_list_model.get(),
    &bibqml::ScriptureListModel::resetWithReference
  );

  // Clear passage model when the manual search starts
  QObject::connect(
    instance.bridge_bible_ref_ocr.get(),
    &bibqml::BridgeBibleRefOcr::manualSearchRunningChanged,
    instance.scripture_list_model.get(),
    [model = instance.scripture_list_model.get()](bool running)
    {
      if(running)
      {
        model->clear();
      }
    }
  );
}

///
///
auto connect_engine(QQmlApplicationEngine& engine, QGuiApplication& app, bridge_instance& bridge) -> void
{
  // Set initial properties for the QML root component
  engine.setInitialProperties({
    {   "listModelSettings",     QVariant::fromValue(bridge.settings_list_model.get())},
    {  "listModelScripture",    QVariant::fromValue(bridge.scripture_list_model.get())},
    {   "bridgeBibleRefOcr",    QVariant::fromValue(bridge.bridge_bible_ref_ocr.get())},
    {"bridgeBibleRefLookup", QVariant::fromValue(bridge.bridge_bible_ref_lookup.get())},
    {   "bridgeApplication",      QVariant::fromValue(bridge.bridge_application.get())},
    {     "bridgeScripture",        QVariant::fromValue(bridge.bridge_scripture.get())},
  });

  load_qml_document(engine, app, QStringLiteral("qrc:/qt/qml/ui/qml/Main.qml"));
}

} // namespace verselens
