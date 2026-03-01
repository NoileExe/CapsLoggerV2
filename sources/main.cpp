
// CapsLoggerV2
// Copyright (c) 2026 Белобрагин Антон Игоревич / Belobragin Anton Igorevich
// MIT License


#include <QApplication>           // for QApplication
#include <QIcon>                  // for QIcon
#include <QLocalServer>           // for QLocalServer
#include <QLocalSocket>           // for QLocalSocket
#include <QMessageBox>            // for QMessageBox
#include <QObject>                // for QObject
#include <QString>                // for QString, operator+
#include "gui/KeysLoggerTray.h"
#include "gui/SettingsDefines.h"

//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	app.setOrganizationName("CAPSLoggerV2");
    app.setApplicationName("CAPSLoggerV2");
    app.setApplicationVersion("2.0");
	app.setWindowIcon(QIcon(defaultThemePath() + capsFileName()));
	app.setQuitOnLastWindowClosed(false);

	CapsLoggerSettings::initSettings();

//----------------------Предотвращение повторного запуска----------------------
    const QString serverName = QString("CAPSLoggerV%0_Singleton").arg(app.applicationVersion());
	
	QLocalSocket socket;
	socket.connectToServer(serverName);
	if (socket.waitForConnected(500))
	{
		QMessageBox::warning(nullptr, QObject::tr("Повторный запуск"),
			QObject::tr("Приложение уже работает.\nНельзя запустить несколько копий одновременно."),
			QMessageBox::Ok);
		return 0;
	}

	QLocalServer::removeServer(serverName);
	
	QLocalServer server;
	if (!server.listen(serverName)) {
		QMessageBox::warning(nullptr, QObject::tr("Ошибка"),
			QObject::tr("Не удалось создать сервер:\n%1").arg(server.errorString()),
			QMessageBox::Ok);
		return 1;
	}
//----------------------Предотвращение повторного запуска----------------------

	KeysLoggerTray capsLogger;

	return app.exec();
}
