#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <memory>

/**
 * @brief 用户活动监测模块
 * 
 * 负责监测用户的键盘、鼠标活动以及屏幕使用时间
 * 提供活动数据给健康引擎进行分析
 */
class ActivityMonitor : public QObject
{
    Q_OBJECT

public:
    struct ActivityData {
        QDateTime timestamp;
        int mouseClicks;      // 鼠标点击次数
        int keystrokes;       // 键盘输入次数
        bool isActive;        // 是否活跃状态
        QString activeWindow; // 当前活跃窗口
    };

    explicit ActivityMonitor(QObject *parent = nullptr);
    ~ActivityMonitor();

    /**
     * @brief 开始监测用户活动
     */
    void start();

    /**
     * @brief 停止监测
     */
    void stop();

    /**
     * @brief 获取当前活动状态
     */
    bool isUserActive() const;

    /**
     * @brief 获取最后活动时间
     */
    QDateTime getLastActivityTime() const;

    /**
     * @brief 获取今日总活动时间（分钟）
     */
    int getTodayActiveMinutes() const;

signals:
    /**
     * @brief 检测到用户活动时发出
     */
    void activityDetected(const ActivityData& data);

    /**
     * @brief 用户变为非活跃状态时发出
     */
    void userBecameInactive();

    /**
     * @brief 用户重新变为活跃状态时发出
     */
    void userBecameActive();

private slots:
    void checkActivity();

private:
    void initializeSystemHooks();
    void cleanupSystemHooks();
    int getCurrentMouseClicks();
    int getCurrentKeystrokes();
    QString getCurrentActiveWindow();

    QTimer* m_timer;
    QDateTime m_lastActivityTime;
    QDateTime m_sessionStartTime;
    bool m_isActive;
    int m_todayActiveMinutes;
    
    // 平台相关的私有数据
    class Private;
    std::unique_ptr<Private> d;
};