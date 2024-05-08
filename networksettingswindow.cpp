#include "networksettingswindow.h"
#include "ui_networksettingswindow.h"
#include "configurationholder.h"
#include "logger.hpp"
#include "main.hpp"

NetworkSettingsWindow::NetworkSettingsWindow(QWidget *parent) : QWidget(parent),
    ui(new Ui::NetworkSettingsWindow)
{
	gLogger->Log("NetworkSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    ui->setupUi(this);
    ui->updateInterval->setText(QString::number(gAppConfig->UpdateInterval/1000));
    ui->threadsCount->setText(QString::number(gAppConfig->ActiveThreadsMaxNum));
    ui->threadTimeout->setText(QString::number(gAppConfig->ThreadLifeTime/1000));
}

NetworkSettingsWindow::~NetworkSettingsWindow()
{
	gLogger->Log("NetworkSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    delete ui;
}

void NetworkSettingsWindow::keyPressEvent(QKeyEvent *event)
{
	gLogger->Log("NetworkSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    switch(event->key())
    {
        case Qt::Key_Escape:
        {
            this->hide();
        }break;
    }
}

void NetworkSettingsWindow::on_applyButton_clicked()
{
	gLogger->Log("NetworkSettingsWindow::"+string(__FUNCTION__), LOG_DEBUG);
    if(ui->updateInterval->text().toDouble()<1)
    {
        ui->updateInterval->setText("1");
    }
    gAppConfig->UpdateInterval=ui->updateInterval->text().toInt()*1000;
    ui->updateInterval->setText(QString::number(gAppConfig->UpdateInterval/1000.0));

    if(ui->threadsCount->text().toUInt()<1)
    {
        ui->threadsCount->setText("1");
    }
    gAppConfig->ActiveThreadsMaxNum=ui->threadsCount->text().toUInt();
    ui->threadsCount->setText(QString::number(gAppConfig->ActiveThreadsMaxNum));

    if(ui->threadTimeout->text().toDouble()<1)
    {
        ui->threadTimeout->setText("1");
    }
    gAppConfig->ThreadLifeTime=ui->threadTimeout->text().toInt()*1000;
    ui->threadTimeout->setText(QString::number(gAppConfig->ThreadLifeTime/1000.0));
    this->hide();
}
