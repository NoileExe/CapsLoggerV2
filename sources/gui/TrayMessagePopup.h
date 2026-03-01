
#pragma once


#include <QColor>             // for QColor
#include <QIcon>              // for QIcon
#include <QNonConstOverload>  // for QT_VERSION, QT_VERSION_CHECK
#include <QObject>            // for QObject, Q_OBJECT, slots
#include <QString>            // for QString
#include <QTimer>             // for QTimer
#include <QWidget>            // for QWidget

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	#include <QEnterEvent>
#else
	class QEvent;
#endif

//-----------------------------------------------------------------------------

class QMouseEvent;
class QPaintEvent;
class QTimer;
class QPropertyAnimation;

//-----------------------------------------------------------------------------

class TrayMessagePopup : public QWidget
{
Q_OBJECT

public:
	TrayMessagePopup(const QString &title, const QString &text,
						const QIcon &icon = QIcon(), int timeoutMs = 5000,
						const QColor& msgColor = QColor(66, 133, 244));		//Цветная полоска по умолчанию синяя

	~TrayMessagePopup();

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		void enterEvent(QEnterEvent *event) override;
	#else
		void enterEvent(QEvent *event) override;
	#endif
	void leaveEvent(QEvent *event) override;

private slots:
	void startFadeOut();	// Запуск анимации затухания

private:
	QTimer *m_closeTimer;	// Таймер закрытия
	int m_timeoutMs;		// Время до закрытия
	
	QColor m_lineColor;		// Время до закрытия
	
	QPropertyAnimation *m_fadeAnimation;	// Анимация закрытия
	bool m_isFading = false;				// Флаг для предотвращения повторных вызовов анимации
};
