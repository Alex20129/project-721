#include <QApplication>

#include "logger.hpp"
#include "main.hpp"

ConfigurationHolder *gAppConfig;

BasicSettingsWindow *bsw;
NetworkSettingsWindow *nsw;
DeviceSettingsWindow *devsw;
ScannerWindow *sw;
AddNewDeviceDialog *andd;
AddNewGroupDialog *angd;
SleepSettingsWindow *ssw;
MainWindow *mw;

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
    andd=new AddNewDeviceDialog;
    angd=new AddNewGroupDialog;
    ssw=new SleepSettingsWindow;
    mw=new MainWindow;

	gLogger->Log("Start now", LOG_NOTICE);
    mw->show();

    ret=a.exec();

	gAppConfig->Save(QString(PROGRAM_SHORT_NAME ".json"));
	gLogger->Log("Exit now", LOG_NOTICE);
    return(ret);
}
