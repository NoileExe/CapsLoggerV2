
#pragma once


#include <QObject>
#include <QString>               // for QString
#include "KeysLoggerProvider.h"

//-----------------------------------------------------------------------------

class QTimer;

//-----------------------------------------------------------------------------

class WaylandKeysLogger : public KeysLoggerProvider
{
	Q_OBJECT

public:
	explicit WaylandKeysLogger(QObject *parent = nullptr);
	~WaylandKeysLogger() override;

	bool isCapsLockOn() const override;
	bool isNumLockOn() const override;
	bool isScrollLockOn() const override;

	void pressCapsLock() override;
	void pressNumLock() override;
	void pressScrollLock() override;

	bool supportsCaps() const override { return m_supportCaps; }
	bool supportsNum() const override { return m_supportNum; }
	bool supportsScroll() const override { return m_supportScroll; }

private:
	void startPolling();
	void updateStateAndEmit();

	// Чтение состояния индикатора
	bool readLedState(const QString &path) const;

private:
	QTimer* m_timer = nullptr;

	// Текущие состояния для сравнения и отправки сигналов при изменении
	bool m_capsLockState = false;
	bool m_numLockState = false;
	bool m_scrollLockState = false;

	// Поддерживает ли система отслеживание определенного состояния
	bool m_supportCaps = false;
	bool m_supportNum = false;
	bool m_supportScroll = false;

	// Пути к LED-индикаторам
	QString m_capsPath;
	QString m_numPath;
	QString m_scrollPath;
};
