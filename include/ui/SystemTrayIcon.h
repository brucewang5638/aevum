#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include "core/HealthEngine.h"

class SettingsDialog;
class NotificationWidget;
class StatisticsPanel;
class DataAnalyzer;

/**
 * @brief 系统托盘图标管理
 * 
 * 提供系统托盘功能，包括右键菜单、通知显示等
 */
class SystemTrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    explicit SystemTrayIcon(DataAnalyzer* analyzer, QObject *parent = nullptr);
    ~SystemTrayIcon();

    /**
     * @brief 显示健康提醒
     */
    void showReminder(HealthEngine::ReminderType type, 
                     const QString& message, 
                     const QString& suggestion);

    /**
     * @brief 更新托盘图标状态
     */
    void updateIcon(bool isWorking, bool hasReminder = false);

    /**
     * @brief 显示快速统计信息
     */
    void showQuickStats(const HealthEngine::HealthStats& stats);

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showSettings();
    void showAbout();
    void togglePause();
    void takeBreakNow();
    void showStatisticsPanel();
    void onPauseTimerTimeout();
    void onNotificationClicked();

private:
    void createActions();
    void createContextMenu();
    void updatePauseAction();
    void showCustomNotification(const QString& title, 
                               const QString& message,
                               HealthEngine::ReminderType type);

    // 菜单动作
    QAction* m_settingsAction;
    QAction* m_pauseAction;
    QAction* m_takeBreakAction;
    QAction* m_statsAction;
    QAction* m_aboutAction;
    QAction* m_quitAction;
    
    // 子菜单
    QMenu* m_breakMenu;
    QAction* m_eyeBreakAction;
    QAction* m_neckBreakAction;
    QAction* m_sittingBreakAction;

    QMenu* m_contextMenu;
    SettingsDialog* m_settingsDialog;
    NotificationWidget* m_notificationWidget;
    StatisticsPanel* m_statisticsPanel;
    DataAnalyzer* m_analyzer;

    QTimer* m_pauseTimer;
    bool m_isPaused;
    int m_pauseMinutesLeft;

    // 图标状态
    QIcon m_normalIcon;
    QIcon m_workingIcon;
    QIcon m_reminderIcon;
    QIcon m_pausedIcon;
};