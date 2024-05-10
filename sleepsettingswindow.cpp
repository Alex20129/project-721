#include "sleepsettingswindow.h"
#include "ui_sleepsettingswindow.h"
#include "configurationholder.h"
#include "mainwindow.hpp"
#include "main.hpp"

SleepSettingsWindow::SleepSettingsWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SleepSettingsWindow)
{
    ui->setupUi(this);
    ui->timeSleep->setTime(gAppConfig->TimeToSleep);
    ui->timeWake->setTime(gAppConfig->TimeToWakeUp);
}

SleepSettingsWindow::~SleepSettingsWindow()
{
    delete ui;
}

void SleepSettingsWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Escape:
        {
            this->hide();
        }break;
    }
}

void SleepSettingsWindow::on_applyButton_clicked()
{
    gAppConfig->TimeToSleep=ui->timeSleep->dateTime().time();
    gAppConfig->TimeToWakeUp=ui->timeWake->dateTime().time();
    if(gAppConfig->TimeToSleep==gAppConfig->TimeToWakeUp)
    {
		gMainWin->SleepWakeTimer->stop();
    }
    else
    {
		gMainWin->SleepWakeTimer->start();
    }
    this->hide();
}
