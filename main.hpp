#define PROGRAM_NAME            "ASIC Monitor"
#define PROGRAM_SHORT_NAME      "asicmon"
#define PROGRAM_VERSION         "0.7.11"
#define PROGRAM_DESCRIPTION     "ASIC device monitoring and maintenance tool"

#define DEVICE_BOARDS_NUM       16
#define DEVICE_POOLS_NUM        8
#define API_HEADER_CONTENT_TYPE "application/json"

#define ENABLE_LOGGING_TO_FILE
#define ENABLE_LOGGING_TO_CONSOLE

#if defined(Q_OS_LINUX)
    #define API_HEADER_USER_AGENT PROGRAM_SHORT_NAME "/" PROGRAM_VERSION " (X11; Linux; x86_64)"
#elif defined(Q_OS_WIN64)
    #define API_HEADER_USER_AGENT PROGRAM_SHORT_NAME "/" PROGRAM_VERSION " (Windows NT 6.1; Windows; x86_64)"
#elif defined(Q_OS_WIN32)
    #define API_HEADER_USER_AGENT PROGRAM_SHORT_NAME "/" PROGRAM_VERSION " (Windows NT 6.1; Windows; x86_32)"
#elif defined(Q_OS_MACOS)
    #define API_HEADER_USER_AGENT PROGRAM_SHORT_NAME "/" PROGRAM_VERSION " (Macintosh; MacOS; x86_64)"
#endif
