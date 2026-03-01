
#include "SettingsDefines.h"
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QObject>				// for QObject
#include <QVariant>				// for QVariant

#ifdef Q_OS_LINUX
	#include <QProcessEnvironment>
	#include <QStringList>		// for QStringList
	#include <QDBusConnection>	// for QDBusConnection
	#include <QDBusInterface>	// for QDBusInterface
	#include <QDBusReply>		// for QDBusReply
#else
	#include <QSystemTrayIcon>
#endif

//-----------------------------------------------------------------------------

namespace CapsLoggerSettings
{
#ifdef Q_OS_LINUX
	bool checkX11Environment()
	{
		auto env = QProcessEnvironment::systemEnvironment();
		QString sessionType = env.value("XDG_SESSION_TYPE", "unknown").toLower();

		return (sessionType == "x11")  ||
					(sessionType != "wayland" && !env.value("DISPLAY").isEmpty());
	}

	bool isX11Environment()
	{
		static const bool v = CapsLoggerSettings::checkX11Environment();
		return v;
	}
#endif

	// Проверка возможности отображения сообщений силами операционной системы
	bool checkNotificationsAvailability()
	{
#ifdef Q_OS_LINUX
		auto bus = QDBusConnection::sessionBus();
		if (!bus.isConnected())
			return false;

		QDBusInterface interface{
			"org.freedesktop.Notifications",
			"/org/freedesktop/Notifications",
			"org.freedesktop.Notifications",
			bus
		};

		if (!interface.isValid())
			return false;

		// Проверка сервиса блокирующим вызовом (разбудить демон для Linux Mint MATE)
		QDBusReply<QStringList> reply = interface.call("GetCapabilities");
		if (!reply.isValid())
			return false;

		return true;
#else
		return QSystemTrayIcon::supportsMessages();
#endif
	}

	bool isSysMessagesAvailable()
	{
		static const bool v = CapsLoggerSettings::checkNotificationsAvailability();
		return v;
	}
	
	void initSettings()
	{
		// Получаем путь к папке с исполняемым файлом
		QString appDir = QApplication::applicationDirPath();
		
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, appDir);
		QSettings::setDefaultFormat(QSettings::IniFormat);		// INI как формат по умолчанию

		//-------------------------------------------------------------------------

		QSettings settings;
		
		QString theme_path = settings.contains(themeSettings()) ?
								settings.value(themeSettings()).toString() :
								defaultThemePath();
		bool isCapsTrackingOn = settings.contains(capsTrackSettings()) ?
									settings.value(capsTrackSettings()).toBool() :
									true;
		bool isNumTrackingOn = settings.contains(numTrackSettings()) ?
									settings.value(numTrackSettings()).toBool() :
									true;
		bool isScrollTrackingOn = settings.contains(scrollTrackSettings()) ?
									settings.value(scrollTrackSettings()).toBool() :
									true;
		int message_type = settings.contains(msgTypeSettings()) ?
								settings.value(msgTypeSettings()).toInt() :
								static_cast<int>(CapsLoggerMessageType::CUSTOM);
		int message_location = settings.contains(msgLocationSettings()) ?
									settings.value(msgLocationSettings()).toInt() :
									static_cast<int>(CapsLoggerMessageLocation::BOTTOM_RIGHT);
		int language = settings.contains(languageSettings()) ?
							settings.value(languageSettings()).toInt() :
							static_cast<int>(CapsLoggerLanguage::RUS);
		int time_icon = settings.contains(timeIconSettings()) ?
							settings.value(timeIconSettings()).toInt() :
							defTimeMs;
		int time_message = settings.contains(timeMsgSettings()) ?
								settings.value(timeMsgSettings()).toInt() :
								defTimeMs;

		//-------------------------------------------------------------------------

		if (theme_path  !=  defaultThemePath())
		{
			if (!theme_path.endsWith('/')  &&  !theme_path.endsWith('\\'))
				theme_path += '/';
			
			// Отсутствует директория или набора иконок не полный -
			// меняем тему на дефолтную
			if (!QFile::exists(theme_path) ||
				!QFile::exists(theme_path + allOffFileName()) ||
				!QFile::exists(theme_path + capsFileName()) ||
				!QFile::exists(theme_path + numFileName()) ||
				!QFile::exists(theme_path + scrollFileName()))
			{
				theme_path = defaultThemePath();
			}
		}

		// Ошибочное значение или настроено на системные сообщения, которые не поддерживаются
		if (static_cast<int>(CapsLoggerMessageType::CUSTOM) < message_type ||
			(message_type == static_cast<int>(CapsLoggerMessageType::SYSTEM) && !CapsLoggerSettings::isSysMessagesAvailable()) )
		{
			message_type = static_cast<int>(CapsLoggerMessageType::CUSTOM);
		}
		
#ifdef Q_OS_LINUX
		if (!settings.contains(msgTypeSettings())  &&
			!CapsLoggerSettings::isX11Environment()  &&
			CapsLoggerSettings::isSysMessagesAvailable())
		{
			message_type = static_cast<int>(CapsLoggerMessageType::SYSTEM);
		}
#endif

		if (static_cast<int>(CapsLoggerMessageLocation::COUNT) <= message_location)
			message_location = static_cast<int>(CapsLoggerMessageLocation::BOTTOM_RIGHT);

		if (static_cast<int>(CapsLoggerLanguage::COUNT) <= language)
			language = static_cast<int>(CapsLoggerLanguage::RUS);
		
		CapsLoggerSettings::setAppLanguage(static_cast<CapsLoggerLanguage>(language));

		if (maxTimeMs < time_icon)
			time_icon = defTimeMs;

		if (maxTimeMs < time_message)
			time_message = defTimeMs;

		//-------------------------------------------------------------------------

		settings.setValue(themeSettings(),		theme_path);
		settings.setValue(capsTrackSettings(),	isCapsTrackingOn);
		settings.setValue(numTrackSettings(),		isNumTrackingOn);
		settings.setValue(scrollTrackSettings(),	isScrollTrackingOn);
		settings.setValue(msgTypeSettings(),		message_type);
		settings.setValue(msgLocationSettings(),	message_location);
		settings.setValue(languageSettings(),		language);
		settings.setValue(timeIconSettings(),		time_icon);
		settings.setValue(timeMsgSettings(),		time_message);
		
		settings.sync();
	}

	//-----------------------------------------------------------------------------

	QString locationName(CapsLoggerMessageLocation location)
	{
		switch (location)
		{
			case CapsLoggerMessageLocation::TOP_LEFT:
				return QObject::tr("верхний левый угол");
			
			case CapsLoggerMessageLocation::TOP_RIGHT:
				return QObject::tr("верхний правый угол");
			
			case CapsLoggerMessageLocation::BOTTOM_LEFT:
				return QObject::tr("нижний левый угол");
			
			case CapsLoggerMessageLocation::BOTTOM_RIGHT:
				return QObject::tr("нижний правый угол");
			
			case CapsLoggerMessageLocation::TOP_CENTER:
				return QObject::tr("посередине сверху");
			
			case CapsLoggerMessageLocation::BOTTOM_CENTER:
				return QObject::tr("посередине снизу");
		}

		return QObject::tr("нижний правый угол");
	}

	QString languageName(CapsLoggerLanguage lang)
	{
		switch (lang)
		{
			case CapsLoggerLanguage::RUS:
				return QString{"русский"};
			
			case CapsLoggerLanguage::ENG:
				return QString{"english"};
		}

		return QString{"русский"};
	}

	//-----------------------------------------------------------------------------

	void setAppLanguage(CapsLoggerLanguage lang)
	{
		qApp->removeTranslator(&appTranslator);

		QString langCode;
		switch (lang)
		{
			case CapsLoggerLanguage::ENG:
				langCode = "en";
				break;
			
			default:
				langCode = "ru";	// По умолчанию CapsLoggerLanguage::RUS
		}

		QString qmLangFile = QString(":/translations/CapsLoggerV2_%1.qm").arg(langCode);
		if ( !QFile::exists(qmLangFile) )
		{
			qmLangFile = ":/translations/CapsLoggerV2_ru.qm";
		}
		
		if ( appTranslator.load(qmLangFile) )
			qApp->installTranslator(&appTranslator);
	}
}

//-----------------------------------------------------------------------------
