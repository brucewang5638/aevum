#pragma once

#include <QObject>
#include <QJsonObject>
#include <QDateTime>
#include <QList>
#include "HealthEngine.h"
#include "ActivityMonitor.h"

/**
 * @brief 数据分析模块
 * 
 * 负责用户行为数据的统计分析，生成健康报告和趋势分析
 */
class DataAnalyzer : public QObject
{
    Q_OBJECT

public:
    struct DailyReport {
        QDate date;
        int totalActiveMinutes;      // 总活跃时间
        int totalBreaks;            // 总休息次数
        int longestSittingSession;  // 最长连续坐立时间
        double healthScore;         // 健康评分
        QList<QPair<QTime, QString>> events; // 事件时间线
    };

    struct WeeklyTrend {
        QDate weekStart;
        double avgHealthScore;      // 平均健康评分
        int totalActiveHours;      // 总活跃小时
        int totalBreaks;           // 总休息次数
        QList<double> dailyScores; // 每日评分
    };

    struct HealthInsight {
        QString title;             // 洞察标题
        QString description;       // 详细描述
        QString suggestion;        // 改进建议
        int priority;             // 优先级(1-5)
        QString category;         // 分类
    };

    explicit DataAnalyzer(QObject *parent = nullptr);
    ~DataAnalyzer();

    /**
     * @brief 记录用户活动数据
     */
    void recordActivity(const ActivityMonitor::ActivityData& data);

    /**
     * @brief 记录健康事件
     */
    void recordHealthEvent(HealthEngine::ReminderType type, const QString& action);

    /**
     * @brief 获取指定日期的报告
     */
    DailyReport getDailyReport(const QDate& date = QDate::currentDate()) const;

    /**
     * @brief 获取周趋势分析
     */
    WeeklyTrend getWeeklyTrend(const QDate& weekStart = QDate::currentDate()) const;

    /**
     * @brief 获取健康洞察建议
     */
    QList<HealthInsight> getHealthInsights() const;

    /**
     * @brief 导出数据到JSON
     */
    QJsonObject exportData(const QDate& startDate, const QDate& endDate) const;

    /**
     * @brief 从JSON导入数据
     */
    bool importData(const QJsonObject& data);

    /**
     * @brief 清理旧数据
     */
    void cleanupOldData(int retentionDays);

    /**
     * @brief 获取统计摘要
     */
    QString getStatsSummary() const;

signals:
    /**
     * @brief 新的健康洞察生成时发出
     */
    void newInsightGenerated(const HealthInsight& insight);

    /**
     * @brief 数据更新时发出
     */
    void dataUpdated();

private:
    void analyzePatterns();
    void generateInsights();
    double calculateDailyHealthScore(const QDate& date) const;
    void saveDataToFile();
    void loadDataFromFile();
    QString getDataFilePath() const;

    struct ActivityRecord {
        QDateTime timestamp;
        ActivityMonitor::ActivityData data;
    };

    struct HealthEventRecord {
        QDateTime timestamp;
        HealthEngine::ReminderType type;
        QString action;
    };

    QList<ActivityRecord> m_activityRecords;
    QList<HealthEventRecord> m_healthEvents;
    QList<HealthInsight> m_insights;
    
    QDateTime m_lastAnalysisTime;
    QTimer* m_analysisTimer;
};