#include <QDateTime>
#include "logger.hpp"
#include "main.hpp"

Logger *gAppLogger;

// LOG_EMERG | LOG_ALERT | LOG_CRITICAL | LOG_ERROR | LOG_WARNING | LOG_NOTICE | LOG_INFO | LOG_DEBUG;
unsigned int Logger::LogSettings=LOG_EMERG | LOG_ALERT | LOG_CRITICAL | LOG_ERROR | LOG_WARNING | LOG_NOTICE;
bool Logger::LogFileEnabled=true;
bool Logger::pIsBusy=false;

Logger::Logger()
{
    pLogFilePath=new char[256];
    sprintf(pLogFilePath, "%.255s.log", PROGRAM_NAME);
    pLogFile=fopen(pLogFilePath, "a");
}

Logger::~Logger()
{
    if(pLogFile)
    {
        fclose(pLogFile);
    }
}

void Logger::SetLogFilePath(const char *newPath)
{
    if(strlen(newPath))
    {
        pIsBusy=true;
        sprintf(pLogFilePath, "%.255s", newPath);
        if(pLogFile)
        {
            fclose(pLogFile);
        }
        pLogFile=fopen(pLogFilePath, "a");
        pIsBusy=false;
    }
}

void Logger::SetLogFilePath(QString newPath)
{
    if(!newPath.isEmpty())
    {
        SetLogFilePath(newPath.toStdString().data());
    }
}

void Logger::Log(const char *message, unsigned int level)
{
    if(!(level&LogSettings))
    {
        return;
    }
    while(pIsBusy)
    {
        QCoreApplication::processEvents();
    }
    pIsBusy=true;
    pCurrDTstring=QDateTime::currentDateTime().toString("dd.MM.yyyy | hh:mm:ss");
    if(pLogFile && LogFileEnabled)
    {
        fprintf(pLogFile, "%s %s\n", pCurrDTstring.toStdString().data(), message);
        fflush(pLogFile);
    }
    fprintf(stdout, "%s %s\n", pCurrDTstring.toStdString().data(), message);
    fflush(stdout);
    pIsBusy=false;
}

void Logger::Log(QByteArray message, unsigned int level)
{
    Log(message.data(), level);
}

void Logger::Log(QString message, unsigned int level)
{
    Log(message.toStdString().data(), level);
}
