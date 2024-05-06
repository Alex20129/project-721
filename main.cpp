#include <QApplication>

#include "logger.hpp"
#include "main.hpp"

QVector <ASICDevice *> GlobalDeviceList;
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
    gAppConfig=new ConfigurationHolder;
    gAppConfig->Load(QString(PROGRAM_SHORT_NAME)+QString(".json"));

    QApplication a(argc, argv);

    gAppLogger=new Logger;
    gAppLogger->Log("Log initiated...");

    bsw=new BasicSettingsWindow;
    nsw=new NetworkSettingsWindow;
    devsw=new DeviceSettingsWindow;
    sw=new ScannerWindow;
    andd=new AddNewDeviceDialog;
    angd=new AddNewGroupDialog;
    ssw=new SleepSettingsWindow;
    mw=new MainWindow;

    gAppLogger->Log("Start now");
    mw->show();

    ret=a.exec();

    gAppConfig->Save(QString(PROGRAM_SHORT_NAME)+QString(".json"));
    return(ret);
}
