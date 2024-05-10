#include <QApplication>

#include "mainwindow.hpp"
#include "basicsettingswindow.h"
#include "networksettingswindow.h"
#include "devicesettingswindow.h"
#include "scannerwindow.h"
#include "addnewdevicedialog.h"
#include "groupsettingsdialog.h"
#include "sleepsettingswindow.h"
#include "configurationholder.h"

#include "logger.hpp"
#include "main.hpp"

BasicSettingsWindow *BasicSetWin;
NetworkSettingsWindow *NetSetWin;
DeviceSettingsWindow *DevSetWin;
ScannerWindow *ScanWin;
AddNewDeviceDialog *andd;
GroupSettingsDialog *GroupSetDia;
SleepSettingsWindow *SleepSetWin;

#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret;

	gLogger=new Logger;
	gLogger->SetLogFilePath(PROGRAM_SHORT_NAME ".log");

    gAppConfig=new ConfigurationHolder;
	gAppConfig->Load(QString(PROGRAM_SHORT_NAME ".json"));

	gLogger->Log("Start now", LOG_NOTICE);

    QApplication a(argc, argv);

	BasicSetWin=new BasicSettingsWindow;
	NetSetWin=new NetworkSettingsWindow;
	DevSetWin=new DeviceSettingsWindow;
	ScanWin=new ScannerWindow;
	SleepSetWin=new SleepSettingsWindow;
    andd=new AddNewDeviceDialog;
	GroupSetDia=new GroupSettingsDialog;
	MainWin=new MainWindow;

	QApplication::connect(MainWin, &MainWindow::NeedToShowBasicSettingsWindow, BasicSetWin, &BasicSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowNetworkSettingsWindow, NetSetWin, &NetworkSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowDeviceSettingsWindow, DevSetWin, &DeviceSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowScannerWindow, ScanWin, &ScannerWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowSleepSettingsWindow, SleepSetWin, &SleepSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowAddNewDeviceDialog, andd, &AddNewDeviceDialog::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowGroupSettings, GroupSetDia, &GroupSettingsDialog::showGroupSettings);
	QApplication::connect(MainWin, &MainWindow::NeedToCreateNewGroup, GroupSetDia, &QDialog::show);
	QApplication::connect(MainWin, &MainWindow::NeedToRescanDevices, ScanWin, &ScannerWindow::ScanDevices);

	QApplication::connect(ScanWin, &ScannerWindow::ScanIsDone, MainWin, &MainWindow::updateDeviceView);
	QApplication::connect(ScanWin, &ScannerWindow::DeviceFound, MainWin, &MainWindow::addDevice);

	QApplication::connect(DevSetWin, &DeviceSettingsWindow::deviceSettingsObtained, MainWin, &MainWindow::uploadSettings);
	QApplication::connect(andd, &AddNewDeviceDialog::deviceDataObtained, MainWin, &MainWindow::addNewDevices);
	QApplication::connect(GroupSetDia, &GroupSettingsDialog::groupDataObtained, MainWin, &MainWindow::addNewGroup);

	MainWin->loadTabs();
	MainWin->show();

    ret=a.exec();

	gAppConfig->Save(QString(PROGRAM_SHORT_NAME ".json"));
	gLogger->Log("Exit now", LOG_NOTICE);
    return(ret);
}
