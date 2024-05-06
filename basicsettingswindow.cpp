#include "basicsettingswindow.h"
#include "ui_basicsettingswindow.h"
#include "main.hpp"

BasicSettingsWindow::BasicSettingsWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BasicSettingsWindow)
{
    ui->setupUi(this);
    ui->clear_firmware_update->setChecked(gAppConfig->ClearUpSettingsWhenFirmwareUpdate);
    ui->hwErrors->setText(QString::number(gAppConfig->AlarmOnHWErrors));
    ui->tempAbove->setText(QString::number(gAppConfig->AlarmWhenTemperatureAbove));
    ui->tempBelow->setText(QString::number(gAppConfig->AlarmWhenTemperatureBelow));
}

BasicSettingsWindow::~BasicSettingsWindow()
{
    delete ui;
}

void BasicSettingsWindow::keyPressEvent(QKeyEvent *event)
{
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
    gAppConfig->ClearUpSettingsWhenFirmwareUpdate=ui->clear_firmware_update->isChecked();
    gAppConfig->AlarmOnHWErrors=ui->hwErrors->text().toUInt();
    gAppConfig->AlarmWhenTemperatureAbove=ui->tempAbove->text().toUInt();
    gAppConfig->AlarmWhenTemperatureBelow=ui->tempBelow->text().toUInt();
    this->hide();
}
