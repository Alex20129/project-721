#include "configurationholder.h"
#include "main.hpp"

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
    this->MakeEncryptionCode();
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

void ConfigurationHolder::MakeEncryptionCode()
{
    QCryptographicHash newHash(QCryptographicHash::Sha256);
    SecretCode.clear();
    SecretCode+=QSysInfo::currentCpuArchitecture()+
                QSysInfo::kernelType()+
                QSysInfo::kernelVersion()+
                QSysInfo::productType()+
                QSysInfo::productVersion()+
                QSysInfo::machineHostName();
    for(uint i=0; i<QNetworkInterface::allInterfaces().size(); i++)
    {
        SecretCode+=QNetworkInterface::allInterfaces().at(i).hardwareAddress();
    }
    newHash.addData(SecretCode);
    SecretCode=newHash.result();
}

void ConfigurationHolder::EncryptData(QByteArray *data)
{
    if(!SecretCode.length())
    {
        return;
    }
    if(!data->length())
    {
        return;
    }
    QByteArray key(SecretCode);
    for(int i=0, k=0; i<data->length(); i++, k++)
    {
        if(k==key.length())
        {
            for(k--; k>=0; k--)
            {
                key.data()[k]=(key.data()[k]>>1)+(key.data()[k]<<7);
            }
        }
        data->data()[i]=data->data()[i]^key.data()[k];
    }
}
