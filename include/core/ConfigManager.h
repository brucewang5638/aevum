#pragma once

#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include "HealthEngine.h"

/**
 * @brief 配置管理器
 * 
 * 负责应用程序配置的保存、加载和管理
 * 支持用户个性化设置和企业级配置
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    struct GeneralConfig {
        bool autoStart = true;           // 开机自启动
        bool minimizeToTray = true;      // 最小化到托盘
        QString language = "zh_CN";      // 界面语言
        bool soundEnabled = true;        // 声音提醒
        bool showNotifications = true;   // 显示通知
        int notificationDuration = 5;    // 通知持续时间（秒）
    };

    struct WorkSchedule {
        QTime workStartTime{9, 0};       // 工作开始时间
        QTime workEndTime{18, 0};        // 工作结束时间
        QList<int> workDays{1,2,3,4,5};  // 工作日（1=周一，7=周日）
        bool respectSchedule = true;     // 是否遵循工作时间
    };

    struct AdvancedConfig {
        bool collectAnonymousStats = false;  // 收集匿名统计
        bool enableLogging = true;           // 启用日志
        QString logLevel = "INFO";           // 日志级别
        int dataRetentionDays = 30;          // 数据保留天数
        bool enableSmartAdaptation = true;   // 智能适应
    };

    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();

    /**
     * @brief 加载配置文件
     */
    bool load();

    /**
     * @brief 保存配置文件
     */
    bool save();

    /**
     * @brief 重置为默认配置
     */
    void resetToDefaults();

    /**
     * @brief 导出配置到文件
     */
    bool exportConfig(const QString& filePath);

    /**
     * @brief 从文件导入配置
     */
    bool importConfig(const QString& filePath);

    // Getter 方法
    GeneralConfig getGeneralConfig() const { return m_generalConfig; }
    WorkSchedule getWorkSchedule() const { return m_workSchedule; }
    AdvancedConfig getAdvancedConfig() const { return m_advancedConfig; }
    HealthEngine::ReminderConfig getReminderConfig(HealthEngine::ReminderType type) const;

    // Setter 方法
    void setGeneralConfig(const GeneralConfig& config);
    void setWorkSchedule(const WorkSchedule& schedule);
    void setAdvancedConfig(const AdvancedConfig& config);
    void setReminderConfig(HealthEngine::ReminderType type, const HealthEngine::ReminderConfig& config);

    /**
     * @brief 检查当前是否在工作时间内
     */
    bool isWorkingTime() const;

    /**
     * @brief 获取配置文件路径
     */
    QString getConfigFilePath() const;

signals:
    /**
     * @brief 配置变更时发出
     */
    void configChanged();

    /**
     * @brief 工作时间状态变更时发出
     */
    void workingTimeChanged(bool isWorking);

private:
    void initializeDefaults();
    QJsonObject configToJson() const;
    void jsonToConfig(const QJsonObject& json);
    QString getDefaultConfigDir() const;
    void loadFromQSettings();
    void saveToQSettings();

    GeneralConfig m_generalConfig;
    WorkSchedule m_workSchedule;
    AdvancedConfig m_advancedConfig;
    QMap<HealthEngine::ReminderType, HealthEngine::ReminderConfig> m_reminderConfigs;

    QSettings* m_settings;
    QString m_configFilePath;
    bool m_configLoaded;
};