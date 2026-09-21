#include "src/construct_tray.hpp"
#include "res/version.hpp"
#include "src/qml_application.hpp"

#include <bibqml/bridge/BridgeApplication.hpp>
#include <bibqml/translation/Translations.hpp>

#include <bibstd/system/open_browser.hpp>
#include <bibstd/system/tray.hpp>
#include <bibstd/util/incbin.hpp>

#include <QMetaObject>
#include <QObject>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

INC_RESOURCE(icon, "res/icon.ico");
const auto icon_view = bibstd::util::incbin::to_span<std::byte>(res_icon_data, res_icon_size);

namespace verselens
{
namespace
{

///
/// Tray button, named by the key of its display name.
///
struct tray_button final
{
  const char* key;
  std::function<void()> callback;
};

///
/// Get the display name of a key in the language the translations are displayed in.
/// \return display name, the key itself if no display name is available
///
[[nodiscard]] auto tray_name(const char* const key) -> std::string
{
  const auto name = QString::fromLatin1(key);
  const auto* const translations = bibqml::Translations::instance();
  return (translations != nullptr ? translations->name(name) : name).toStdString();
}

} // namespace

///
///
auto construct_tray(QGuiApplication& app, bridge_instance& bridge, translations_instance& translations)
  -> bibstd::util::shared_scope_guard
{
  const auto do_on_exit = [&app, &bridge, &translations]() { quit_application(app, bridge, translations); };
  const auto open_github = []() { bibstd::system::open_browser::open(std::string{version::repository_url}); };
  const auto show_window = [&bridge]() { bridge.bridge_application->requestShowWindow(); };

  // The position in this list is the index the tray knows an entry by.
  const auto buttons = std::vector<tray_button>{
    {.key = "tray_show_window", .callback = show_window},
    {.key = "tray_open_github", .callback = open_github},
    {       .key = "tray_exit",  .callback = do_on_exit},
  };

  auto entries = std::vector<bibstd::system::tray::entry_type>{};
  std::ranges::transform(
    buttons,
    std::back_inserter(entries),
    [](const auto& button) { return bibstd::system::tray::button{.text = tray_name(button.key), .callback = button.callback}; }
  );
  auto guard = bibstd::system::tray::init(bibstd::system::tray::icon_buffer{icon_view}, std::move(entries));

  if(const auto* const names = bibqml::Translations::instance(); names != nullptr)
  {
    QObject::connect(
      names,
      &bibqml::Translations::languageChanged,
      &app,
      [buttons]()
      {
        for(auto index = std::size_t{0}; index < buttons.size(); ++index)
        {
          bibstd::system::tray::set_text(index, tray_name(buttons[index].key));
        }
      }
    );
  }
  return guard;
}

} // namespace verselens
