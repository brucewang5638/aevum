#include "ui/SystemTrayIcon.h"
#include "ui/SettingsDialog.h"
#include "ui/NotificationWidget.h"
#include "ui/StatisticsPanel.h"
#include "core/DataAnalyzer.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QMessageBox>

SystemTrayIcon::SystemTrayIcon(DataAnalyzer* analyzer, QObject *parent)
    : QSystemTrayIcon(parent)
    , m_analyzer(analyzer)
    , m_settingsDialog(nullptr)
    , m_notificationWidget(nullptr)
    , m_statisticsPanel(nullptr)
    , m_pauseTimer(new QTimer(this))
    , m_isPaused(false)
    , m_pauseMinutesLeft(0)
{
    createActions();
    createContextMenu();
    
    // 设置默认图标
    m_normalIcon = QIcon(":/icons/tray.png");
    m_workingIcon = QIcon(":/icons/working.png");
    m_reminderIcon = QIcon(":/icons/reminder.png");
    m_pausedIcon = QIcon(":/icons/paused.png");
    
    setIcon(m_normalIcon);
    setToolTip("工位健康精灵");
    
    connect(this, &QSystemTrayIcon::activated, 
            this, &SystemTrayIcon::onTrayIconActivated);
    
    connect(m_pauseTimer, &QTimer::timeout,
            this, &SystemTrayIcon::onPauseTimerTimeout);
    
    Logger::info("系统托盘图标初始化完成");
}

SystemTrayIcon::~SystemTrayIcon()
{
    if (m_settingsDialog) {
        m_settingsDialog->deleteLater();
    }
    if (m_notificationWidget) {
        m_notificationWidget->deleteLater();
    }
}

void SystemTrayIcon::showReminder(HealthEngine::ReminderType type, 
                                 const QString& message, 
                                 const QString& suggestion)
{
    if (m_isPaused) {
        Logger::info("提醒已暂停，跳过显示");
        return;
    }
    
    // 显示自定义通知
    showCustomNotification("健康提醒", message, type);
    
    // 更新托盘图标
    updateIcon(true, true);
    
    Logger::info(QString("显示健康提醒: %1").arg(message));
}

void SystemTrayIcon::updateIcon(bool isWorking, bool hasReminder)
{
    if (m_isPaused) {
        setIcon(m_pausedIcon);
    } else if (hasReminder) {
        setIcon(m_reminderIcon);
    } else if (isWorking) {
        setIcon(m_workingIcon);
    } else {
        setIcon(m_normalIcon);
    }
}

void SystemTrayIcon::showQuickStats(const HealthEngine::HealthStats& stats)
{
    QString message = QString("今日统计:\n"
                             "坐立时间: %1分钟\n"
                             "休息次数: %2次\n"
                             "健康评分: %3/100")
                             .arg(stats.totalSittingMinutes)
                             .arg(stats.eyeBreaksTaken + stats.neckExercisesDone)
                             .arg(qRound(stats.healthScore));
    
    showMessage("每日统计", message, Information, 5000);
}

void SystemTrayIcon::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::DoubleClick:
        showSettings();
        break;
    case QSystemTrayIcon::MiddleClick:
        showStatisticsPanel();
        break;
    default:
        break;
    }
}

void SystemTrayIcon::showSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog();
        // 这里需要设置配置管理器，但在构造函数中还没有访问权限
        // 将在主程序中设置
    }
    
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void SystemTrayIcon::showAbout()
{
    QMessageBox::about(nullptr, "关于工位健康精灵",
                      "工位健康精灵 v1.0.0\n\n"
                      "从\"被动记录\"走向\"主动关怀\"的智能健康伙伴\n\n"
                      "© 2024 WorkstationWellness");
}

void SystemTrayIcon::togglePause()
{
    if (m_isPaused) {
        // 取消暂停
        m_isPaused = false;
        m_pauseTimer->stop();
        updateIcon(true, false);
        showMessage("暂停取消", "健康提醒已恢复", Information, 2000);
    } else {
        // 暂停30分钟
        m_isPaused = true;
        m_pauseMinutesLeft = 30;
        m_pauseTimer->start(60000); // 每分钟更新一次
        updateIcon(true, false);
        showMessage("暂停提醒", "健康提醒已暂停30分钟", Information, 3000);
    }
    
    updatePauseAction();
}

void SystemTrayIcon::takeBreakNow()
{
    // 这里可以显示休息指导界面
    showMessage("休息时间", "请站起来活动一下，保护您的健康！", Information, 5000);
}

void SystemTrayIcon::showStatisticsPanel()
{
    if (!m_statisticsPanel) {
        m_statisticsPanel = new StatisticsPanel(m_analyzer);
    }
    m_statisticsPanel->show();
    m_statisticsPanel->raise();
    m_statisticsPanel->activateWindow();
}

void SystemTrayIcon::onPauseTimerTimeout()
{
    m_pauseMinutesLeft--;
    if (m_pauseMinutesLeft <= 0) {
        togglePause(); // 自动取消暂停
    } else {
        updatePauseAction();
    }
}

void SystemTrayIcon::onNotificationClicked()
{
    // 处理通知点击事件
    Logger::info("用户点击了通知");
}

void SystemTrayIcon::createActions()
{
    m_settingsAction = new QAction("设置(&S)", this);
    connect(m_settingsAction, &QAction::triggered, this, &SystemTrayIcon::showSettings);
    
    m_pauseAction = new QAction("暂停提醒(&P)", this);
    connect(m_pauseAction, &QAction::triggered, this, &SystemTrayIcon::togglePause);
    
    m_takeBreakAction = new QAction("立即休息(&B)", this);
    connect(m_takeBreakAction, &QAction::triggered, this, &SystemTrayIcon::takeBreakNow);
    
    m_statsAction = new QAction("统计数据(&T)", this);
    connect(m_statsAction, &QAction::triggered, this, &SystemTrayIcon::showStatisticsPanel);
    
    m_aboutAction = new QAction("关于(&A)", this);
    connect(m_aboutAction, &QAction::triggered, this, &SystemTrayIcon::showAbout);
    
    m_quitAction = new QAction("退出(&Q)", this);
    connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void SystemTrayIcon::createContextMenu()
{
    m_contextMenu = new QMenu();
    
    m_contextMenu->addAction(m_settingsAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_pauseAction);
    m_contextMenu->addAction(m_takeBreakAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_statsAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_aboutAction);
    m_contextMenu->addAction(m_quitAction);
    
    setContextMenu(m_contextMenu);
}

void SystemTrayIcon::updatePauseAction()
{
    if (m_isPaused) {
        m_pauseAction->setText(QString("取消暂停 (%1分钟)").arg(m_pauseMinutesLeft));
    } else {
        m_pauseAction->setText("暂停提醒(&P)");
    }
}

void SystemTrayIcon::showCustomNotification(const QString& title, 
                                           const QString& message,
                                           HealthEngine::ReminderType type)
{
    if (!m_notificationWidget) {
        m_notificationWidget = new NotificationWidget();
        connect(m_notificationWidget, &NotificationWidget::notificationClosed,
                this, &SystemTrayIcon::onNotificationClicked);
    }
    
    m_notificationWidget->showReminder(type, title, message);
}