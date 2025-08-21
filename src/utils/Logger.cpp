#include "utils/Logger.h"
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QCoreApplication>
#include <QThread>
#include <iostream>
#include <QMutexLocker>

// 静态成员初始化
Logger::LogLevel Logger::s_logLevel = Logger::LogLevel::Info;
QString Logger::s_logFilePath;
bool Logger::s_consoleOutput = true;
QMutex Logger::s_mutex;
QFile Logger::s_logFile;
QTextStream Logger::s_logStream;
bool Logger::s_initialized = false;

void Logger::initialize()
{
    QMutexLocker locker(&s_mutex);
    
    if (s_initialized) {
        return;
    }
    
    // 设置默认日志路径
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);
    
    s_logFilePath = logDir + "/" + getLogFileName();
    
    // 打开日志文件
    s_logFile.setFileName(s_logFilePath);
    if (s_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        s_logStream.setDevice(&s_logFile);
    }
    
    s_initialized = true;
    
    // 记录启动日志
    info("Logger initialized", "System");
    info(QString("Log file: %1").arg(s_logFilePath), "System");
    info(QString("Application: %1 %2").arg(QCoreApplication::applicationName())
         .arg(QCoreApplication::applicationVersion()), "System");
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&s_mutex);
    s_logLevel = level;
}

void Logger::setLogFile(const QString& filePath)
{
    QMutexLocker locker(&s_mutex);
    
    if (s_logFile.isOpen()) {
        s_logStream.setDevice(nullptr);
        s_logFile.close();
    }
    
    s_logFilePath = filePath;
    
    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.absolutePath());
    
    s_logFile.setFileName(filePath);
    if (s_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        s_logStream.setDevice(&s_logFile);
    }
}

void Logger::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&s_mutex);
    s_consoleOutput = enabled;
}

void Logger::debug(const QString& message, const QString& category)
{
    writeLog(LogLevel::Debug, message, category);
}

void Logger::info(const QString& message, const QString& category)
{
    writeLog(LogLevel::Info, message, category);
}

void Logger::warning(const QString& message, const QString& category)
{
    writeLog(LogLevel::Warning, message, category);
}

void Logger::error(const QString& message, const QString& category)
{
    writeLog(LogLevel::Error, message, category);
}

void Logger::critical(const QString& message, const QString& category)
{
    writeLog(LogLevel::Critical, message, category);
}

void Logger::cleanupOldLogs(int retentionDays)
{
    if (!s_initialized) {
        return;
    }
    
    QFileInfo fileInfo(s_logFilePath);
    QDir logDir = fileInfo.absoluteDir();
    
    QStringList nameFilters;
    nameFilters << "*.log";
    
    QFileInfoList logFiles = logDir.entryInfoList(nameFilters, QDir::Files, QDir::Time);
    
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-retentionDays);
    
    for (const QFileInfo& logFile : logFiles) {
        if (logFile.lastModified() < cutoffDate) {
            QFile::remove(logFile.absoluteFilePath());
            info(QString("Cleaned up old log file: %1").arg(logFile.fileName()), "System");
        }
    }
}

qint64 Logger::getLogFileSize()
{
    if (!s_initialized || !s_logFile.exists()) {
        return 0;
    }
    
    return s_logFile.size();
}

void Logger::writeLog(LogLevel level, const QString& message, const QString& category)
{
    if (!s_initialized) {
        initialize();
    }
    
    // 检查日志级别
    if (level < s_logLevel) {
        return;
    }
    
    QMutexLocker locker(&s_mutex);
    
    QString formattedMessage = formatLogMessage(level, message, category);
    
    // 输出到控制台
    if (s_consoleOutput) {
        if (level >= LogLevel::Error) {
            std::cerr << formattedMessage.toStdString() << std::endl;
        } else {
            std::cout << formattedMessage.toStdString() << std::endl;
        }
    }
    
    // 输出到文件
    if (s_logFile.isOpen()) {
        s_logStream << formattedMessage << Qt::endl;
        s_logStream.flush();
    }
}

QString Logger::formatLogMessage(LogLevel level, const QString& message, const QString& category)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = logLevelToString(level);
    QString threadId = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
    
    QString formatted = QString("[%1] [%2] [Thread:%3]")
                       .arg(timestamp)
                       .arg(levelStr)
                       .arg(threadId);
    
    if (!category.isEmpty()) {
        formatted += QString(" [%1]").arg(category);
    }
    
    formatted += QString(" %1").arg(message);
    
    return formatted;
}

QString Logger::logLevelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:    return "DEBUG";
    case LogLevel::Info:     return "INFO ";
    case LogLevel::Warning:  return "WARN ";
    case LogLevel::Error:    return "ERROR";
    case LogLevel::Critical: return "FATAL";
    default:                 return "UNKNOWN";
    }
}

QString Logger::getLogFileName()
{
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString appName = QCoreApplication::applicationName();
    if (appName.isEmpty()) {
        appName = "WorkstationWellnessElf";
    }
    
    return QString("%1_%2.log").arg(appName).arg(date);
}