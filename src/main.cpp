#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QTimer>
#include <iostream> // Added for std::cerr

#include "ui/SystemTrayIcon.h"
#include "core/ActivityMonitor.h"
#include "core/HealthEngine.h"
#include "core/ConfigManager.h"
#include "core/DataAnalyzer.h"
#include "utils/Logger.h"
#include "utils/SystemUtils.h"

int main(int argc, char *argv[])
{
    std::cerr << "DEBUG: Point 1 - main() entry" << std::endl;

    QApplication app(argc, argv);
    std::cerr << "DEBUG: Point 2 - QApplication constructed" << std::endl;

    // 设置应用程序信息
    app.setApplicationName("工位健康精灵");
    app.setApplicationDisplayName("Workstation Wellness Elf");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("WorkstationWellness");
    app.setOrganizationDomain("workstationwellness.com");
    std::cerr << "DEBUG: Point 3 - Application info set" << std::endl;

    // 检查系统托盘是否可用
    std::cerr << "DEBUG: Point 4 - Checking system tray availability" << std::endl;
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        std::cerr << "DEBUG: Point 5 - System tray not available" << std::endl;
        QMessageBox::critical(nullptr, "工位健康精灵",
                             "系统托盘不可用，程序无法正常运行。");
        return -1;
    }
    std::cerr << "DEBUG: Point 6 - System tray available" << std::endl;

    // 设置应用程序不在关闭最后一个窗口时退出
    QApplication::setQuitOnLastWindowClosed(false);
    std::cerr << "DEBUG: Point 7 - Quit on last window closed set" << std::endl;

    // 初始化日志系统
    Logger::initialize();
    Logger::info("工位健康精灵启动中...");
    std::cerr << "DEBUG: Point 8 - Logger initialized" << std::endl;

    // 初始化配置管理器
    ConfigManager configManager;
    if (!configManager.load()) {
        Logger::warning("配置文件加载失败，使用默认配置");
    }

    // 初始化核心模块
    ActivityMonitor activityMonitor;
    HealthEngine healthEngine;
    DataAnalyzer dataAnalyzer;

    // 初始化系统托盘
    SystemTrayIcon trayIcon(&dataAnalyzer);
    trayIcon.show();

    // 连接信号槽 - 活动监测 -> 健康引擎
    QObject::connect(&activityMonitor, &ActivityMonitor::activityDetected,
                     &healthEngine, &HealthEngine::onActivityDetected);

    // 连接信号槽 - 活动监测 -> 数据分析
    QObject::connect(&activityMonitor, &ActivityMonitor::activityDetected,
                     &dataAnalyzer, &DataAnalyzer::recordActivity);

    // 连接信号槽 - 健康引擎 -> 系统托盘
    QObject::connect(&healthEngine, &HealthEngine::reminderTriggered,
                     &trayIcon, &SystemTrayIcon::showReminder);

    // 连接信号槽 - 健康引擎 -> 数据分析（记录健康事件）
    QObject::connect(&healthEngine, &HealthEngine::reminderTriggered,
                     &dataAnalyzer, [&dataAnalyzer](HealthEngine::ReminderType type, const QString&, const QString&) {
                         dataAnalyzer.recordHealthEvent(type, "reminder_triggered");
                     });

    // 连接信号槽 - 健康统计更新 -> 系统托盘
    QObject::connect(&healthEngine, &HealthEngine::statsUpdated,
                     &trayIcon, &SystemTrayIcon::showQuickStats);

    // 连接信号槽 - 配置变更
    QObject::connect(&configManager, &ConfigManager::configChanged, [&]() {
        // 更新健康引擎配置
        auto reminderTypes = {
            HealthEngine::ReminderType::SittingTooLong,
            HealthEngine::ReminderType::EyeRest,
            HealthEngine::ReminderType::NeckExercise,
            HealthEngine::ReminderType::PostureCheck,
            HealthEngine::ReminderType::Hydration
        };

        for (auto type : reminderTypes) {
            auto config = configManager.getReminderConfig(type);
            healthEngine.configureReminder(type, config);
        }

        Logger::info("健康引擎配置已更新");
    });

    // 应用初始配置
    auto reminderTypes = {
        HealthEngine::ReminderType::SittingTooLong,
        HealthEngine::ReminderType::EyeRest,
        HealthEngine::ReminderType::NeckExercise,
        HealthEngine::ReminderType::PostureCheck,
        HealthEngine::ReminderType::Hydration
    };

    for (auto type : reminderTypes) {
        auto config = configManager.getReminderConfig(type);
        healthEngine.configureReminder(type, config);
    }

    // 启动核心模块
    activityMonitor.start();
    healthEngine.start();

    // 设置自动启动
    ConfigManager::GeneralConfig generalConfig = configManager.getGeneralConfig();
    if (generalConfig.autoStart) {
        SystemUtils::setAutoStart(true, app.applicationName(), app.applicationFilePath());
    }

    Logger::info("工位健康精灵启动完成");

    // 显示启动通知
    if (generalConfig.showNotifications) {
        trayIcon.showMessage("工位健康精灵",
                            "程序已启动，正在为您的健康保驾护航！\n双击托盘图标打开设置",
                            QSystemTrayIcon::Information, 3000);
    }

    // 设置退出时的清理工作
    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        Logger::info("工位健康精灵正在退出...");

        activityMonitor.stop();
        healthEngine.stop();

        // 保存配置和数据
        configManager.save();

        Logger::info("工位健康精灵已安全退出");
    });

    return app.exec();
}