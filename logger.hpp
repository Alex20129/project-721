#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <mutex>

#if defined(__linux__)
#include <sys/syslog.h>
#endif

#if !defined(_SYS_SYSLOG_H)
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
#endif

using namespace std;

class Logger
{
public:
	Logger();
	~Logger();
	uint8_t LogLevel() const;
	void SetLogLevel(uint8_t log_level);
	bool LogFileEnabled() const;
	void SetLogFileEnabled(bool enabled);
	string LogFilePath() const;
	void SetLogFilePath(const string newPath);
	bool SysLogEnabled() const;
	void SetSysLogEnabled(bool enabled);
	void Log(const char *message, uint8_t msg_level);
	void Log(const string message, uint8_t msg_level);
private:
	static uint8_t pLogLevel;
	static bool pLogFileEnabled;
	static mutex *pLogMutex;
	string pLogFilePath;
	bool pSysLogEnabled;
	char pLogDTstr[256];
	time_t pRawTime;
	tm *pTimeInfo;
    FILE *pLogFile;
};

extern Logger *gLogger;

#endif // LOGGER_HPP
