
#include "core/DataAnalyzer.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

DataAnalyzer::DataAnalyzer(QObject *parent)
    : QObject(parent), m_lastAnalysisTime(QDateTime::currentDateTime())
{
    loadDataFromFile();

    m_analysisTimer = new QTimer(this);
    connect(m_analysisTimer, &QTimer::timeout, this, &DataAnalyzer::analyzePatterns);
    // 每 5 分钟分析一次并保存
    m_analysisTimer->start(1000 * 60 * 5); 
}

DataAnalyzer::~DataAnalyzer()
{
    saveDataToFile();
}

void DataAnalyzer::recordActivity(const ActivityMonitor::ActivityData& data)
{
    ActivityRecord record;
    record.timestamp = data.timestamp;
    record.data = data;
    m_activityRecords.append(record);
    emit dataUpdated();
}

void DataAnalyzer::recordHealthEvent(HealthEngine::ReminderType type, const QString& action)
{
    HealthEventRecord record;
    record.timestamp = QDateTime::currentDateTime();
    record.type = type;
    record.action = action;
    m_healthEvents.append(record);
    emit dataUpdated();
}

DataAnalyzer::DailyReport DataAnalyzer::getDailyReport(const QDate& date) const
{
    // TODO: 实现详细的日报生成逻辑
    DailyReport report;
    report.date = date;
    report.totalActiveMinutes = 0;
    report.totalBreaks = 0;
    report.longestSittingSession = 0;
    report.healthScore = 0.0;

    for (const auto& record : m_activityRecords) {
        if (record.timestamp.date() == date) {
            if (record.data.isActive) {
                report.totalActiveMinutes++; // 假设每条记录代表1分钟
            }
        }
    }
    return report;
}

DataAnalyzer::WeeklyTrend DataAnalyzer::getWeeklyTrend(const QDate& weekStart) const
{
    // TODO: 实现周报生成逻辑
    WeeklyTrend trend;
    trend.weekStart = weekStart;
    trend.avgHealthScore = 0.0;
    trend.totalActiveHours = 0;
    trend.totalBreaks = 0;
    return trend;
}

QList<DataAnalyzer::HealthInsight> DataAnalyzer::getHealthInsights() const
{
    // TODO: 实现健康洞察生成逻辑
    return m_insights;
}

QString DataAnalyzer::getStatsSummary() const
{
    int activeMinutesToday = getDailyReport(QDate::currentDate()).totalActiveMinutes;
    return QString("今日已专注工作 %1 小时 %2 分钟。").arg(activeMinutesToday / 60).arg(activeMinutesToday % 60);
}

void DataAnalyzer::saveDataToFile()
{
    QString path = getDataFilePath();
    if (path.isEmpty()) {
        qWarning() << "Could not get data file path. Data not saved.";
        return;
    }

    QJsonObject rootObj;
    
    QJsonArray activitiesArray;
    for (const auto& record : m_activityRecords) {
        QJsonObject activityObj;
        activityObj["timestamp"] = record.timestamp.toString(Qt::ISODate);
        activityObj["mouseClicks"] = record.data.mouseClicks;
        activityObj["keystrokes"] = record.data.keystrokes;
        activityObj["isActive"] = record.data.isActive;
        activityObj["activeWindow"] = record.data.activeWindow;
        activitiesArray.append(activityObj);
    }
    rootObj["activities"] = activitiesArray;

    QJsonArray healthEventsArray;
    for (const auto& record : m_healthEvents) {
        QJsonObject eventObj;
        eventObj["timestamp"] = record.timestamp.toString(Qt::ISODate);
        eventObj["type"] = static_cast<int>(record.type);
        eventObj["action"] = record.action;
        healthEventsArray.append(eventObj);
    }
    rootObj["health_events"] = healthEventsArray;

    QJsonDocument doc(rootObj);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    } else {
        qWarning() << "Could not open file for writing:" << path;
    }
}

void DataAnalyzer::loadDataFromFile()
{
    QString path = getDataFilePath();
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << "Data file does not exist, starting fresh:" << path;
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file for reading:" << path;
        return;
    }

    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Failed to parse data file, it might be corrupted.";
        return;
    }

    QJsonObject rootObj = doc.object();
    m_activityRecords.clear();
    m_healthEvents.clear();

    if (rootObj.contains("activities") && rootObj["activities"].isArray()) {
        QJsonArray activitiesArray = rootObj["activities"].toArray();
        for (const auto& val : activitiesArray) {
            QJsonObject obj = val.toObject();
            ActivityRecord record;
            record.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
            record.data.timestamp = record.timestamp;
            record.data.mouseClicks = obj["mouseClicks"].toInt();
            record.data.keystrokes = obj["keystrokes"].toInt();
            record.data.isActive = obj["isActive"].toBool();
            record.data.activeWindow = obj["activeWindow"].toString();
            m_activityRecords.append(record);
        }
    }

    if (rootObj.contains("health_events") && rootObj["health_events"].isArray()) {
        QJsonArray healthEventsArray = rootObj["health_events"].toArray();
        for (const auto& val : healthEventsArray) {
            QJsonObject obj = val.toObject();
            HealthEventRecord record;
            record.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
            record.type = static_cast<HealthEngine::ReminderType>(obj["type"].toInt());
            record.action = obj["action"].toString();
            m_healthEvents.append(record);
        }
    }
    emit dataUpdated();
}

QString DataAnalyzer::getDataFilePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dataDir.isEmpty()) {
        return "";
    }

    QDir dir(dataDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Could not create data directory:" << dataDir;
            return "";
        }
    }
    
    return dataDir + "/activity_log.json";
}

void DataAnalyzer::analyzePatterns()
{
    // TODO: 实现模式分析逻辑
    generateInsights();
    
    // 定期保存数据
    saveDataToFile();
}

void DataAnalyzer::generateInsights()
{
    // TODO: 实现洞察生成逻辑
}
