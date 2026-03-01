
#pragma once


#include <QWidget>                           // for QWidget
#include <QIcon>                             // for QIcon
#include <QObject>                           // for Q_OBJECT, slots
#include <QString>                           // for QString
#include <QSystemTrayIcon>                   // for QSystemTrayIcon
#include <QtCore>                            // for uint, Q_OS_LINUX
#include <bitset>
#include "../platform/KeysLoggerProvider.h"

//-----------------------------------------------------------------------------

class QMenu;
class QAction;
class QPixmap;
class SettingsWidget;
class TrayMessagePopup;

//-----------------------------------------------------------------------------

class KeysLoggerTray : public QSystemTrayIcon
{
Q_OBJECT

using KB_button = KeysLoggerProvider::Keyboard;

public:
	KeysLoggerTray(QWidget* pwgt = 0);
	
	~KeysLoggerTray();

protected slots:
	void slotChangeKeyState(KB_button btn, bool on);
	void slotBreathTick();
	
	void slotSetButtonState();
	void slotShowSettings();
	void slotShowAbout();

#ifdef Q_OS_LINUX
	void onNotificationClosed(uint id, uint reason);
#endif

private:
	void applyAlpha(QPixmap &base, double alpha) const;

	void startNewAnimation();			// Выставляет текущую иконку и, если требуется,
										// начинает анимацию с её полной непрозрачности
	bool isSet(KB_button btn) const;

	void showSystemNotify(const QString& title, const QString& msg, 
							const QIcon& icon, int timeoutMs);

	QString createTooltip() const;
	void buildMenu();

private:
	QMenu* m_pContextMenu;
	QAction* m_pActCheckCaps = nullptr;		// Включение/выключение CAPS из меню
	QAction* m_pActCheckNum = nullptr;		// Включение/выключение NUM из меню
	QAction* m_pActCheckScroll = nullptr;	// Включение/выключение SCROLL из меню
#ifdef Q_OS_LINUX
	uint m_lastNotificationId = 0;			// ID последнего отправленного системного уведомления
#endif

	TrayMessagePopup* m_pPopupMessage;		// Всплывающее сообщение об изменении состояния
	SettingsWidget* m_pSettingsWindow;		// Всплывающее сообщение об изменении состояния

	KeysLoggerProvider *m_keysProvider;		// Класс отслеживающий изменение состояний и
											// отправляющий сигналы об этом

	std::bitset<4> m_keyStates;				// Состояния функций кнопок (вкл/выкл) - используется
											// ТОЛЬКО для отображения отслеживаемых состояний:
											// тултипов и анимации смены иконок (не для меню)

	KB_button m_currentIcon;				// Текущая иконка

	QTimer* m_breathTimer;					// Таймер анимации "дыхание" для смены иконки
	int m_breathDuration = 0;				// Время на 1 полный цикл анимации (мс)
	int m_breathElapsed = 0;				// Пройденное время с начала анимации (мс)
};
