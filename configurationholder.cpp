#include "configurationholder.h"
#include "main.hpp"
#include <QVariant>

ConfigurationHolder *gAppConfig;

ConfigurationHolder::ConfigurationHolder()
{
    ThreadLifeTime=DEFAULT_THREAD_LIFETIME;
    ActiveThreadsMaxNum=DEFAULT_THREADS_MAX_NUM;
    UpdateInterval=DEFAULT_UPDATE_INTERVAL;
    UserAgent=API_HEADER_USER_AGENT;
    ClearUpSettingsWhenFirmwareUpdate=0;
    AddressBasedPostfixToWorkerName=0;
    AlarmWhenTemperatureAbove=DEFAULT_ALARM_TEMP_ABOVE;
    AlarmWhenTemperatureBelow=DEFAULT_ALARM_TEMP_BELOW;
    AlarmOnHWErrors=DEFAULT_ALARM_ON_HW_ERRORS;
}

int ConfigurationHolder::Save()
{
    if(LastUsedFile.isEmpty())
    {
        return(ERROR_BAD_PARAMETER);
    }
    return(Save(LastUsedFile));
}

int ConfigurationHolder::Save(QString pathToFile)
{
    QByteArray ConfigData;
    QFile ConfigFile(pathToFile);
    if(!ConfigFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return(ERROR_OPENING_FILE);
    }

    JSONData.insert("AddressBasedPostfixToWorkerName", QJsonValue::fromVariant(AddressBasedPostfixToWorkerName));
    JSONData.insert("AlarmWhenTemperatureAbove", QJsonValue::fromVariant(AlarmWhenTemperatureAbove));
    JSONData.insert("AlarmWhenTemperatureBelow", QJsonValue::fromVariant(AlarmWhenTemperatureBelow));
    JSONData.insert("AlarmOnHWErrors", QJsonValue::fromVariant(AlarmOnHWErrors));
    JSONData.insert("ClearUpSettingsWhenFirmwareUpdate", QJsonValue::fromVariant(ClearUpSettingsWhenFirmwareUpdate));
    JSONData.insert("TimeToSleep", QJsonValue::fromVariant(TimeToSleep));
    JSONData.insert("TimeToWakeUp", QJsonValue::fromVariant(TimeToWakeUp));
    JSONData.insert("ThreadLifeTime", QJsonValue::fromVariant(ThreadLifeTime));
    JSONData.insert("ThreadsMaxNum", QJsonValue::fromVariant(ActiveThreadsMaxNum));
    JSONData.insert("UpdateInterval", QJsonValue::fromVariant(UpdateInterval));

    QJsonDocument jdoc(JSONData);

    ConfigData=jdoc.toJson();
    //EncryptData(&ConfigData);
    ConfigFile.write(ConfigData);
    ConfigFile.close();

    LastUsedFile=pathToFile;
    return(0);
}

int ConfigurationHolder::Load()
{
    if(LastUsedFile.isEmpty())
    {
        return(ERROR_BAD_PARAMETER);
    }
    return(Load(LastUsedFile));
}

int ConfigurationHolder::Load(QString pathToFile)
{
    QFile ConfigFile(pathToFile);
    if(!ConfigFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return(ERROR_OPENING_FILE);
    }

    QByteArray ConfigData=ConfigFile.readAll();
    ConfigFile.close();

    LastUsedFile=pathToFile;

    QJsonDocument jd=QJsonDocument::fromJson(ConfigData);
    JSONData=jd.object();

    AddressBasedPostfixToWorkerName=JSONData.value("AddressBasedPostfixToWorkerName").toBool();
    AlarmWhenTemperatureAbove=JSONData.value("AlarmWhenTemperatureAbove").toDouble(DEFAULT_ALARM_TEMP_ABOVE);
    AlarmWhenTemperatureBelow=JSONData.value("AlarmWhenTemperatureBelow").toDouble(DEFAULT_ALARM_TEMP_BELOW);
    AlarmOnHWErrors=static_cast<unsigned int>(JSONData.value("AlarmOnHWErrors").toInt(DEFAULT_ALARM_ON_HW_ERRORS));
    ClearUpSettingsWhenFirmwareUpdate=JSONData.value("ClearUpSettingsWhenFirmwareUpdate").toBool();
    TimeToSleep=QTime::fromString(JSONData.value("TimeToSleep").toString());
    TimeToWakeUp=QTime::fromString(JSONData.value("TimeToWakeUp").toString());
    ThreadLifeTime=JSONData.value("ThreadLifeTime").toInt(DEFAULT_THREAD_LIFETIME);
    ActiveThreadsMaxNum=static_cast<unsigned int>(JSONData.value("ThreadsMaxNum").toInt(DEFAULT_THREADS_MAX_NUM));
    UpdateInterval=JSONData.value("UpdateInterval").toInt(DEFAULT_UPDATE_INTERVAL);
    return(0);
}
