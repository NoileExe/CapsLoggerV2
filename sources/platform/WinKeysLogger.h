
#pragma once


#include <windows.h>			// IWYU pragma: keep
#include <QObject>				// for QObject, Q_OBJECT
#include <QString>				// for QString
#include "KeysLoggerProvider.h"


//-----------------------------------------------------------------------------

class WinKeysLogger : public KeysLoggerProvider {
Q_OBJECT

public:
	explicit WinKeysLogger(QObject *parent = nullptr);
	~WinKeysLogger() override;

	bool isCapsLockOn() const override;
	bool isNumLockOn() const override;
	bool isScrollLockOn() const override;

	void pressCapsLock() override;
	void pressNumLock() override;
	void pressScrollLock() override;

	bool supportsCaps() const override { return true; }
	bool supportsNum() const override { return true; }
	bool supportsScroll() const override { return true; }

private:
	static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
	static WinKeysLogger* s_instance;		// Для доступа из статического callback

	void updateStateAndEmit();

private:
	HHOOK m_hook = nullptr;

	bool m_capsLockState = false;
	bool m_numLockState = false;
	bool m_scrollLockState = false;
};
