#pragma once

#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QTextStream>
#include <QFile>

/**
 * @brief 日志管理工具类
 * 
 * 提供统一的日志记录功能，支持不同级别的日志输出
 */
class Logger
{
public:
    enum class LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    /**
     * @brief 初始化日志系统
     */
    static void initialize();

    /**
     * @brief 设置日志级别
     */
    static void setLogLevel(LogLevel level);

    /**
     * @brief 设置日志文件路径
     */
    static void setLogFile(const QString& filePath);

    /**
     * @brief 设置是否输出到控制台
     */
    static void setConsoleOutput(bool enabled);

    /**
     * @brief 记录调试信息
     */
    static void debug(const QString& message, const QString& category = QString());

    /**
     * @brief 记录一般信息
     */
    static void info(const QString& message, const QString& category = QString());

    /**
     * @brief 记录警告信息
     */
    static void warning(const QString& message, const QString& category = QString());

    /**
     * @brief 记录错误信息
     */
    static void error(const QString& message, const QString& category = QString());

    /**
     * @brief 记录严重错误信息
     */
    static void critical(const QString& message, const QString& category = QString());

    /**
     * @brief 清理旧日志文件
     */
    static void cleanupOldLogs(int retentionDays = 7);

    /**
     * @brief 获取日志文件大小（字节）
     */
    static qint64 getLogFileSize();

private:
    static void writeLog(LogLevel level, const QString& message, const QString& category);
    static QString formatLogMessage(LogLevel level, const QString& message, const QString& category);
    static QString logLevelToString(LogLevel level);
    static QString getLogFileName();

    static LogLevel s_logLevel;
    static QString s_logFilePath;
    static bool s_consoleOutput;
    static QMutex s_mutex;
    static QFile s_logFile;
    static QTextStream s_logStream;
    static bool s_initialized;
};