#include "devicesettingswindow.h"
#include "ui_devicesettingswindow.h"
#include "main.hpp"

DeviceSettingsWindow::DeviceSettingsWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceSettingsWindow)
{
    ui->setupUi(this);
    ui->add_postfix->setChecked(gAppConfig->AddressBasedPostfixToWorkerName);
}

DeviceSettingsWindow::~DeviceSettingsWindow()
{
    delete ui;
}

void DeviceSettingsWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Escape:
        {
            this->hide();
        }break;
    }
}

void DeviceSettingsWindow::on_applyButton_clicked()
{
    gAppConfig->AddressBasedPostfixToWorkerName=ui->add_postfix->isChecked();
    QStringList settings;
    settings.append(QString("pool1url=")+ui->pool1url->text());
    settings.append(QString("pool1user=")+ui->pool1user->text());
    settings.append(QString("pool1pw=")+ui->pool1password->text());
    settings.append(QString("pool2url=")+ui->pool2url->text());
    settings.append(QString("pool2user=")+ui->pool2user->text());
    settings.append(QString("pool2pw=")+ui->pool2password->text());
    settings.append(QString("pool3url=")+ui->pool3url->text());
    settings.append(QString("pool3user=")+ui->pool3user->text());
    settings.append(QString("pool3pw=")+ui->pool3password->text());
    settings.append(QString("nobeeper=")+QString(ui->nobeeper->isChecked() ? "true" : "false"));
    settings.append(QString("notempoverctrl=")+QString(ui->notempoverctrl->isChecked() ? "true" : "false"));
    settings.append(QString("fan_customize_switch=")+QString(ui->fan_customize_switch->isChecked() ? "true" : "false"));
    settings.append(QString("fan_customize_value=")+QString::number(ui->fan_customize_value->text().toUInt()));
    settings.append(QString("freq=")+QString::number(ui->freq->text().toUInt()));
    settings.append(QString("voltage=")+ui->voltage->text());
    settings.append(QString("api_allow=")+ui->api_allow->text());
    settings.append(QString("api_listen=")+ui->api_listen->text());
    settings.append(QString("enabled_boards=")+ui->enabled_boards->text());
    emit(deviceSettingsObtained(settings));
    this->hide();
}
