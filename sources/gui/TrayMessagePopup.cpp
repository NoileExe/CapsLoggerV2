
#include "TrayMessagePopup.h"
#include <QtCore/qobjectdefs.h>  // for CheckCompatibleArguments<>::value
#include <QAbstractAnimation>    // for QAbstractAnimation
#include <QApplication>          // for QApplication
#include <QFlags>                // for QFlags
#include <QGradientStop>         // for QBrush
#include <QGuiApplication>       // for QGuiApplication
#include <QHBoxLayout>           // for QHBoxLayout
#include <QLabel>                // for QLabel
#include <QPainter>              // for QPainter
#include <QPalette>              // for QPalette
#include <QPen>                  // for QPen
#include <QPoint>                // for QPoint, operator+, operator-
#include <QPropertyAnimation>    // for QPropertyAnimation
#include <QRect>                 // for QRect
#include <QScreen>               // for QScreen
#include <QSettings>             // for QSettings
#include <QSize>                 // for QSize, operator*
#include <QTimer>                // for QTimer
#include <QVBoxLayout>           // for QVBoxLayout
#include <QVariant>              // for QVariant
#include <QtCore>                // for WindowType, WidgetAttribute, operator|
#include "SettingsDefines.h"     // for CapsLoggerMessageLocation, msgLocati...

//-----------------------------------------------------------------------------

TrayMessagePopup::TrayMessagePopup(const QString &title, const QString &text,
									const QIcon& icon, int timeoutMs,
									const QColor& msgColor)
	: QWidget(nullptr)
	, m_closeTimer(nullptr)
	, m_timeoutMs(timeoutMs)
	, m_fadeAnimation(nullptr)
	, m_lineColor(msgColor)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setWindowOpacity(0.85); // Прозрачность 85%
	
	// Кликабельность
	setMouseTracking(true);
	
	// Layout
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(15, 12, 15, 12);
	layout->setSpacing(8);
	
	// Заголовок с иконкой
	QHBoxLayout *titleLayout = new QHBoxLayout();
	
	if (!icon.isNull())
	{
		QLabel *iconLabel = new QLabel();
		
		QSize iconSize = QSize(32, 32) * devicePixelRatioF();
		iconLabel->setPixmap(icon.pixmap(iconSize));
		iconLabel->setFixedSize(32, 32);	// логические пиксели
		
		titleLayout->addWidget(iconLabel);
	}

	QPalette palette = QApplication::palette();
	QString titleColor = palette.color(QPalette::WindowText).name();
	QString textColor = palette.color(QPalette::Text).name();

	QLabel *titleLabel = new QLabel(title);
	titleLabel->setStyleSheet( QString("font-weight: bold; font-size: 12px; color: %1;").arg(titleColor) );
	titleLayout->addWidget(titleLabel);
	titleLayout->addStretch();
	
	// Сообщение
	QLabel *messageLabel = new QLabel(text);
	messageLabel->setStyleSheet( QString("font-size: 12px; color: %1;").arg(textColor) );
	messageLabel->setWordWrap(true);
	messageLabel->setMaximumWidth(150);
	
	layout->addLayout(titleLayout);
	layout->addWidget(messageLabel);
	
	setFixedWidth(220);
	adjustSize();


	// Позиционируем относительно настроек с учётом панели задач
	QPoint pos;
	{
		QScreen *screen = QGuiApplication::primaryScreen();
		QRect screenGeom = screen->availableGeometry();
		
		
		int w = width();			// Ширина окна сообщения
		int h = height();			// Высота окна сообщения
		const int margin = 20;		// Отступ от края экрана

		using MsgLocation = CapsLoggerMessageLocation;
		MsgLocation loc = static_cast<MsgLocation>( QSettings().value(msgLocationSettings()).toInt() );

		if (loc == MsgLocation::TOP_LEFT)
			pos =  screenGeom.topLeft() + QPoint(margin, margin);
		else if (loc == MsgLocation::TOP_RIGHT)
			pos =  screenGeom.topRight() - QPoint(w + margin, -margin);
		else if (loc == MsgLocation::BOTTOM_LEFT)
			pos =  screenGeom.bottomLeft() + QPoint(margin, -h - margin);
		else if (loc == MsgLocation::BOTTOM_RIGHT)
			pos =  screenGeom.bottomRight() - QPoint(w + margin, h + margin);
		else if (loc == MsgLocation::TOP_CENTER)
			pos =  screenGeom.topLeft() + QPoint((screenGeom.width() - w) / 2, margin);
		else if (loc == MsgLocation::BOTTOM_CENTER)
			pos =  screenGeom.bottomLeft() + QPoint((screenGeom.width() - w) / 2, -h - margin);
	}
	move(pos);
	
	// Показ
	setWindowOpacity(0.0);
	show();
	raise();

	// Анимация открытия
	m_fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);
	m_fadeAnimation->setDuration(timeoutMs > 1500 || timeoutMs <= 0 ? 1500  :  timeoutMs / 2);
	m_fadeAnimation->setStartValue(0.0);
	m_fadeAnimation->setEndValue(0.82);
	m_fadeAnimation->start();


	// Таймер закрытия
	if (timeoutMs > 0)
	{
		m_closeTimer = new QTimer(this);
		m_closeTimer->setSingleShot(true);
		connect(m_closeTimer, &QTimer::timeout, this, &TrayMessagePopup::startFadeOut);
		m_closeTimer->start(timeoutMs);
	}
}

TrayMessagePopup::~TrayMessagePopup()
{
	// Останавливаем таймер при удалении
	if (m_closeTimer)
		m_closeTimer->stop();
	
	if (m_fadeAnimation->state() == QAbstractAnimation::Running)
	{
		disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::deleteLater);
		m_fadeAnimation->stop();
		m_isFading = false;
	}
}

//-----------------------------------------------------------------------------

void TrayMessagePopup::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	
	// Фон без прозрачности
	painter.setBrush(QColor(255, 255, 255));
	painter.setPen(QPen(QColor(200, 200, 200, 200), 1));
	
	// Левая цветная полоска
	painter.setBrush(m_lineColor);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(QRect(0, 0, 4, height()), 2, 2);
}

//-----------------------------------------------------------------------------

void TrayMessagePopup::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);

	if (m_closeTimer  &&  m_closeTimer->isActive())
		m_closeTimer->stop();

	if (m_fadeAnimation->state() == QAbstractAnimation::Running)
	{
		disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::deleteLater);
		m_fadeAnimation->stop();
	}

	deleteLater(); // Закрытие по клику
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void TrayMessagePopup::enterEvent(QEnterEvent *event)
#else
void TrayMessagePopup::enterEvent(QEvent *event)
#endif
{
	if (m_closeTimer  &&  m_closeTimer->isActive())
		m_closeTimer->stop();

	if (m_fadeAnimation->state() == QAbstractAnimation::Running)
	{
		disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::deleteLater);
		m_fadeAnimation->stop();
		m_isFading = false;
	}

	setWindowOpacity(1.0);		// Полная непрозрачность при наведении

	QWidget::enterEvent(event);
}

void TrayMessagePopup::leaveEvent(QEvent* event)
{
	setWindowOpacity(0.85);		// Возвращаем прозрачность

	// Перезапускаем таймер при уходе курсора
	if (m_closeTimer  &&  m_timeoutMs > 0)
		m_closeTimer->start(m_timeoutMs);

	QWidget::leaveEvent(event);
}

//-----------------------------------------------------------------------------

void TrayMessagePopup::startFadeOut()
{
	if (m_isFading || !isVisible())
		return;

	m_isFading = true;
	if (m_closeTimer  &&  m_closeTimer->isActive())
		m_closeTimer->stop();

	// Останавливаем текущую анимацию (если идёт появление)
	if (m_fadeAnimation->state() == QAbstractAnimation::Running)
		m_fadeAnimation->stop();

	connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::deleteLater);

	// Запускаем анимацию исчезновения
	m_fadeAnimation->setStartValue(windowOpacity());
	m_fadeAnimation->setEndValue(0.0);
	m_fadeAnimation->start();
}
