
#pragma once

#include <QtCore/qglobal.h>  // for Q_OS_LINUX
#include <QString>           // for QString
#include <QTranslator>       // for QTranslator

//-----------------------------------------------------------------------------

enum class CapsLoggerMessageType
{
	SYSTEM,
	CUSTOM
};

enum class CapsLoggerMessageLocation
{
	TOP_LEFT		= 0,
	TOP_RIGHT		= 1,
	BOTTOM_LEFT		= 2,
	BOTTOM_RIGHT	= 3,
	TOP_CENTER		= 4,
	BOTTOM_CENTER	= 5,
	COUNT			= 6
};

//-----------------------------------------------------------------------------

static QTranslator appTranslator;

enum class CapsLoggerLanguage
{
	RUS,
	ENG,
	COUNT
};

//-----------------------------------------------------------------------------

namespace CapsLoggerSettings
{
#ifdef Q_OS_LINUX
	// Проверка на то используется ли X11 окружение
	bool isX11Environment();
#endif

	// Возможность отображения popup-сообщений силами ОС
	bool isSysMessagesAvailable();

	void initSettings();

	// Получить строковое значение расположения по значению из enum
	QString locationName(CapsLoggerMessageLocation location);
	QString languageName(CapsLoggerLanguage lang);

	// Устанвока .qm файла из ресурсов в качестве источника строк для приложения
	// с использованием указанного языка из enum
	void setAppLanguage(CapsLoggerLanguage lang);
}

//-----------------------------------------------------------------------------

// Текущий язык интерфейса
inline const QString& languageSettings() {
	static const QString v = QStringLiteral("design/language");
	return v;
}

// Текущая настроеная тема
inline const QString& themeSettings() {
	static const QString v = QStringLiteral("design/theme_path");
	return v;
}

// Текущий тип сообщений о смене состояний
inline const QString& msgTypeSettings() {
	static const QString v = QStringLiteral("design/message_type");
	return v;
}

// В каком углу будут отображаться кастомные сообщения
inline const QString& msgLocationSettings() {
	static const QString v = QStringLiteral("design/message_location");
	return v;
}

// Время анимации иконки, мс
inline const QString& timeIconSettings() {
	static const QString v = QStringLiteral("animation/time/icon");
	return v;
}

// Время видимости на экране сообщения, мс
inline const QString& timeMsgSettings() {
	static const QString v = QStringLiteral("animation/time/message");
	return v;
}

// Отслеживать ли смену CAPS Lock
inline const QString& capsTrackSettings() {
	static const QString v = QStringLiteral("tracking/caps_lock");
	return v;
}

// Отслеживать ли смену NUM Lock
inline const QString& numTrackSettings() {
	static const QString v = QStringLiteral("tracking/num_lock");
	return v;
}

// Отслеживать ли смену SCROLL Lock
inline const QString& scrollTrackSettings() {
	static const QString v = QStringLiteral("tracking/scroll_lock");
	return v;
}

//-----------------------------------------------------------------------------

inline const QString& allOffFileName() {
	static const QString v = QStringLiteral("ALL_OFF.png");
	return v;
}

inline const QString& capsFileName() {
	static const QString v = QStringLiteral("CAPS.png");
	return v;
}

inline const QString& numFileName() {
	static const QString v = QStringLiteral("NUM.png");
	return v;
}

inline const QString& scrollFileName() {
	static const QString v = QStringLiteral("SCROLL.png");
	return v;
}

//-----------------------------------------------------------------------------

// Путь к папке со сторонними темами
inline const QString& customThemePath() {
	static const QString v = QStringLiteral("THEMES/");
	return v;
}

// Путь к теме по умолчанию (из ресурсов)
inline const QString& defaultThemePath() {
	static const QString v = QStringLiteral(":/resources/");
	return v;
}

constexpr int defTimeMs{ 3000 };						// Стандартная длительность
														// анимации/отображения сообщения
constexpr int maxTimeMs{ 300000 };						// Максимальная длительность (5 мин)
														// анимации/отображения сообщения