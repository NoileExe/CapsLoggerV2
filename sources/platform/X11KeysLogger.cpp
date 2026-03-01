
#include "X11KeysLogger.h"
#include <QGuiApplication>                // for qApp, QGuiApplication
#include <QMessageBox>                    // for QMessageBox
#include <QTimer>                         // for QTimer
#include <X11/X.h>                        // for Atom, KeyCode, KeySym, Success
#include <X11/XKBlib.h>                   // for XkbGetIndicatorState, XkbFr...
#include <X11/Xlib.h>                     // for XKeysymToKeycode, XFlush
#include <X11/extensions/XKB.h>           // for XkbUseCoreKbd, XkbIndicator...
#include <X11/extensions/XKBstr.h>        // for XkbDescPtr, _XkbDesc, _XkbN...
#include <X11/extensions/XTest.h>         // for XTestFakeKeyEvent
#include <X11/keysym.h>                   // for XK_Caps_Lock, XK_Num_Lock, XK_Scroll_Lock
#include <stdint.h>                       // for uint8_t
#include <xcb/xcb.h>                      // for xcb_generic_event_t
#include "KeysLoggerProvider.h"

//-----------------------------------------------------------------------------

X11KeysLogger::X11KeysLogger(QObject *parent)
	: KeysLoggerProvider(parent)
	, m_timer(nullptr)
{
	m_display = XOpenDisplay(nullptr);

	if (!m_display)
	{
		QMessageBox::critical(nullptr, QObject::tr("Ошибка"),
			QObject::tr("X11KeysLogger:  не удалось открыть X11 display"),
			QMessageBox::Ok);
		return;
	}

	// начальное состояние
	m_capsLockState   = X11KeysLogger::isCapsLockOn();
	m_numLockState    = X11KeysLogger::isNumLockOn();
	m_scrollLockState = X11KeysLogger::isScrollLockOn();

	initXkb();
	detectSupport();
	updateStateAndEmit();

	// пробуем подписаться на XKB события
	XkbSelectEvents(m_display, XkbUseCoreKbd,
					XkbStateNotifyMask | XkbIndicatorStateNotifyMask,
					XkbStateNotifyMask | XkbIndicatorStateNotifyMask);
	XFlush(m_display);

	qApp->installNativeEventFilter(this);

	// X11 не всегда дает глобальные события о смене LED -
	//  делаем опрос по таймеру
	m_timer = new QTimer(this);
	startPolling();
}

X11KeysLogger::~X11KeysLogger()
{
	if (m_timer)
		m_timer->stop();

	if (qApp)
		qApp->removeNativeEventFilter(this);

	if (m_display)
		XCloseDisplay(m_display);
}

//-----------------------------------------------------------------------------

void X11KeysLogger::initXkb()
{
	int opcode, eventBase, errorBase;
	if (!XkbQueryExtension(m_display, &opcode, &eventBase, &errorBase, nullptr, nullptr))
		return;

	XkbDescPtr xkb = XkbGetMap(m_display, XkbAllComponentsMask, XkbUseCoreKbd);
	if (!xkb)
		return;

	if (XkbGetNames(m_display, XkbIndicatorNamesMask, xkb) == Success) {
		for (int i = 0; i < XkbNumIndicators; ++i) {
			Atom atom = xkb->names->indicators[i];
			if (!atom)
				continue;

			char* name = XGetAtomName(m_display, atom);
			if (!name)
				continue;

			QString n = QString::fromUtf8(name).toLower();
			unsigned int mask = (1u << i);

			if (n.contains("caps"))
				m_maskCaps = mask;
			else if (n.contains("num"))
				m_maskNum = mask;
			else if (n.contains("scroll"))
				m_maskScroll = mask;

			XFree(name);
		}
	}

	XkbFreeKeyboard(xkb, 0, True);
}

void X11KeysLogger::detectSupport()
{
	m_supportCaps   = false;
	m_supportNum    = false;
	m_supportScroll = false;

	auto testToggle = [&](KeySym sym, bool &supportFlag)
	{
		if (supportFlag)
			return; // Уже определено по маске

		KeyCode kc = XKeysymToKeycode(m_display, sym);
		if (kc == 0) return;

		unsigned int before = 0;
		XkbGetIndicatorState(m_display, XkbUseCoreKbd, &before);

		// Нажатие
		XTestFakeKeyEvent(m_display, kc, True, 0);
		XTestFakeKeyEvent(m_display, kc, False, 0);
		XFlush(m_display);

		XSync(m_display, False);

		unsigned int after = 0;
		XkbGetIndicatorState(m_display, XkbUseCoreKbd, &after);

		if (before != after)
			supportFlag = true;

		// вернуть назад
		XTestFakeKeyEvent(m_display, kc, True, 0);
		XTestFakeKeyEvent(m_display, kc, False, 0);
		XFlush(m_display);
	};

	// Проверяем все три клавиши одинаково
	testToggle(XK_Caps_Lock,    m_supportCaps);
	testToggle(XK_Num_Lock,     m_supportNum);
	testToggle(XK_Scroll_Lock,  m_supportScroll);
}

//-----------------------------------------------------------------------------

bool X11KeysLogger::isCapsLockOn() const
{
	if (!m_supportCaps)     return false;
	
	unsigned int state;
	XkbGetIndicatorState(m_display, XkbUseCoreKbd, &state);
	return state & (m_maskCaps ? m_maskCaps : 0x01);
}

bool X11KeysLogger::isNumLockOn() const
{
	if (!m_supportNum)      return false;
	
	unsigned int state;
	XkbGetIndicatorState(m_display, XkbUseCoreKbd, &state);
	return state & (m_maskNum ? m_maskNum : 0x02);
}

bool X11KeysLogger::isScrollLockOn() const
{
	if (!m_supportScroll)   return false;
	
	unsigned int state;
	XkbGetIndicatorState(m_display, XkbUseCoreKbd, &state);
	return state & (m_maskScroll ? m_maskScroll : 0x04);
}

//-----------------------------------------------------------------------------

void X11KeysLogger::pressCapsLock()
{
	if (!m_supportCaps)		return;

	XTestFakeKeyEvent(m_display, XKeysymToKeycode(m_display, XK_Caps_Lock), True, 0);
	XTestFakeKeyEvent(m_display, XKeysymToKeycode(m_display, XK_Caps_Lock), False, 0);
	XFlush(m_display);
}

void X11KeysLogger::pressNumLock()
{
	if (!m_supportNum)		return;

	XTestFakeKeyEvent(m_display, XKeysymToKeycode(m_display, XK_Num_Lock), True, 0);
	XTestFakeKeyEvent(m_display, XKeysymToKeycode(m_display, XK_Num_Lock), False, 0);
	XFlush(m_display);
}

void X11KeysLogger::pressScrollLock()
{
	if (!m_supportScroll)	return;

	XTestFakeKeyEvent(m_display, XKeysymToKeycode(m_display, XK_Scroll_Lock), True, 0);
	XTestFakeKeyEvent(m_display, XKeysymToKeycode(m_display, XK_Scroll_Lock), False, 0);
	XFlush(m_display);
}

//-----------------------------------------------------------------------------

void X11KeysLogger::startPolling()
{
	connect(m_timer, &QTimer::timeout, this, &X11KeysLogger::updateStateAndEmit);
	m_timer->start(250);    // 250 мс
}

void X11KeysLogger::updateStateAndEmit()
{
	bool capsNew   = m_supportCaps   ? X11KeysLogger::isCapsLockOn() : false;
	bool numNew    = m_supportNum    ? X11KeysLogger::isNumLockOn() : false;
	bool scrollNew = m_supportScroll ? X11KeysLogger::isScrollLockOn() : false;

	if (capsNew != m_capsLockState) {
		m_capsLockState = capsNew;
		emit keyFunctionChanged(KeysLoggerProvider::Keyboard::CAPS, capsNew);
	}
	if (numNew != m_numLockState) {
		m_numLockState = numNew;
		emit keyFunctionChanged(KeysLoggerProvider::Keyboard::NUM, numNew);
	}
	if (scrollNew != m_scrollLockState) {
		m_scrollLockState = scrollNew;
		emit keyFunctionChanged(KeysLoggerProvider::Keyboard::SCROLL, scrollNew);
	}
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool X11KeysLogger::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
#else
bool X11KeysLogger::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
#endif
{
	Q_UNUSED(result);

	if (eventType != "xcb_generic_event_t")
		return false;

	auto* ev = static_cast<xcb_generic_event_t*>(message);
	uint8_t type = ev->response_type & ~0x80;

	// XKB events = 98
	if (type == 98)
	{
		updateStateAndEmit();
		return false;
	}

	return false;
}
