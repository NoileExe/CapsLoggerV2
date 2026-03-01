
#include "KeysLoggerProvider.h"
#include <QtCore/qglobal.h>     // for Q_OS_LINUX

// Подключаем реализации для всех платформ (компилятор возьмёт нужную)
#ifdef Q_OS_WIN
	#include "WinKeysLogger.h"
#elif defined(Q_OS_LINUX)
	#include <QProcessEnvironment>
	#include "X11KeysLogger.h"
	#include "WaylandKeysLogger.h"
#else
	#error "Unsupported platform!"
#endif

//-----------------------------------------------------------------------------

KeysLoggerProvider* KeysLoggerProvider::create(QObject *parent)
{
#ifdef Q_OS_WIN
	// Windows: всегда используем хук
	return new WinKeysLogger(parent);

#elif defined(Q_OS_LINUX)
	auto env = QProcessEnvironment::systemEnvironment();

	// Linux: определяем сессию (X11 или Wayland) через переменную окружения
	QString sessionType = env.value("XDG_SESSION_TYPE", "unknown").toLower();

	if (sessionType == "x11")               return new X11KeysLogger(parent);
	if (sessionType == "wayland")           return new WaylandKeysLogger(parent);
	if (!env.value("DISPLAY").isEmpty())    return new X11KeysLogger(parent);

	// По умолчанию Wayland
	return new WaylandKeysLogger(parent);
#else
	#error "Unsupported platform — cannot create KeysLoggerProvider"
	return nullptr;
#endif
}
