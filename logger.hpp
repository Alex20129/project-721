#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QString>

#define	LOG_EMERG		1	/* system is unusable */
#define	LOG_ALERT		2	/* action must be taken immediately */
#define	LOG_CRITICAL	4	/* critical conditions */
#define	LOG_ERROR		8	/* error conditions */
#define	LOG_WARNING		16	/* warning conditions */
#define	LOG_NOTICE		32	/* normal but significant condition */
#define	LOG_INFO		64	/* informational */
#define	LOG_DEBUG		128	/* debug-level messages */

class Logger
{
public:
    Logger();
    ~Logger();
    void SetLogFilePath(const char *newPath);
    void SetLogFilePath(QString newPath);
    void Log(const char *message, unsigned int level=LOG_INFO);
    void Log(QByteArray message, unsigned int level=LOG_INFO);
    void Log(QString message, unsigned int level=LOG_INFO);
    static unsigned int LogSettings;
    static bool LogFileEnabled;
private:
    static bool pIsBusy;
    char *pLogFilePath;
    FILE *pLogFile;
    QString pCurrDTstring;
};

extern Logger *gAppLogger;

#endif // LOGGER_HPP
