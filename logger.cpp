#include <ctime>
#include <cstring>
#include "logger.hpp"
#include "main.hpp"

uint8_t Logger::pLogLevel=LOG_DEBUG;
bool Logger::pLogFileEnabled=true;
mutex *Logger::pLogMutex=nullptr;

Logger *gLogger=nullptr;

const char mgstypestring[8][10]=
{
	"EMERGENCY",
	"ALERT",
	"CRITICAL",
	"ERROR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG"
};

Logger::Logger()
{
	if(!pLogMutex)
	{
		pLogMutex=new mutex;
	}
	if(!gLogger)
	{
		gLogger=this;
	}
	pSysLogEnabled=false;
	pLogFilePath.clear();
	pLogFile=nullptr;
}

Logger::~Logger()
{
	if(pLogFile)
	{
		fclose(pLogFile);
		pLogFile=nullptr;
	}
}

uint8_t Logger::LogLevel() const
{
	return(pLogLevel);
}

void Logger::SetLogLevel(uint8_t log_level)
{
	if(log_level>LOG_DEBUG)
	{
		log_level=LOG_DEBUG;
	}
	pLogMutex->lock();
	pLogLevel=log_level;
	pLogMutex->unlock();
}

bool Logger::LogFileEnabled() const
{
	return(pLogFileEnabled);
}

void Logger::SetLogFileEnabled(bool enabled)
{
	pLogMutex->lock();
	pLogFileEnabled=enabled;
	pLogMutex->unlock();
}

string Logger::LogFilePath() const
{
	return(pLogFilePath);
}

void Logger::SetLogFilePath(const string newPath)
{
	if(newPath.length()==0)
	{
		return;
	}
	pLogMutex->lock();
	pLogFilePath=newPath;
	pLogMutex->unlock();
}

bool Logger::SysLogEnabled() const
{
	return(pSysLogEnabled);
}

void Logger::SetSysLogEnabled(bool enabled)
{
	pLogMutex->lock();
	pSysLogEnabled=enabled;
	pLogMutex->unlock();
}

void Logger::Log(const char *message, uint8_t msg_level)
{
	if(!message)
	{
		return;
	}
	string msg(message);
	Log(msg, msg_level);
}

void Logger::Log(const string message, uint8_t msg_level)
{
	if(msg_level>pLogLevel)
	{
		return;
	}
	if(0==message.length())
	{
		return;
	}
	setlocale(LC_NUMERIC, "C");
	pLogMutex->lock();
	time(&pRawTime);
	pTimeInfo=localtime(&pRawTime);
	sprintf(pLogDTstr, "%.2i/%.2i/%i %.2i:%.2i:%.2i", pTimeInfo->tm_mday, 1+pTimeInfo->tm_mon, 1900+pTimeInfo->tm_year, pTimeInfo->tm_hour, pTimeInfo->tm_min, pTimeInfo->tm_sec);
	fprintf(stdout, "%s [%s] %s\n", pLogDTstr, mgstypestring[msg_level], message.data());
	fflush(stdout);
	if(pLogFileEnabled)
	{
		pLogFile=fopen(pLogFilePath.data(), "a");
		if(pLogFile)
		{
			fprintf(pLogFile, "%s [%s] %s\n", pLogDTstr, mgstypestring[msg_level], message.data());
			fflush(pLogFile);
			fclose(pLogFile);
			pLogFile=nullptr;
		}
	}
#if defined(__linux__)
	if(pSysLogEnabled)
	{
		syslog(msg_level, "%s %s", pLogDTstr, message.data());
	}
#endif
	pLogMutex->unlock();
}

