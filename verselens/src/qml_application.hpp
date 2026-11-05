#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>

namespace verselens
{

struct bridge_instance;
class translations_instance;

///
/// Configure how the qml layer is rendered and styled.
/// \note Must be called before the qml application is constructed.
///
auto configure_qml_layer() -> void;

///
/// Load a qml document and exit the application if its root object cannot be created.
///
auto load_qml_document(QQmlApplicationEngine& engine, QGuiApplication& app, const QString& url) -> void;

///
/// Quit the application: the bridge and the translations stop reaching the qml layer first, then the event loop ends.
///
auto quit_application(QGuiApplication& app, bridge_instance& bridge, translations_instance& translations) -> void;

} // namespace verselens
