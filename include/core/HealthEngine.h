#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "ActivityMonitor.h"

/**
 * @brief 健康提醒引擎
 * 
 * 根据用户活动数据分析，提供个性化的健康提醒
 * 包括颈椎保护、眼部休息、久坐提醒等功能
 */
class HealthEngine : public QObject
{
    Q_OBJECT

public:
    enum class ReminderType {
        SittingTooLong,     // 久坐提醒
        EyeRest,           // 眼部休息
        NeckExercise,      // 颈椎运动
        PostureCheck,      // 姿势检查
        Hydration         // 喝水提醒
    };

    struct ReminderConfig {
        bool enabled = true;
        int intervalMinutes = 30;    // 提醒间隔（分钟）
        int durationSeconds = 5;     // 提醒持续时间（秒）
        QString message;             // 提醒消息
        QString suggestion;          // 健康建议
    };

    struct HealthStats {
        int totalSittingMinutes;     // 总坐立时间
        int longestSittingSession;   // 最长连续坐立时间
        int eyeBreaksTaken;         // 眼部休息次数
        int neckExercisesDone;      // 颈椎运动次数
        double healthScore;         // 健康评分(0-100)
    };

    explicit HealthEngine(QObject *parent = nullptr);
    ~HealthEngine();

    /**
     * @brief 启动健康引擎
     */
    void start();

    /**
     * @brief 停止健康引擎
     */
    void stop();

    /**
     * @brief 配置特定类型的提醒
     */
    void configureReminder(ReminderType type, const ReminderConfig& config);

    /**
     * @brief 获取提醒配置
     */
    ReminderConfig getReminderConfig(ReminderType type) const;

    /**
     * @brief 获取今日健康统计
     */
    HealthStats getTodayStats() const;

    /**
     * @brief 手动触发休息
     */
    void takeBreak(ReminderType type);

    /**
     * @brief 暂停所有提醒（例如会议模式）
     */
    void pauseReminders(int minutes);

signals:
    /**
     * @brief 需要显示提醒时发出
     */
    void reminderTriggered(ReminderType type, const QString& message, const QString& suggestion);

    /**
     * @brief 健康统计更新时发出
     */
    void statsUpdated(const HealthStats& stats);

public slots:
    /**
     * @brief 接收用户活动数据
     */
    void onActivityDetected(const ActivityMonitor::ActivityData& data);

private slots:
    void checkSittingTime();
    void checkEyeRest();
    void checkNeckExercise();
    void updateHealthScore();

private:
    void initializeDefaultConfigs();
    void logHealthEvent(ReminderType type, const QString& action);
    double calculateHealthScore() const;

    QDateTime m_sessionStartTime;
    QDateTime m_lastSittingBreak;
    QDateTime m_lastEyeBreak;
    QDateTime m_lastNeckExercise;
    
    QTimer* m_sittingTimer;
    QTimer* m_eyeRestTimer;
    QTimer* m_neckTimer;
    QTimer* m_statsTimer;
    
    bool m_remindersPaused;
    QDateTime m_pauseEndTime;
    
    QMap<ReminderType, ReminderConfig> m_configs;
    HealthStats m_todayStats;
    
    int m_continuousSittingMinutes;
    bool m_isCurrentlyActive;
};