
#include <QMessageBox>			// for QMessageBox
#include <QTimer>				// for QTimer
#include "WinKeysLogger.h"
#include "KeysLoggerProvider.h"	// for KeysLoggerProvider

//-----------------------------------------------------------------------------

WinKeysLogger* WinKeysLogger::s_instance = nullptr;

//-----------------------------------------------------------------------------

WinKeysLogger::WinKeysLogger(QObject *parent)
	: KeysLoggerProvider(parent)
{
	s_instance = this;

	m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardHookProc, nullptr, 0);
	if (!m_hook)
	{
		QMessageBox::critical(nullptr, QObject::tr("Ошибка"),
			QObject::tr("Не удалось установить хук клавиатуры (ошибка:\n%0\n)").arg(GetLastError()),
			QMessageBox::Ok);
		return;
	}

	// Инициализация текущего состояния
	m_capsLockState		= isCapsLockOn();
	m_numLockState		= isNumLockOn();
	m_scrollLockState	= isScrollLockOn();
}

WinKeysLogger::~WinKeysLogger()
{
	if (m_hook)
	{
		UnhookWindowsHookEx(m_hook);
		m_hook = nullptr;
	}

	s_instance = nullptr;
}

//-----------------------------------------------------------------------------

bool WinKeysLogger::isCapsLockOn() const
{
	return (GetKeyState(VK_CAPITAL) & 1) != 0;
}

bool WinKeysLogger::isNumLockOn() const
{
	return (GetKeyState(VK_NUMLOCK) & 1) != 0;
}

bool WinKeysLogger::isScrollLockOn() const
{
	return (GetKeyState(VK_SCROLL) & 1) != 0;
}

//-----------------------------------------------------------------------------

void WinKeysLogger::pressCapsLock()
{
	// keybd_event - deprecated.
	// SendInput поддерживается начиная с Win 2000
	// Сам Qt 5/6 минимальная поддерживает Windows гораздо новее

	INPUT inputs[2] = {};
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_CAPITAL;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_CAPITAL;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, inputs, sizeof(INPUT));

	QTimer::singleShot(250, this, &WinKeysLogger::updateStateAndEmit);
}

void WinKeysLogger::pressNumLock()
{
	INPUT inputs[2] = {};
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_NUMLOCK;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_NUMLOCK;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, inputs, sizeof(INPUT));

	QTimer::singleShot(250, this, &WinKeysLogger::updateStateAndEmit);
}

void WinKeysLogger::pressScrollLock()
{
	INPUT inputs[2] = {};
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_SCROLL;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_SCROLL;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, inputs, sizeof(INPUT));

	QTimer::singleShot(250, this, &WinKeysLogger::updateStateAndEmit);
}

//-----------------------------------------------------------------------------

void WinKeysLogger::updateStateAndEmit()
{
	bool capsNew = isCapsLockOn();
	bool numNew = isNumLockOn();
	bool scrollNew = isScrollLockOn();

	if (capsNew != m_capsLockState)
	{
		m_capsLockState = capsNew;
		emit keyFunctionChanged(KeysLoggerProvider::Keyboard::CAPS, capsNew);
	}

	if (numNew != m_numLockState)
	{
		m_numLockState = numNew;
		emit keyFunctionChanged(KeysLoggerProvider::Keyboard::NUM, numNew);
	}

	if (scrollNew != m_scrollLockState)
	{
		m_scrollLockState = scrollNew;
		emit keyFunctionChanged(KeysLoggerProvider::Keyboard::SCROLL, scrollNew);
	}
}

//-----------------------------------------------------------------------------

LRESULT CALLBACK WinKeysLogger::keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION  &&  s_instance  &&  wParam == WM_KEYUP)
	{
		KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		switch (kb->vkCode)
		{
			case VK_CAPITAL:  case VK_NUMLOCK:  case VK_SCROLL:
				s_instance->updateStateAndEmit();
				break;
		}
	}
	
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
