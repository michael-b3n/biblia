#pragma once

#include "bibqml/translation/DisplayNameTable.hpp"

#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlengine.h>
#include <QtQmlIntegration/qqmlintegration.h>

#include <cstddef>
#include <span>

namespace bibqml
{

///
/// QML Translations singleton, the display names of the identifiers the QML layer shows, e.g. setting segments.
/// Owned by the application, which sets the language, it outlives the QML engine.
/// \note Bindings read `language` to be reevaluated on a change, the invokables take it as last argument.
///
class Translations final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QString language READ language NOTIFY languageChanged FINAL)
  Q_PROPERTY(QStringList availableLanguages READ availableLanguages CONSTANT FINAL)

  // Variables
  const DisplayNameTable names_;
  QString language_;

public: // Constants
  ///
  /// Separator between the display names of several keys displayed as one text.
  ///
  static constexpr auto nameSeparator = QLatin1Char{' '};

public: // Static interface
  ///
  /// Access the instance the QML layer operates on.
  /// \return the instance, nullptr if no instance exists
  ///
  [[nodiscard]] static bibstd::util::non_owning_ptr<Translations> instance();

  ///
  /// QML singleton factory. The returned instance stays owned by the application.
  /// \return the instance, nullptr if no instance exists
  ///
  static Translations* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

public: // Structors
  ///
  /// \throws util::exception if the document does not describe a table of display names
  ///
  explicit Translations(std::span<const std::byte> csv, bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~Translations() noexcept override;

public: // Accessors
  ///
  /// \return language the display names are written in
  ///
  [[nodiscard]] QString language() const;

  ///
  /// \return all languages the application provides display names for
  ///
  [[nodiscard]] QStringList availableLanguages() const;

public: // Setters
  ///
  /// Set the language the display names are written in. Unknown languages are ignored.
  ///
  void setLanguage(const QString& language);

public: // Methods
  ///
  /// Get the display name of a key, e.g. the path of a setting or the name of a button.
  /// \return display name, the key itself if no display name is available
  ///
  Q_INVOKABLE QString name(const QString& key, const QString& language = {}) const;

  ///
  /// Get the display names of several keys as one text, e.g. the segments a setting is named by.
  /// \return display names in the order of the keys, separated by a space
  ///
  Q_INVOKABLE QString names(const QStringList& keys, const QString& language = {}) const;

signals:
  void languageChanged();
};

} // namespace bibqml
