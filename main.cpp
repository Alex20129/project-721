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

BasicSettingsWindow *bsw;
NetworkSettingsWindow *nsw;
DeviceSettingsWindow *devsw;
ScannerWindow *sw;
AddNewDeviceDialog *andd;
AddNewGroupDialog *angd;
SleepSettingsWindow *ssw;

#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret;

	gLogger=new Logger;
	gLogger->SetLogFilePath(PROGRAM_SHORT_NAME ".log");

    gAppConfig=new ConfigurationHolder;
	gAppConfig->Load(QString(PROGRAM_SHORT_NAME ".json"));

    QApplication a(argc, argv);

    bsw=new BasicSettingsWindow;
    nsw=new NetworkSettingsWindow;
    devsw=new DeviceSettingsWindow;
    sw=new ScannerWindow;
	ssw=new SleepSettingsWindow;
    andd=new AddNewDeviceDialog;
    angd=new AddNewGroupDialog;
    mw=new MainWindow;

	QApplication::connect(mw, &MainWindow::NeedToShowBasicSettingsWindow, bsw, &BasicSettingsWindow::show);
	QApplication::connect(mw, &MainWindow::NeedToShowNetworkSettingsWindow, nsw, &NetworkSettingsWindow::show);
	QApplication::connect(mw, &MainWindow::NeedToShowDeviceSettingsWindow, devsw, &DeviceSettingsWindow::show);
	QApplication::connect(mw, &MainWindow::NeedToShowScannerWindow, sw, &ScannerWindow::show);
	QApplication::connect(mw, &MainWindow::NeedToShowSleepSettingsWindow, ssw, &SleepSettingsWindow::show);
	QApplication::connect(mw, &MainWindow::NeedToShowAddNewDeviceDialog, andd, &AddNewDeviceDialog::show);
	QApplication::connect(mw, &MainWindow::NeedToShowAddNewGroupDialog, angd, &AddNewGroupDialog::show);

	QApplication::connect(sw, SIGNAL(ScanIsDone()), mw, SLOT(updateDeviceView()));
	QApplication::connect(angd, SIGNAL(groupDataObtained(QString, QString, QString, QString, quint16, quint16)), mw, SLOT(addNewGroup(QString, QString, QString, QString, quint16, quint16)));
	QApplication::connect(andd, SIGNAL(deviceDataObtained(QString, QString)), mw, SLOT(addNewDevices(QString, QString)));
	QApplication::connect(devsw, SIGNAL(deviceSettingsObtained(QStringList)), mw, SLOT(uploadSettings(QStringList)));

	QApplication::connect(sw, &ScannerWindow::DeviceFound, mw, &MainWindow::addDevice);

	gLogger->Log("Start now", LOG_NOTICE);
    mw->show();

    ret=a.exec();

	gAppConfig->Save(QString(PROGRAM_SHORT_NAME ".json"));
	gLogger->Log("Exit now", LOG_NOTICE);
    return(ret);
}
