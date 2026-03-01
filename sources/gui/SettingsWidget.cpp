
#include "SettingsWidget.h"
#include <QtCore/qobjectdefs.h>  // for FunctionPointer<>::ArgumentCount
#include <QApplication>          // for QApplication
#include <QCheckBox>             // for QCheckBox
#include <QComboBox>             // for QComboBox
#include <QCoreApplication>      // for QCoreApplication
#include <QDialogButtonBox>      // for QDialogButtonBox
#include <QDir>                  // for QDir, operator|
#include <QFile>                 // for QFile
#include <QIcon>                 // for QIcon
#include <QLabel>                // for QLabel
#include <QPushButton>           // for QPushButton
#include <QRadioButton>          // for QRadioButton
#include <QSettings>             // for QSettings
#include <QSize>                 // for QSize
#include <QSpinBox>              // for QSpinBox
#include <QStringList>           // for QStringList
#include <QStyle>                // for QStyle
#include <QVariant>              // for QVariant
#include <QtCore>                // for ItemDataRole, QOverload, Q_OS_LINUX
#include "SettingsDefines.h"
#include "ui_SettingsWidget.h"

//-----------------------------------------------------------------------------

SettingsWidget::SettingsWidget(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SettingsWidget)
{
	ui->setupUi(this);
	setFixedSize(350, 550);
	setWindowModality(Qt::ApplicationModal);
	setWindowTitle(tr("Настройки"));

	connect(ui->iconTheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &SettingsWidget::slotUpdateIconsPreview);
	initIconThemes();

	initTracking();
	connect(ui->capsCheckBox, &QCheckBox::stateChanged,
			this, &SettingsWidget::slotKeyTrackingChanged);
	connect(ui->numCheckBox, &QCheckBox::stateChanged,
			this, &SettingsWidget::slotKeyTrackingChanged);
	connect(ui->scrollCheckBox, &QCheckBox::stateChanged,
			this, &SettingsWidget::slotKeyTrackingChanged);

	initMessageType();
	connect(ui->systemRadioButton, &QRadioButton::toggled,
			this, &SettingsWidget::slotMessageTypeChanged);
	connect(ui->customRadioButton, &QRadioButton::toggled,
			this, &SettingsWidget::slotMessageTypeChanged);

	connect(ui->msgLocation, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &SettingsWidget::slotLocationChanged);
	initMessageLocations();

	int time_icon = QSettings().value(timeIconSettings()).toInt();
	ui->iconTimeSpinBox->setValue(time_icon);
	ui->iconTimeSpinBox->setMaximum( static_cast<int>(maxTimeMs) );
	connect(ui->iconTimeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
			this, &SettingsWidget::slotValueChanged);

	int time_message = QSettings().value(timeMsgSettings()).toInt();
	ui->msgTimeSpinBox->setValue(time_message);
	ui->msgTimeSpinBox->setMaximum( static_cast<int>(maxTimeMs) );
	connect(ui->msgTimeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
			this, &SettingsWidget::slotValueChanged);

	initLanguages();
	connect(ui->appLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &SettingsWidget::slotLanguageChanged);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsWidget::accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsWidget::reject);
	
	QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
	if (okBtn)
	{
		okBtn->setText( tr("OK") );
		okBtn->setEnabled(false);		// Выключаем кнопку пока пользователем не будут внесены изменения
	}
	
	QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
	if (cancelBtn)
		cancelBtn->setText( tr("Отмена") );
}

SettingsWidget::~SettingsWidget()
{
	delete ui;
}

//-----------------------------------------------------------------------------

bool SettingsWidget::isThemeCorrect(const QString& themePath)
{
	if (!QFile::exists(themePath) ||
		!QFile::exists(themePath + allOffFileName()) ||
		!QFile::exists(themePath + capsFileName()) ||
		!QFile::exists(themePath + numFileName()) ||
		!QFile::exists(themePath + scrollFileName()))
	{
		return false;
	}

	return true;
}

void SettingsWidget::initIconThemes()
{
	QString theme_path = QSettings().value(themeSettings()).toString();

	//-------------------------------------------------------------------------

	ui->iconTheme->addItem(tr("По умолчанию"), defaultThemePath());
	ui->iconTheme->setCurrentIndex(0);

	QString searchThemePath = QCoreApplication::applicationDirPath();
	if (!searchThemePath.endsWith('/')  &&  !searchThemePath.endsWith('\\'))
			searchThemePath += '/';
	
	searchThemePath += customThemePath();
	
	QDir dir(searchThemePath);
	QStringList themes = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	
	int elem_id{ 0 };
	for (const QString& curr : themes)
	{
		QString currTheme = searchThemePath + curr + '/';
		
		if (isThemeCorrect(currTheme))
		{
			ui->iconTheme->addItem(curr, currTheme);
			++elem_id;
			
			if (currTheme == theme_path)
				ui->iconTheme->setCurrentIndex(elem_id);
		}
		else if (currTheme == theme_path)
		{
			theme_path = defaultThemePath();
			QSettings().setValue(themeSettings(), theme_path);
			QSettings().sync();
			
			ui->iconTheme->setCurrentIndex(0);
		}
	}
}

void SettingsWidget::initTracking()
{
	bool isCapsTrackingOn = QSettings().value(capsTrackSettings()).toBool();
	bool isNumTrackingOn = QSettings().value(numTrackSettings()).toBool();
	bool isScrollTrackingOn = QSettings().value(scrollTrackSettings()).toBool();
	
	ui->capsCheckBox->setChecked(isCapsTrackingOn);
	ui->numCheckBox->setChecked(isNumTrackingOn);
	ui->scrollCheckBox->setChecked(isScrollTrackingOn);
}

void SettingsWidget::initMessageType()
{
	int message_type = QSettings().value(msgTypeSettings()).toInt();
	int system_type = static_cast<int>(CapsLoggerMessageType::SYSTEM);
	int custom_type = static_cast<int>(CapsLoggerMessageType::CUSTOM);

	if (!CapsLoggerSettings::isSysMessagesAvailable())
	{
		ui->systemRadioButton->setEnabled(false);
		
		if (message_type == system_type)
		{
			message_type = custom_type;
			QSettings().setValue(msgTypeSettings(), message_type);
		}
	}

	if (message_type == system_type)
	{
		ui->msgLocationLabel->setEnabled(false);
		ui->msgLocation->setEnabled(false);
		ui->msgLocationLabel->setVisible(false);
		ui->msgLocation->setVisible(false);
		
		ui->systemRadioButton->setChecked(true);
	}
	else if (message_type == custom_type)
		ui->customRadioButton->setChecked(true);
}

void SettingsWidget::initMessageLocations()
{
	const int message_location = QSettings().value(msgLocationSettings()).toInt();
	int count = static_cast<int>(CapsLoggerMessageLocation::COUNT);

	for (int i = 0; i < count; i++)
	{
		CapsLoggerMessageLocation currLocation = static_cast<CapsLoggerMessageLocation>(i);
		QString str = CapsLoggerSettings::locationName(currLocation);
		
		ui->msgLocation->addItem(str, i);
		if (i == message_location)
			ui->msgLocation->setCurrentIndex(i);
	}

#ifdef Q_OS_LINUX
	if (!CapsLoggerSettings::isX11Environment())
	{
		int idx = static_cast<int>(CapsLoggerMessageLocation::TOP_RIGHT);
		ui->msgLocation->setCurrentIndex(idx);

		ui->msgLocationLabel->setEnabled(false);
		ui->msgLocation->setEnabled(false);
	}
#endif
}

void SettingsWidget::initLanguages()
{
	const int language = QSettings().value(languageSettings()).toInt();
	int count = static_cast<int>(CapsLoggerLanguage::COUNT);

	for (int i = 0; i < count; i++)
	{
		CapsLoggerLanguage currLang = static_cast<CapsLoggerLanguage>(i);
		QString str = CapsLoggerSettings::languageName(currLang);
		
		ui->appLanguage->addItem(str, i);
		if (i == language)
			ui->appLanguage->setCurrentIndex(i);
	}
}

//-----------------------------------------------------------------------------

void SettingsWidget::initIcon(QLabel* iconWgt, const QString& iconPath)
{
	int size = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
	QSize iconSize = QSize(size, size);
	
	if (QFile::exists(iconPath))
	{
		QIcon icon{ iconPath };
		
		iconWgt->setFixedHeight(size);
		iconWgt->setAlignment(Qt::AlignCenter);
		iconWgt->setPixmap(icon.pixmap(size, size));
	}
	else
	{
		iconWgt->setText("X");
	}
}

void SettingsWidget::slotUpdateIconsPreview(int index)
{
	QString conf_theme = QSettings().value(themeSettings()).toString();
	QString curr_theme = ui->iconTheme->itemData(index, Qt::UserRole).toString();

	if (curr_theme != conf_theme)
	{
		QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
		if (okBtn)
			okBtn->setEnabled(true);
	}

	initIcon(ui->allOffIconLbl, curr_theme + allOffFileName());
	initIcon(ui->capsIconLbl, curr_theme + capsFileName());
	initIcon(ui->numIconLbl, curr_theme + numFileName());
	initIcon(ui->scrollIconLbl, curr_theme + scrollFileName());
}

void SettingsWidget::slotKeyTrackingChanged(int state)
{
	QCheckBox *keyCheckBox = qobject_cast<QCheckBox*>(sender());
	if (keyCheckBox)
	{
		QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
		if (okBtn)
			okBtn->setEnabled(true);
	}
}

void SettingsWidget::slotMessageTypeChanged(bool checked)
{
	CapsLoggerMessageType message_type = static_cast<CapsLoggerMessageType>( QSettings().value(msgTypeSettings()).toInt() );

	QRadioButton *button = qobject_cast<QRadioButton*>(sender());
	if (button == ui->customRadioButton)
	{
		ui->msgLocationLabel->setEnabled(checked);
		ui->msgLocation->setEnabled(checked);
		ui->msgLocationLabel->setVisible(checked);
		ui->msgLocation->setVisible(checked);
#ifdef Q_OS_LINUX
		if (!CapsLoggerSettings::isX11Environment())
		{
			ui->msgLocationLabel->setEnabled(false);
			ui->msgLocation->setEnabled(false);
		}
#endif

		if (checked  &&  message_type != CapsLoggerMessageType::CUSTOM)
		{
			QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
			if (okBtn)
				okBtn->setEnabled(true);
		}
	}
	else if (checked  &&  message_type != CapsLoggerMessageType::SYSTEM)
	{
		QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
		if (okBtn)
			okBtn->setEnabled(true);
	}
}

void SettingsWidget::slotLocationChanged(int index)
{
	int conf_location = static_cast<int>( QSettings().value(msgLocationSettings()).toInt() );
	int curr_location = ui->msgLocation->itemData(index, Qt::UserRole).toInt();

	if (curr_location != conf_location)
	{
		QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
		if (okBtn)
			okBtn->setEnabled(true);
	}
}

void SettingsWidget::slotLanguageChanged(int index)
{
	int conf_language = static_cast<int>( QSettings().value(languageSettings()).toInt() );
	int curr_language = ui->appLanguage->itemData(index, Qt::UserRole).toInt();

	if (curr_language != conf_language)
	{
		QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
		if (okBtn)
			okBtn->setEnabled(true);
	}
}

void SettingsWidget::slotValueChanged(int num)
{
	QSpinBox *spBox = qobject_cast<QSpinBox*>(sender());
	if (!spBox)
		return;

	int conf_value{ 0 };
	int curr_value = num;

	if (spBox == ui->iconTimeSpinBox)
	{
		conf_value = QSettings().value(timeIconSettings()).toInt();
		
		if (curr_value != conf_value)
		{
			QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
			if (okBtn)
				okBtn->setEnabled(true);
		}
	}
	else if (spBox == ui->msgTimeSpinBox)
	{
		conf_value = QSettings().value(timeMsgSettings()).toInt();
		
		if (curr_value != conf_value)
		{
			QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
			if (okBtn)
				okBtn->setEnabled(true);
		}
	}
}

//-----------------------------------------------------------------------------

void SettingsWidget::accept()
{
	QString theme_path = ui->iconTheme->currentData(Qt::UserRole).toString();
	bool isCapsTrackingOn = ui->capsCheckBox->isChecked(); QSettings().value(capsTrackSettings()).toBool();
	bool isNumTrackingOn = ui->numCheckBox->isChecked(); QSettings().value(numTrackSettings()).toBool();
	bool isScrollTrackingOn = ui->scrollCheckBox->checkState(); QSettings().value(scrollTrackSettings()).toBool();
	int message_type = ui->systemRadioButton->isChecked() ?
							static_cast<int>(CapsLoggerMessageType::SYSTEM) :
							static_cast<int>(CapsLoggerMessageType::CUSTOM);
	int message_location = ui->msgLocation->currentData(Qt::UserRole).toInt();
	int language = ui->appLanguage->currentData(Qt::UserRole).toInt();
	int time_icon = ui->iconTimeSpinBox->value();
	int time_message = ui->msgTimeSpinBox->value();

	// Сохраняем настройки только если что-то изменилось
	{
		QSettings settings;
		
		if (settings.value(themeSettings()).toString() != theme_path)
			settings.setValue(themeSettings(),		theme_path);
		
		if (settings.value(capsTrackSettings()).toBool() != isCapsTrackingOn)
			settings.setValue(capsTrackSettings(),	isCapsTrackingOn);
		if (settings.value(numTrackSettings()).toBool() != isNumTrackingOn)
			settings.setValue(numTrackSettings(),		isNumTrackingOn);
		if (settings.value(scrollTrackSettings()).toBool() != isScrollTrackingOn)
			settings.setValue(scrollTrackSettings(),	isScrollTrackingOn);
		
		if (settings.value(msgTypeSettings()).toInt() != message_type)
			settings.setValue(msgTypeSettings(),		message_type);
		
		if (settings.value(msgLocationSettings()).toInt() != message_location)
		{
			if (message_type != static_cast<int>(CapsLoggerMessageType::SYSTEM))
				settings.setValue(msgLocationSettings(),	message_location);
		}
		
		if (settings.value(languageSettings()).toInt() != language)
		{
			settings.setValue(languageSettings(),		language);
			CapsLoggerSettings::setAppLanguage(static_cast<CapsLoggerLanguage>(language));
		}
		
		if (settings.value(timeIconSettings()).toInt() != time_icon)
			settings.setValue(timeIconSettings(),		time_icon);
		
		if (settings.value(timeMsgSettings()).toInt() != time_message)
			settings.setValue(timeMsgSettings(),		time_message);
		
		settings.sync();
	}

	QDialog::accept();
}

void SettingsWidget::reject()
{
	QDialog::reject();
}
