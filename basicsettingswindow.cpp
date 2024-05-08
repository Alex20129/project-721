#include "basicsettingswindow.h"
#include "ui_basicsettingswindow.h"
#include "configurationholder.h"
#include "logger.hpp"
#include "main.hpp"

BasicSettingsWindow::BasicSettingsWindow(QWidget *parent) : QWidget(parent), ui(new Ui::BasicSettingsWindow)
{
	gLogger->Log("BasicSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    ui->setupUi(this);
    ui->clear_firmware_update->setChecked(gAppConfig->ClearUpSettingsWhenFirmwareUpdate);
    ui->hwErrors->setText(QString::number(gAppConfig->AlarmOnHWErrors));
    ui->tempAbove->setText(QString::number(gAppConfig->AlarmWhenTemperatureAbove));
    ui->tempBelow->setText(QString::number(gAppConfig->AlarmWhenTemperatureBelow));
}

BasicSettingsWindow::~BasicSettingsWindow()
{
	gLogger->Log("BasicSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    delete ui;
}

void BasicSettingsWindow::keyPressEvent(QKeyEvent *event)
{
	gLogger->Log("BasicSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    switch(event->key())
    {
        case Qt::Key_Escape:
        {
            this->hide();
        }break;
    }
}

void BasicSettingsWindow::on_applyButton_clicked()
{
	gLogger->Log("BasicSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    gAppConfig->ClearUpSettingsWhenFirmwareUpdate=ui->clear_firmware_update->isChecked();
    gAppConfig->AlarmOnHWErrors=ui->hwErrors->text().toUInt();
    gAppConfig->AlarmWhenTemperatureAbove=ui->tempAbove->text().toUInt();
    gAppConfig->AlarmWhenTemperatureBelow=ui->tempBelow->text().toUInt();
    this->hide();
}
