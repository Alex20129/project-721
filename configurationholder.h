#ifndef CONFIGURATIONHOLDER_H
#define CONFIGURATIONHOLDER_H

#include <QString>
#include <QFile>
#include <QSysInfo>
#include <QTime>
#include <QJsonDocument>
#include <QJsonObject>

#define DEFAULT_ALARM_TEMP_ABOVE   85
#define DEFAULT_ALARM_TEMP_BELOW   10
#define DEFAULT_ALARM_ON_HW_ERRORS 1000
#define DEFAULT_UPDATE_INTERVAL    1900
#define DEFAULT_THREADS_MAX_NUM    20
#define DEFAULT_THREAD_LIFETIME    2000

#define ERROR_BAD_PARAMETER -1
#define ERROR_OPENING_FILE  -2

class ConfigurationHolder
{
public:
    ConfigurationHolder();
    QString UserAgent;
    int ThreadLifeTime;
    uint ActiveThreadsMaxNum;
    int UpdateInterval;
    double AlarmWhenTemperatureAbove;
    double AlarmWhenTemperatureBelow;
    uint AlarmOnHWErrors;
    bool ClearUpSettingsWhenFirmwareUpdate;
    bool AddressBasedPostfixToWorkerName;
    QTime TimeToSleep, TimeToWakeUp;
    QJsonObject JSONData;
    int Save();
    int Save(QString pathToFile);
    int Load();
    int Load(QString pathToFile);
private:
	QString LastUsedFile;
};

extern ConfigurationHolder *gAppConfig;

#endif // CONFIGURATIONHOLDER_H
