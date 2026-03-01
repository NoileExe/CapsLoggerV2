
#pragma once


#include <QAbstractNativeEventFilter>
#include <QByteArray>                  // for QByteArray
#include <QNonConstOverload>           // for QT_VERSION, QT_VERSION_CHECK
#include <QObject>                     // for Q_OBJECT
#include <QString>                     // for QString
#include "KeysLoggerProvider.h"

//-----------------------------------------------------------------------------

class QTimer;

using Display = struct _XDisplay;

//-----------------------------------------------------------------------------

class X11KeysLogger : public KeysLoggerProvider, public QAbstractNativeEventFilter
{
	Q_OBJECT

public:
	explicit X11KeysLogger(QObject *parent = nullptr);
	~X11KeysLogger() override;

	bool isCapsLockOn() const override;
	bool isNumLockOn() const override;
	bool isScrollLockOn() const override;

	void pressCapsLock() override;
	void pressNumLock() override;
	void pressScrollLock() override;

	bool supportsCaps() const override { return m_supportCaps; }
	bool supportsNum() const override { return m_supportNum; }
	bool supportsScroll() const override { return m_supportScroll; }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#else
	bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
#endif

private:
	void updateStateAndEmit();
	void startPolling();
	void initXkb();
	void detectSupport();

private:
	QTimer* m_timer;
	Display* m_display = nullptr;

	// Маски для вычленения состояний
	unsigned int m_maskCaps = 0;
	unsigned int m_maskNum = 0;
	unsigned int m_maskScroll = 0;

	// Текущие состояния для сравнения и отправки сигналов при изменении
	bool m_capsLockState = false;
	bool m_numLockState = false;
	bool m_scrollLockState = false;

	// Поддерживает ли система отслеживание определенного состояния
	bool m_supportCaps = false;
	bool m_supportNum = false;
	bool m_supportScroll = false;
};
