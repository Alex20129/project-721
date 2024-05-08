#include <QApplication>

#include "mainwindow.hpp"
#include "basicsettingswindow.h"
#include "networksettingswindow.h"
#include "devicesettingswindow.h"
#include "scannerwindow.h"
#include "addnewdevicedialog.h"
#include "addnewgroupdialog.h"
#include "sleepsettingswindow.h"
#include "configurationholder.h"

#include "logger.hpp"
#include "main.hpp"

BasicSettingsWindow *BasicSetWin;
NetworkSettingsWindow *NetSetWin;
DeviceSettingsWindow *DevSetWin;
ScannerWindow *ScanWin;
AddNewDeviceDialog *andd;
AddNewGroupDialog *angd;
SleepSettingsWindow *SleepSetWin;

#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret;

	gLogger=new Logger;
	gLogger->SetLogFilePath(PROGRAM_SHORT_NAME ".log");

    gAppConfig=new ConfigurationHolder;
	gAppConfig->Load(QString(PROGRAM_SHORT_NAME ".json"));

    QApplication a(argc, argv);

	BasicSetWin=new BasicSettingsWindow;
	NetSetWin=new NetworkSettingsWindow;
	DevSetWin=new DeviceSettingsWindow;
	ScanWin=new ScannerWindow;
	SleepSetWin=new SleepSettingsWindow;
    andd=new AddNewDeviceDialog;
    angd=new AddNewGroupDialog;
	MainWin=new MainWindow;

	QApplication::connect(MainWin, &MainWindow::NeedToShowBasicSettingsWindow, BasicSetWin, &BasicSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowNetworkSettingsWindow, NetSetWin, &NetworkSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowDeviceSettingsWindow, DevSetWin, &DeviceSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowScannerWindow, ScanWin, &ScannerWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowSleepSettingsWindow, SleepSetWin, &SleepSettingsWindow::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowAddNewDeviceDialog, andd, &AddNewDeviceDialog::show);
	QApplication::connect(MainWin, &MainWindow::NeedToShowAddNewGroupDialog, angd, &AddNewGroupDialog::show);

	QApplication::connect(ScanWin, &ScannerWindow::ScanIsDone, MainWin, &MainWindow::updateDeviceView);
	QApplication::connect(DevSetWin, &DeviceSettingsWindow::deviceSettingsObtained, MainWin, &MainWindow::uploadSettings);
	QApplication::connect(andd, &AddNewDeviceDialog::deviceDataObtained, MainWin, &MainWindow::addNewDevices);
	QApplication::connect(angd, &AddNewGroupDialog::groupDataObtained, MainWin, &MainWindow::addNewGroup);

	QApplication::connect(ScanWin, &ScannerWindow::DeviceFound, MainWin, &MainWindow::addDevice);

	gLogger->Log("Start now", LOG_NOTICE);
	MainWin->show();

    ret=a.exec();

	gAppConfig->Save(QString(PROGRAM_SHORT_NAME ".json"));
	gLogger->Log("Exit now", LOG_NOTICE);
    return(ret);
}
