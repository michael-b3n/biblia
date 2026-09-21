#pragma once

#include "src/construct_backend.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

// Forward declarations
namespace bibqml
{
class BridgeApplication;
class BridgeBibleRefLookup;
class BridgeBibleRefOcr;
class BridgeScripture;
class BridgeSettings;
class ScriptureListModel;
class SettingsListModel;
} // namespace bibqml

namespace verselens
{

struct bridge_instance final
{
  // Structors
  ~bridge_instance() noexcept;

  // Variables
  std::unique_ptr<bibqml::BridgeApplication> bridge_application;
  std::unique_ptr<bibqml::BridgeSettings> bridge_settings;
  std::unique_ptr<bibqml::SettingsListModel> settings_list_model;
  std::unique_ptr<bibqml::BridgeBibleRefOcr> bridge_bible_ref_ocr;
  std::unique_ptr<bibqml::BridgeBibleRefLookup> bridge_bible_ref_lookup;
  std::unique_ptr<bibqml::ScriptureListModel> scripture_list_model;
  std::unique_ptr<bibqml::BridgeScripture> bridge_scripture;
};

///
/// Disconnect all signal connections and perform necessary cleanup for the bridge instance.
/// This will stop the frontend backend communication.
///
auto disconnect_bridge(bridge_instance& instance) -> void;

///
/// Initialize the bridge instances.
/// \return bridge instance
///
auto construct_bridge(QGuiApplication& app, backend_instance& backend) -> bridge_instance;

///
/// Connect internal signals between bridge components.
///
auto connect_bridge(bridge_instance& instance) -> void;

///
/// Connect the QML engine to the bridge instances and load the main QML file.
///
auto connect_engine(QQmlApplicationEngine& engine, QGuiApplication& app, bridge_instance& bridge) -> void;

} // namespace verselens
