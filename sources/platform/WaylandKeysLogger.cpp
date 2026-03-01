
#include "WaylandKeysLogger.h"
#include <QByteArray>                     // for QByteArray, operator==
#include <QDir>                           // for QDir, operator|
#include <QFile>                          // for QFile
#include <QIODevice>                      // for QIODevice
#include <QMessageBox>                    // for QMessageBox
#include <QStringList>                    // for QStringList
#include <QTimer>                         // for QTimer
#include <Qt>                             // for CaseSensitivity
#include "KeysLoggerProvider.h"           // for KeysLoggerProvider


//-----------------------------------------------------------------------------

WaylandKeysLogger::WaylandKeysLogger(QObject *parent)
	: KeysLoggerProvider(parent)
{
	// Ищем каталоги LED-ов
	QString ledsPathStr{ "/sys/class/leds/" };
	QDir leds(ledsPathStr);
	if (leds.exists())
		for (const QString& name : leds.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
		{
			if (name.contains("capslock", Qt::CaseInsensitive))
				m_capsPath = ledsPathStr + name + "/brightness";
			if (name.contains("numlock", Qt::CaseInsensitive))
				m_numPath = ledsPathStr + name + "/brightness";
			if (name.contains("scrolllock", Qt::CaseInsensitive))
				m_scrollPath = ledsPathStr + name + "/brightness";
		}

	// поддержка
	m_supportCaps   = !m_capsPath.isEmpty();
	m_supportNum    = !m_numPath.isEmpty();
	m_supportScroll = !m_scrollPath.isEmpty();  // С большой долей вероятности Wayland не поддерживает Scroll Lock

	// начальное состояние
	m_capsLockState   = isCapsLockOn();
	m_numLockState    = isNumLockOn();
	m_scrollLockState = isScrollLockOn();

	m_timer = new QTimer(this);
	startPolling();
}

WaylandKeysLogger::~WaylandKeysLogger()
{
	if (m_timer)
		m_timer->stop();
}

//-----------------------------------------------------------------------------

bool WaylandKeysLogger::readLedState(const QString &path) const
{
	if (path.isEmpty())     return false;

	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return false;

	QByteArray data = f.readAll().trimmed();
	return (data == "1");
}

bool WaylandKeysLogger::isCapsLockOn() const
{
	return m_supportCaps ? readLedState(m_capsPath) : false;
}

bool WaylandKeysLogger::isNumLockOn() const
{
	return m_supportNum ? readLedState(m_numPath) : false;
}

bool WaylandKeysLogger::isScrollLockOn() const
{
	return m_supportScroll ? readLedState(m_scrollPath) : false;
}

//-----------------------------------------------------------------------------

void WaylandKeysLogger::pressCapsLock()
{
	QMessageBox::warning(nullptr, QObject::tr("Не поддерживается"),
		QObject::tr("Wayland не имеет стандартного API для виртуального нажатия кнопок."),
		QMessageBox::Ok);
}

void WaylandKeysLogger::pressNumLock()
{
	QMessageBox::warning(nullptr, QObject::tr("Не поддерживается"),
		QObject::tr("Wayland не имеет стандартного API для виртуального нажатия кнопок."),
		QMessageBox::Ok);
}

void WaylandKeysLogger::pressScrollLock()
{
	QMessageBox::warning(nullptr, QObject::tr("Не поддерживается"),
		QObject::tr("Wayland не имеет стандартного API для виртуального нажатия кнопок."),
		QMessageBox::Ok);
}

//-----------------------------------------------------------------------------

void WaylandKeysLogger::startPolling()
{
	connect(m_timer, &QTimer::timeout, this, &WaylandKeysLogger::updateStateAndEmit);
	m_timer->start(250);    // 250 мс
}

void WaylandKeysLogger::updateStateAndEmit()
{
	bool capsNew   = isCapsLockOn();
	bool numNew    = isNumLockOn();
	bool scrollNew = isScrollLockOn();

	if (capsNew != m_capsLockState)
	{
		m_capsLockState = capsNew;
		emit keyFunctionChanged(KeysLoggerProvider::CAPS, capsNew);
	}
	if (numNew != m_numLockState)
	{
		m_numLockState = numNew;
		emit keyFunctionChanged(KeysLoggerProvider::NUM, numNew);
	}
	if (scrollNew != m_scrollLockState)
	{
		m_scrollLockState = scrollNew;
		emit keyFunctionChanged(KeysLoggerProvider::SCROLL, scrollNew);
	}
}
