#pragma once

#include "src/construct_bridge.hpp"
#include "src/construct_translations.hpp"

#include <arca/util/scope_guard.hpp>

#include <QGuiApplication>

namespace verselens
{

///
/// Start the system tray. Its entries are named through the translations and follow the
/// language they are displayed in.
/// \return guard removing the tray when it goes out of scope
///
[[nodiscard]] auto construct_tray(QGuiApplication& app, bridge_instance& bridge, translations_instance& translations)
  -> bibstd::util::shared_scope_guard;

} // namespace verselens
