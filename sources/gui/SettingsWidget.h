
#pragma once


#include <QWidget>  // for QWidget
#include <QObject>  // for Q_OBJECT, slots
#include <QDialog>
#include <QString>

//-----------------------------------------------------------------------------

class QLabel;

namespace Ui { class SettingsWidget; }

//-----------------------------------------------------------------------------

class SettingsWidget : public QDialog
{
Q_OBJECT

public:
	explicit SettingsWidget(QWidget *parent = nullptr);
	~SettingsWidget();

	void accept() override;
	void reject() override;

	static bool isThemeCorrect(const QString& themePath);

private slots:
	void slotUpdateIconsPreview(int index);
	void slotKeyTrackingChanged(int state);
	void slotMessageTypeChanged(bool checked);
	void slotLocationChanged(int index);
	void slotLanguageChanged(int index);
	void slotValueChanged(int num);

private:
	void initIconThemes();
	void initTracking();
	void initMessageType();
	void initMessageLocations();
	void initLanguages();
	
	void initIcon(QLabel* iconWgt, const QString& iconPath);

private:
	Ui::SettingsWidget *ui;
};