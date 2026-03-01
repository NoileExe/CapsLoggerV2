
#pragma once


#include <QObject>
#include <QString>  // for QString

//-----------------------------------------------------------------------------

class KeysLoggerProvider : public QObject {
Q_OBJECT

public:
	enum Keyboard
	{
		CAPS	= 0,
		NUM		= 1,
		SCROLL	= 2,
		UNKNOWN	= 3,
	};

	explicit KeysLoggerProvider(QObject *parent = nullptr) : QObject(parent) {}
	virtual ~KeysLoggerProvider() = default;

	// Текущее состояние (для инициализации интерфейса)
	virtual bool isCapsLockOn() const = 0;
	virtual bool isNumLockOn() const = 0;
	virtual bool isScrollLockOn() const = 0;
	
	// Программное переключение состояний (имитация нажатия клавиши)
	virtual void pressCapsLock() = 0;
	virtual void pressNumLock() = 0;
	virtual void pressScrollLock() = 0;

	// Поддерживаются ли операционной системой состояния
	virtual bool supportsCaps() const = 0;
	virtual bool supportsNum() const = 0;
	virtual bool supportsScroll() const = 0;

	// Фабрика — создаёт нужную реализацию под текущую ОС
	static KeysLoggerProvider* create(QObject *parent = nullptr);

signals:
	void keyFunctionChanged(KeysLoggerProvider::Keyboard btn, bool on);
};
