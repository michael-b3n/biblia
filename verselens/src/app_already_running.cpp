#include "src/app_already_running.hpp"
#include "res/version.hpp"
#include "src/construct_translations.hpp"
#include "src/qml_application.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <string>

namespace verselens
{

///
///
auto show_already_running(int argc, char** argv) -> int
{
  configure_qml_layer();
  QGuiApplication app(argc, argv);

  // The notice reads like the rest of the application: same display names, same language. The
  // language is only read, a second instance must never write what the running one owns.
  const auto translations = construct_translations(read_language_setting());

  QQmlApplicationEngine engine;
  engine.setInitialProperties({
    {"applicationName", QString::fromStdString(std::string{version::app_name})},
  });
  load_qml_document(engine, app, QStringLiteral("qrc:/qt/qml/ui/qml/windows/AlreadyRunningWindow.qml"));
  return QGuiApplication::exec();
}

} // namespace verselens
