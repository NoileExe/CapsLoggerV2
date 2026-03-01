
#include "KeysLoggerTray.h"
#include <QtMath>
#include <QtCore>
#include <QAction>						// for QAction
#include <QApplication>					// for QApplication, qApp
#include <QColor>						// for QColor
#include <QDialog>						// for QDialog
#include <QMenu>						// for QMenu
#include <QMessageBox>					// for QMessageBox
#include <QPainter>						// for QPainter
#include <QPixmap>						// for QPixmap
#include <QSettings>					// for QSettings
#include <QStyle>						// for QStyle
#include <QTimer>						// for QTimer
#include <QVariant>						// for QVariant, QTypeInfo<>::isLarge
#include <QWidget>						// for QWidget
#include <QtConfig>						// for QT_VERSION_STR
#include "SettingsDefines.h"
#include "SettingsWidget.h"
#include "TrayMessagePopup.h"
#include "../platform/KeysLoggerProvider.h"

#ifdef Q_OS_LINUX
	#include <QDBusArgument>			// for QDBusArgument, QTypeInfo<>:...
	#include <QDBusConnection>			// for QDBusConnection, CallMode
	#include <QDBusInterface>			// for QDBusInterface
	#include <QDBusMessage>				// for QDBusMessage
	#include <QByteArray>				// for QByteArray
	#include <QCoreApplication>				// for QCoreApplication
	#include <QImage>						// for QImage
	#include <QList>						// for QList
	#include <QStringList>					// for QStringList
	#include <QVariantMap>					// for QVariantMap
#endif

//-----------------------------------------------------------------------------

KeysLoggerTray::KeysLoggerTray(QWidget* pwgt /*= 0*/)
	: QSystemTrayIcon(pwgt)
	, m_pContextMenu(nullptr)
	, m_pPopupMessage(nullptr)
	, m_pSettingsWindow(nullptr)
	, m_keysProvider(nullptr)
	, m_breathTimer(nullptr)
	, m_breathDuration(0)
{
	m_keysProvider = KeysLoggerProvider::create(this);

	connect(m_keysProvider, &KeysLoggerProvider::keyFunctionChanged,
			this, &KeysLoggerTray::slotChangeKeyState);

	qApp->installEventFilter(this);

	//-------------------------------------------------------------------------

#ifdef Q_OS_LINUX
	if (CapsLoggerSettings::isSysMessagesAvailable())
	{
		// Подписываемся на событие закрытия системным popup-сообщений (для KDE)
		QDBusConnection::sessionBus().connect(
			QString(),							// Пустая строка = слушать от любого отправителя
			"/org/freedesktop/Notifications",
			"org.freedesktop.Notifications",
			"NotificationClosed",
			this,
			SLOT(onNotificationClosed(uint,uint))
		);
	}
#endif

	//-------------------------------------------------------------------------

	m_breathDuration = QSettings().value(timeIconSettings()).toInt();

	m_breathTimer = new QTimer(this);
	m_breathTimer->setInterval(40);		// ~25 кадров/сек
#ifdef Q_OS_LINUX
	m_breathTimer->setInterval(80);
#endif
	connect(m_breathTimer, &QTimer::timeout, this, &KeysLoggerTray::slotBreathTick);

	//-------------------------------------------------------------------------

	// Инициализируем информацию о состоянии ф-ций кнопок на момент запуска
	bool isCapsTrackingOn = QSettings().value(capsTrackSettings()).toBool();
	bool isNumTrackingOn = QSettings().value(numTrackSettings()).toBool();
	bool isScrollTrackingOn = QSettings().value(scrollTrackSettings()).toBool();

	m_keyStates.set(static_cast<int>(KB_button::CAPS),
					m_keysProvider->isCapsLockOn() && isCapsTrackingOn);
	m_keyStates.set(static_cast<int>(KB_button::NUM),
					m_keysProvider->isNumLockOn() && isNumTrackingOn);
	m_keyStates.set(static_cast<int>(KB_button::SCROLL),
					m_keysProvider->isScrollLockOn() && isScrollTrackingOn);

	//-------------------------------------------------------------------------

	buildMenu();
	
	//-------------------------------------------------------------------------

	QIcon infoIcon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
	QString tooltip = createTooltip();

	int time_message = QSettings().value(timeMsgSettings()).toInt();
	bool isUseSystemMessage = false;
	{
		CapsLoggerMessageType msgType = static_cast<CapsLoggerMessageType>( QSettings().value(msgTypeSettings()).toInt() );
		isUseSystemMessage = msgType == CapsLoggerMessageType::SYSTEM  &&  CapsLoggerSettings::isSysMessagesAvailable();
	}

	if (isUseSystemMessage)
	{
		showSystemNotify(tr("Текущие состояния"), tooltip, infoIcon, time_message);
	}
	else
	{
		// Удаляется сам
		m_pPopupMessage = new TrayMessagePopup(tr("Текущие состояния"), tooltip, infoIcon, time_message,
								m_keyStates.none() ? QColor(66, 133, 244)  :  QColor(155, 27, 48));
		connect(m_pPopupMessage, &QObject::destroyed,
				this, [this]() { m_pPopupMessage = nullptr; });
	}

	setToolTip(tooltip);
	startNewAnimation();
	show();
}

KeysLoggerTray::~KeysLoggerTray()
{
	if (m_breathTimer && m_breathTimer->isActive())
		m_breathTimer->stop();

	if (m_pPopupMessage)
	{
		if (m_keysProvider)
			disconnect(m_keysProvider, &KeysLoggerProvider::keyFunctionChanged,
						this, &KeysLoggerTray::slotChangeKeyState);

		m_pPopupMessage->deleteLater();
	}

#ifdef Q_OS_LINUX
	if (m_lastNotificationId)
	{
		 QDBusInterface notify("org.freedesktop.Notifications",
								"/org/freedesktop/Notifications",
								"org.freedesktop.Notifications",
								QDBusConnection::sessionBus());

		if (notify.isValid())
		{
			notify.call(QDBus::NoBlock, "CloseNotification", m_lastNotificationId);
			m_lastNotificationId = 0;
		}
	}
#endif

	if (m_pContextMenu)
	{
		m_pContextMenu->deleteLater();
		m_pContextMenu = nullptr;
	}	

	if (m_pSettingsWindow)
	{
		m_pSettingsWindow->reject();

		m_pSettingsWindow->deleteLater();
	}
}

//-----------------------------------------------------------------------------

void KeysLoggerTray::buildMenu()
{
	if (m_pContextMenu)
		m_pContextMenu->clear();
	else
	{
		m_pContextMenu = new QMenu();
		connect(m_pContextMenu, &QMenu::aboutToShow, this, [&]()
		{
			if (m_pSettingsWindow  &&  m_pSettingsWindow->isVisible())
			{
				// Блокируем показ меню и издаём звуковой сигнал
				QApplication::beep();
				slotShowSettings();
				m_pContextMenu->close();
			}
		});
	}


	if (m_keysProvider->supportsCaps())
	{
		m_pActCheckCaps = m_pContextMenu->addAction("CAPS Lock");
		m_pActCheckCaps->setToolTip(tr("Включить/выключить CAPS Lock"));
		m_pActCheckCaps->setCheckable(true);
		m_pActCheckCaps->setChecked(m_keysProvider->isCapsLockOn());
#ifdef Q_OS_LINUX
		m_pActCheckCaps->setEnabled(CapsLoggerSettings::isX11Environment());
#endif
		connect(m_pActCheckCaps, &QAction::triggered, this, &KeysLoggerTray::slotSetButtonState);
	}
	if (m_keysProvider->supportsNum())
	{
		m_pActCheckNum = m_pContextMenu->addAction("NUM Lock");
		m_pActCheckNum->setToolTip(tr("Включить/выключить NUM Lock"));
		m_pActCheckNum->setCheckable(true);
		m_pActCheckNum->setChecked(m_keysProvider->isNumLockOn());
#ifdef Q_OS_LINUX
		m_pActCheckNum->setEnabled(CapsLoggerSettings::isX11Environment());
#endif
		connect(m_pActCheckNum, &QAction::triggered, this, &KeysLoggerTray::slotSetButtonState);
	}
	if (m_keysProvider->supportsScroll())
	{
		m_pActCheckScroll = m_pContextMenu->addAction("SCROLL Lock");
		m_pActCheckScroll->setToolTip(tr("Включить/выключить SCROLL Lock"));
		m_pActCheckScroll->setCheckable(true);
		m_pActCheckScroll->setChecked(m_keysProvider->isScrollLockOn());
#ifdef Q_OS_LINUX
		m_pActCheckScroll->setEnabled(CapsLoggerSettings::isX11Environment());
#endif
		connect(m_pActCheckScroll, &QAction::triggered, this, &KeysLoggerTray::slotSetButtonState);
	}
	if (!m_pContextMenu->isEmpty())
	{
		m_pContextMenu->addSeparator();
	}

	QAction* pBtnSettings = m_pContextMenu->addAction(tr("Настройки..."));
	QAction* pBtnAbout = m_pContextMenu->addAction(tr("О приложении"));
	m_pContextMenu->addSeparator();
	QAction* pBtnQuit = m_pContextMenu->addAction(tr("Выход"));
		
	connect(pBtnSettings, &QAction::triggered, this, &KeysLoggerTray::slotShowSettings);
	connect(pBtnAbout, &QAction::triggered, this, &KeysLoggerTray::slotShowAbout);
	connect(pBtnQuit, &QAction::triggered, qApp, &QApplication::quit);

	setContextMenu(m_pContextMenu);
}

//-----------------------------------------------------------------------------

void KeysLoggerTray::startNewAnimation()
{
	if (m_breathTimer && m_breathTimer->isActive())
		m_breathTimer->stop();

	QString currentIconFile = QSettings().value(themeSettings()).toString();
	if ( !SettingsWidget::isThemeCorrect(currentIconFile) )
		currentIconFile = defaultThemePath();

	// За текущую считается первая подходящая иконка
	// в очередности Caps -> Num -> Scroll -> Caps
	if (m_keyStates.none())
	{
		m_currentIcon = KB_button::UNKNOWN;
		currentIconFile += allOffFileName();
	}
	else if (isSet(KB_button::CAPS))
	{
		m_currentIcon = KB_button::CAPS;
		currentIconFile += capsFileName();
	}
	else if (isSet(KB_button::NUM))
	{
		m_currentIcon = KB_button::NUM;
		currentIconFile += numFileName();
	}
	else if (isSet(KB_button::SCROLL))
	{
		m_currentIcon = KB_button::SCROLL;
		currentIconFile += scrollFileName();
	}

	setIcon(QPixmap(currentIconFile));

	// Более одного активной функции кнопок
	if (1 < m_keyStates.count())
	{
		// Первый цикл начинается с иконки без прозрачности
		m_breathElapsed = m_breathDuration / 2;
		m_breathTimer->start();
	}
}

bool KeysLoggerTray::isSet(KB_button btn) const
{
	return m_keyStates.test(static_cast<int>(btn));
}

QString KeysLoggerTray::createTooltip() const
{
	QString tooltipTxt;

	if (isSet(KB_button::CAPS))
		tooltipTxt = tr("CAPS Lock включен");
	if (isSet(KB_button::NUM))
		tooltipTxt += (!tooltipTxt.isEmpty() ? "\n" : "") + tr("NUM Lock включен");
	if (isSet(KB_button::SCROLL))
		tooltipTxt += (!tooltipTxt.isEmpty() ? "\n" : "") + tr("SCROLL Lock включен");

	if (tooltipTxt.isEmpty())
		tooltipTxt = tr("Все выключены");

	return tooltipTxt;
}

//-----------------------------------------------------------------------------

void KeysLoggerTray::slotChangeKeyState(KB_button btn, bool on)
{
	if (!isVisible())
		return;

	if (btn == KB_button::CAPS  &&  m_pActCheckCaps)
		m_pActCheckCaps->setChecked(on);
	else if (btn == KB_button::NUM  &&  m_pActCheckNum)
		m_pActCheckNum->setChecked(on);
	else if (btn == KB_button::SCROLL  &&  m_pActCheckScroll)
		m_pActCheckScroll->setChecked(on);

	//-------------------------------------------------------------------------

	bool isCapsTrackingOn = QSettings().value(capsTrackSettings()).toBool();
	bool isNumTrackingOn = QSettings().value(numTrackSettings()).toBool();
	bool isScrollTrackingOn = QSettings().value(scrollTrackSettings()).toBool();
	bool isCurrentKeyTrackingOn = (isCapsTrackingOn		&&  btn == KB_button::CAPS) ||
								  (isNumTrackingOn		&&  btn == KB_button::NUM) ||
								  (isScrollTrackingOn	&&  btn == KB_button::SCROLL);

	//Вывод сообщения только если кнопка отслеживается
	if (isCurrentKeyTrackingOn)
	{
		if (m_pPopupMessage)
			delete m_pPopupMessage;

		QString keyNameState;
		QString currentIconFile = QSettings().value(themeSettings()).toString();
		if ( !SettingsWidget::isThemeCorrect(currentIconFile) )
			currentIconFile = defaultThemePath();

		switch (btn)
		{
			case KB_button::CAPS:
				keyNameState = "CAPS Lock";
				currentIconFile += capsFileName();
				break;

			case KB_button::NUM:
				keyNameState = "NUM Lock";
				currentIconFile += numFileName();
				break;

			case KB_button::SCROLL:
				keyNameState = "SCROLL Lock";
				currentIconFile += scrollFileName();
				break;

			default:
				keyNameState = "UNKNOWN key";
				return;
		}

		keyNameState += on ? tr(" включен") : tr(" выключен");

		int time_message = QSettings().value(timeMsgSettings()).toInt();
		bool isUseSystemMessage = false;
		{
			CapsLoggerMessageType msgType = static_cast<CapsLoggerMessageType>( QSettings().value(msgTypeSettings()).toInt() );
			isUseSystemMessage = msgType == CapsLoggerMessageType::SYSTEM  &&  CapsLoggerSettings::isSysMessagesAvailable();
		}

		if (isUseSystemMessage)
		{
			showSystemNotify(tr("Изменение состояния"), keyNameState, QIcon(currentIconFile), time_message);
		}
		else
		{
			m_pPopupMessage = new TrayMessagePopup(tr("Изменение состояния"), keyNameState, QIcon(currentIconFile), time_message,
													on ? QColor(155, 27, 48)  :  QColor(66, 133, 244));
			connect(m_pPopupMessage, &QObject::destroyed, this, [this]() { m_pPopupMessage = nullptr; });
		}
	}

	m_keyStates.set(static_cast<int>(btn), on  &&  isCurrentKeyTrackingOn);
	setToolTip( createTooltip() );

	//-------------------------------------------------------------------------

	if (m_keyStates.count() <= 1)
	{
		startNewAnimation();
	}
	else if (m_breathTimer)
	{
		// Таймер не активен - ранее была выбрана одна иконка
		if (!m_breathTimer->isActive())
		{
			// Первый цикл начинается с иконки без прозрачности
			m_breathElapsed = m_breathDuration / 2;
			m_breathTimer->start();
		}

		// Проверяем если текущая иконка была выключена, но учавствовала ранее в анимации -
		// нужно удостоверится что она не является текущей чтобы актуализировать информацию
		else if (!on  &&  m_currentIcon == btn)
		{
			startNewAnimation();
		}
	}
}

void KeysLoggerTray::showSystemNotify(const QString& title, const QString& msg, 
										const QIcon& icon, int timeoutMs)
{
	if (!CapsLoggerSettings::isSysMessagesAvailable())		return;

#ifdef Q_OS_LINUX
	QDBusInterface notify("org.freedesktop.Notifications",
							"/org/freedesktop/Notifications",
							"org.freedesktop.Notifications",
							QDBusConnection::sessionBus(), this);
				
	if (notify.isValid())
	{
		QVariantMap hint;
		QPixmap pixmap = icon.pixmap(48);
		if (!pixmap.isNull())
		{
			QImage img = pixmap.toImage().convertToFormat(QImage::Format_RGBA8888);
			if (!img.isNull())
			{
				const uchar* bits = img.constBits();
				int rowstride = img.width() * 4;
				int dataSize = rowstride * img.height();
				
				QByteArray data((const char*)bits, dataSize);

				QDBusArgument arg;
				arg.beginStructure();
					arg << img.width();						// int32: width
					arg << img.height();					// int32: height
					arg << rowstride;						// int32: rowstride
					arg << true;							// boolean: has_alpha
					arg << 8;								// int32: bits_per_sample
					arg << 4;								// int32: channels (RGBA)
					arg.beginArray(qMetaTypeId<uchar>());	// array of bytes
						for (int i = 0; i < dataSize; ++i) {
							arg << static_cast<uchar>(bits[i]);
						}
					arg.endArray();
				arg.endStructure();

				hint.insert("image-data", QVariant::fromValue(arg));
				hint.insert("transient", true);
				hint.insert("suppress-text", false);
			}
		}	
		
		// Аргументы метода Notify
		QList<QVariant> args;
		args << QCoreApplication::applicationName() << m_lastNotificationId << QString()
			 << title << msg << QStringList() << hint << timeoutMs;

		QDBusMessage reply = notify.callWithArgumentList(QDBus::Block, "Notify", args);

		// Сохраняем ID для следующей замены или закрытия
		if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty())
		{
			m_lastNotificationId = reply.arguments().first().toUInt();
		}
		
		if (reply.type() != QDBusMessage::ErrorMessage)
			return;
	}
#endif

	showMessage(title, msg, icon, timeoutMs);
}

#ifdef Q_OS_LINUX
void KeysLoggerTray::onNotificationClosed(uint id, uint reason)
{
	if (id != m_lastNotificationId)		return;

	// reason: 1=expired (таймаут), 2=dismissed by user, 3=CloseNotification, 4=undefined
	if (reason == 1  ||  reason == 2)
	{
		m_lastNotificationId = 0;	// Сброс ID
	}
}
#endif

//-----------------------------------------------------------------------------

void KeysLoggerTray::slotBreathTick()
{
	if (m_keyStates.none())
	{
		if (m_breathTimer && m_breathTimer->isActive())
			m_breathTimer->stop();

		return;
	}


	m_breathElapsed += m_breathTimer->interval();

	// Определение следующей иконки для анимации
	if (m_breathElapsed >= m_breathDuration)
	{
		if (m_breathTimer && m_breathTimer->isActive())
			m_breathTimer->stop();

		// Анимация в очередности
		// Caps -> Num -> Scroll -> Caps
		int val = static_cast<int>(m_currentIcon) + 1;
		for (int edgeVal = static_cast<int>(KB_button::UNKNOWN);
			!m_keyStates.test(val);
			val = (val == edgeVal) ? 0 : val + 1)
		{
		}

		m_currentIcon = static_cast<KB_button>(val);
		m_breathElapsed = 0;
		m_breathTimer->start();
		return;
	}

	//-------------------------------------------------------------------------

	QString currentIconFile = QSettings().value(themeSettings()).toString();
	if ( !SettingsWidget::isThemeCorrect(currentIconFile) )
		currentIconFile = defaultThemePath();

	if (m_currentIcon == KB_button::CAPS)			currentIconFile += capsFileName();
	else if (m_currentIcon == KB_button::NUM)		currentIconFile += numFileName();
	else if (m_currentIcon == KB_button::SCROLL)	currentIconFile += scrollFileName();

	// Применяем эффект к текущей иконке
	QPixmap base{ currentIconFile };
	if (!base.isNull())
	{
		// Фаза "дыхания": 0.0 → 1.0 → 0.0
		double t = static_cast<double>(m_breathElapsed) / m_breathDuration;
		double alpha = sin(t * M_PI);

		applyAlpha(base, alpha);
		setIcon(base);
	}
}

void KeysLoggerTray::applyAlpha(QPixmap &base, double alpha) const
{
	if (base.isNull())
		return;

	QPainter painter(&base);
	painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	painter.fillRect( base.rect(), QColor(255, 255, 255, static_cast<int>(alpha * 255)) );
	painter.end();
}

//-----------------------------------------------------------------------------

void KeysLoggerTray::slotSetButtonState()
{
	QAction *button = qobject_cast<QAction*>(sender());
	if (!button)
		return;

	if (button == m_pActCheckCaps)
	{
		m_keysProvider->pressCapsLock();
	}
	else if (button == m_pActCheckNum)
	{
		m_keysProvider->pressNumLock();
	}
	else if (button == m_pActCheckScroll)
	{
		m_keysProvider->pressScrollLock();
	}
}


void KeysLoggerTray::slotShowSettings()
{
	if (m_pSettingsWindow)
	{
		m_pSettingsWindow->activateWindow();	// Фокус на уже открытое окно
		m_pSettingsWindow->raise();
		return;
	}

	m_pSettingsWindow = new SettingsWidget{ nullptr };
	if (m_pSettingsWindow->exec() == QDialog::Accepted)
	{
		//------------------Обновление отслеживания значений-------------------
		bool isCapsTrackingOn = QSettings().value(capsTrackSettings()).toBool();
		bool isNumTrackingOn = QSettings().value(numTrackSettings()).toBool();
		bool isScrollTrackingOn = QSettings().value(scrollTrackSettings()).toBool();

		m_keyStates.set(static_cast<int>(KB_button::CAPS),
						m_keysProvider->isCapsLockOn() && isCapsTrackingOn);
		m_keyStates.set(static_cast<int>(KB_button::NUM),
						m_keysProvider->isNumLockOn() && isNumTrackingOn);
		m_keyStates.set(static_cast<int>(KB_button::SCROLL),
						m_keysProvider->isScrollLockOn() && isScrollTrackingOn);
		//------------------Обновление отслеживания значений-------------------
		//------------------Смена языка (перезагрузка строк)-------------------
		if (m_pPopupMessage)
			delete m_pPopupMessage;

		setToolTip( createTooltip() );
		buildMenu();
		//------------------Смена языка (перезагрузка строк)-------------------

		m_breathDuration = QSettings().value(timeIconSettings()).toInt();
		startNewAnimation();
	}
	
	m_pSettingsWindow->deleteLater();
	m_pSettingsWindow = nullptr;
}

void KeysLoggerTray::slotShowAbout()
{
	QString text = tr(
		"<b>CAPSLoggerV2</b><br><br>"
		"Утилита для отслеживания состояний Caps/Num/Scroll Lock в трее.<br><br>"
		"<i>Лицензия:</i> MIT<br>"
		"<i>Использует Qt %1 (LGPL v3)</i><br><br>"
		"<i>Автор:</i> Антон Белобрагин<br><br>"
		"<a href='https://github.com/NoileExe/CAPSLoggerV2'>GitHub</a> | "
		"<a href='https://www.qt.io/licensing/'>Qt Licensing</a>"
	).arg(QT_VERSION_STR);

	// Сначала без иконки чтобы не было сигнала beep
	QMessageBox about(QMessageBox::NoIcon,  "О приложении", text,  QMessageBox::Ok, nullptr);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QIcon icon = about.style()->standardIcon(QStyle::SP_MessageBoxInformation);
	about.setIconPixmap(icon.pixmap(48, 48));
#else
	about.setIconPixmap(QMessageBox::standardIcon(QMessageBox::Information));
#endif
	about.setWindowIcon( QIcon(defaultThemePath() + capsFileName()) );
	about.setTextFormat(Qt::RichText);
	about.setWindowModality(Qt::WindowModal);

	about.exec();
}
