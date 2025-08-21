#include "core/ConfigManager.h"
#include "utils/Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QJsonArray>
#include <QTime>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_settings(nullptr)
    , m_configLoaded(false)
{
    initializeDefaults();
    
    QString configDir = getDefaultConfigDir();
    QDir().mkpath(configDir);
    
    m_configFilePath = configDir + "/config.json";
    
    // 使用QSettings作为备用存储
    m_settings = new QSettings(this);
}

ConfigManager::~ConfigManager()
{
    if (m_configLoaded) {
        save();
    }
}

bool ConfigManager::load()
{
    // 首先尝试从JSON文件加载
    QFile configFile(m_configFilePath);
    if (configFile.open(QIODevice::ReadOnly)) {
        QByteArray data = configFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (!doc.isNull() && doc.isObject()) {
            jsonToConfig(doc.object());
            m_configLoaded = true;
            Logger::info("配置文件加载成功", "ConfigManager");
            return true;
        }
    }
    
    // 如果JSON加载失败，尝试从QSettings加载
    if (m_settings->contains("general/autoStart")) {
        loadFromQSettings();
        m_configLoaded = true;
        Logger::info("从QSettings加载配置", "ConfigManager");
        return true;
    }
    
    // 都失败则使用默认配置
    Logger::warning("配置文件加载失败，使用默认配置", "ConfigManager");
    m_configLoaded = true;
    save(); // 保存默认配置
    return false;
}

bool ConfigManager::save()
{
    if (!m_configLoaded) {
        return false;
    }
    
    // 保存到JSON文件
    QJsonObject configJson = configToJson();
    QJsonDocument doc(configJson);
    
    QFile configFile(m_configFilePath);
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(doc.toJson());
        configFile.close();
        
        // 同时保存到QSettings作为备份
        saveToQSettings();
        
        emit configChanged();
        Logger::info("配置文件保存成功", "ConfigManager");
        return true;
    }
    
    Logger::error("配置文件保存失败", "ConfigManager");
    return false;
}

void ConfigManager::resetToDefaults()
{
    initializeDefaults();
    save();
    Logger::info("配置已重置为默认值", "ConfigManager");
}

bool ConfigManager::exportConfig(const QString& filePath)
{
    QJsonObject configJson = configToJson();
    QJsonDocument doc(configJson);
    
    QFile exportFile(filePath);
    if (exportFile.open(QIODevice::WriteOnly)) {
        exportFile.write(doc.toJson());
        exportFile.close();
        Logger::info(QString("配置导出到: %1").arg(filePath), "ConfigManager");
        return true;
    }
    
    Logger::error(QString("配置导出失败: %1").arg(filePath), "ConfigManager");
    return false;
}

bool ConfigManager::importConfig(const QString& filePath)
{
    QFile importFile(filePath);
    if (importFile.open(QIODevice::ReadOnly)) {
        QByteArray data = importFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (!doc.isNull() && doc.isObject()) {
            jsonToConfig(doc.object());
            save();
            Logger::info(QString("配置导入成功: %1").arg(filePath), "ConfigManager");
            return true;
        }
    }
    
    Logger::error(QString("配置导入失败: %1").arg(filePath), "ConfigManager");
    return false;
}

HealthEngine::ReminderConfig ConfigManager::getReminderConfig(HealthEngine::ReminderType type) const
{
    return m_reminderConfigs.value(type, HealthEngine::ReminderConfig());
}

void ConfigManager::setGeneralConfig(const GeneralConfig& config)
{
    m_generalConfig = config;
    save();
}

void ConfigManager::setWorkSchedule(const WorkSchedule& schedule)
{
    bool wasWorking = isWorkingTime();
    m_workSchedule = schedule;
    save();
    
    bool isWorking = isWorkingTime();
    if (wasWorking != isWorking) {
        emit workingTimeChanged(isWorking);
    }
}

void ConfigManager::setAdvancedConfig(const AdvancedConfig& config)
{
    m_advancedConfig = config;
    save();
}

void ConfigManager::setReminderConfig(HealthEngine::ReminderType type, const HealthEngine::ReminderConfig& config)
{
    m_reminderConfigs[type] = config;
    save();
}

bool ConfigManager::isWorkingTime() const
{
    if (!m_workSchedule.respectSchedule) {
        return true;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    QTime currentTime = now.time();
    int currentDay = now.date().dayOfWeek(); // 1=Monday, 7=Sunday
    
    // 检查是否在工作日
    if (!m_workSchedule.workDays.contains(currentDay)) {
        return false;
    }
    
    // 检查是否在工作时间内
    return currentTime >= m_workSchedule.workStartTime && 
           currentTime <= m_workSchedule.workEndTime;
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configFilePath;
}

void ConfigManager::initializeDefaults()
{
    // 初始化默认基本配置
    m_generalConfig.autoStart = true;
    m_generalConfig.minimizeToTray = true;
    m_generalConfig.language = "zh_CN";
    m_generalConfig.soundEnabled = true;
    m_generalConfig.showNotifications = true;
    m_generalConfig.notificationDuration = 5;
    
    // 初始化默认工作时间
    m_workSchedule.workStartTime = QTime(9, 0);
    m_workSchedule.workEndTime = QTime(18, 0);
    m_workSchedule.workDays = {1, 2, 3, 4, 5}; // Monday to Friday
    m_workSchedule.respectSchedule = true;
    
    // 初始化默认高级配置
    m_advancedConfig.collectAnonymousStats = false;
    m_advancedConfig.enableLogging = true;
    m_advancedConfig.logLevel = "INFO";
    m_advancedConfig.dataRetentionDays = 30;
    m_advancedConfig.enableSmartAdaptation = true;
    
    // 初始化默认提醒配置
    HealthEngine::ReminderConfig sittingConfig;
    sittingConfig.enabled = true;
    sittingConfig.intervalMinutes = 30;
    sittingConfig.durationSeconds = 10;
    sittingConfig.message = "您已连续工作30分钟了";
    sittingConfig.suggestion = "请起身活动一下，伸展腰背，促进血液循环。";
    m_reminderConfigs[HealthEngine::ReminderType::SittingTooLong] = sittingConfig;
    
    HealthEngine::ReminderConfig eyeConfig;
    eyeConfig.enabled = true;
    eyeConfig.intervalMinutes = 20;
    eyeConfig.durationSeconds = 8;
    eyeConfig.message = "该让眼睛休息一下了";
    eyeConfig.suggestion = "请看向20英尺外的物体20秒钟，缓解眼部疲劳。";
    m_reminderConfigs[HealthEngine::ReminderType::EyeRest] = eyeConfig;
    
    HealthEngine::ReminderConfig neckConfig;
    neckConfig.enabled = true;
    neckConfig.intervalMinutes = 45;
    neckConfig.durationSeconds = 12;
    neckConfig.message = "关爱您的颈椎健康";
    neckConfig.suggestion = "请缓慢转动头部，伸展颈椎，每个动作保持5秒。";
    m_reminderConfigs[HealthEngine::ReminderType::NeckExercise] = neckConfig;
}

QJsonObject ConfigManager::configToJson() const
{
    QJsonObject root;
    
    // 基本配置
    QJsonObject general;
    general["autoStart"] = m_generalConfig.autoStart;
    general["minimizeToTray"] = m_generalConfig.minimizeToTray;
    general["language"] = m_generalConfig.language;
    general["soundEnabled"] = m_generalConfig.soundEnabled;
    general["showNotifications"] = m_generalConfig.showNotifications;
    general["notificationDuration"] = m_generalConfig.notificationDuration;
    root["general"] = general;
    
    // 工作时间配置
    QJsonObject schedule;
    schedule["workStartTime"] = m_workSchedule.workStartTime.toString("hh:mm");
    schedule["workEndTime"] = m_workSchedule.workEndTime.toString("hh:mm");
    QJsonArray workDays;
    for (int day : m_workSchedule.workDays) {
        workDays.append(day);
    }
    schedule["workDays"] = workDays;
    schedule["respectSchedule"] = m_workSchedule.respectSchedule;
    root["schedule"] = schedule;
    
    // 高级配置
    QJsonObject advanced;
    advanced["collectAnonymousStats"] = m_advancedConfig.collectAnonymousStats;
    advanced["enableLogging"] = m_advancedConfig.enableLogging;
    advanced["logLevel"] = m_advancedConfig.logLevel;
    advanced["dataRetentionDays"] = m_advancedConfig.dataRetentionDays;
    advanced["enableSmartAdaptation"] = m_advancedConfig.enableSmartAdaptation;
    root["advanced"] = advanced;
    
    // 提醒配置
    QJsonObject reminders;
    for (auto it = m_reminderConfigs.begin(); it != m_reminderConfigs.end(); ++it) {
        QJsonObject reminderObj;
        reminderObj["enabled"] = it.value().enabled;
        reminderObj["intervalMinutes"] = it.value().intervalMinutes;
        reminderObj["durationSeconds"] = it.value().durationSeconds;
        reminderObj["message"] = it.value().message;
        reminderObj["suggestion"] = it.value().suggestion;
        
        QString key = QString::number(static_cast<int>(it.key()));
        reminders[key] = reminderObj;
    }
    root["reminders"] = reminders;
    
    return root;
}

void ConfigManager::jsonToConfig(const QJsonObject& json)
{
    // 加载基本配置
    if (json.contains("general")) {
        QJsonObject general = json["general"].toObject();
        m_generalConfig.autoStart = general["autoStart"].toBool(true);
        m_generalConfig.minimizeToTray = general["minimizeToTray"].toBool(true);
        m_generalConfig.language = general["language"].toString("zh_CN");
        m_generalConfig.soundEnabled = general["soundEnabled"].toBool(true);
        m_generalConfig.showNotifications = general["showNotifications"].toBool(true);
        m_generalConfig.notificationDuration = general["notificationDuration"].toInt(5);
    }
    
    // 加载工作时间配置
    if (json.contains("schedule")) {
        QJsonObject schedule = json["schedule"].toObject();
        m_workSchedule.workStartTime = QTime::fromString(schedule["workStartTime"].toString("09:00"), "hh:mm");
        m_workSchedule.workEndTime = QTime::fromString(schedule["workEndTime"].toString("18:00"), "hh:mm");
        m_workSchedule.respectSchedule = schedule["respectSchedule"].toBool(true);
        
        m_workSchedule.workDays.clear();
        QJsonArray workDays = schedule["workDays"].toArray();
        for (const QJsonValue& day : workDays) {
            m_workSchedule.workDays.append(day.toInt());
        }
        if (m_workSchedule.workDays.isEmpty()) {
            m_workSchedule.workDays = {1, 2, 3, 4, 5};
        }
    }
    
    // 加载高级配置
    if (json.contains("advanced")) {
        QJsonObject advanced = json["advanced"].toObject();
        m_advancedConfig.collectAnonymousStats = advanced["collectAnonymousStats"].toBool(false);
        m_advancedConfig.enableLogging = advanced["enableLogging"].toBool(true);
        m_advancedConfig.logLevel = advanced["logLevel"].toString("INFO");
        m_advancedConfig.dataRetentionDays = advanced["dataRetentionDays"].toInt(30);
        m_advancedConfig.enableSmartAdaptation = advanced["enableSmartAdaptation"].toBool(true);
    }
    
    // 加载提醒配置
    if (json.contains("reminders")) {
        QJsonObject reminders = json["reminders"].toObject();
        for (auto it = reminders.begin(); it != reminders.end(); ++it) {
            int typeInt = it.key().toInt();
            HealthEngine::ReminderType type = static_cast<HealthEngine::ReminderType>(typeInt);
            
            QJsonObject reminderObj = it.value().toObject();
            HealthEngine::ReminderConfig config;
            config.enabled = reminderObj["enabled"].toBool(true);
            config.intervalMinutes = reminderObj["intervalMinutes"].toInt(30);
            config.durationSeconds = reminderObj["durationSeconds"].toInt(5);
            config.message = reminderObj["message"].toString();
            config.suggestion = reminderObj["suggestion"].toString();
            
            m_reminderConfigs[type] = config;
        }
    }
}

QString ConfigManager::getDefaultConfigDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

void ConfigManager::loadFromQSettings()
{
    // 从QSettings加载配置的实现
    m_settings->beginGroup("general");
    m_generalConfig.autoStart = m_settings->value("autoStart", true).toBool();
    m_generalConfig.minimizeToTray = m_settings->value("minimizeToTray", true).toBool();
    m_generalConfig.language = m_settings->value("language", "zh_CN").toString();
    m_settings->endGroup();
    
    // 加载其他配置组...
}

void ConfigManager::saveToQSettings()
{
    // 保存配置到QSettings的实现
    m_settings->beginGroup("general");
    m_settings->setValue("autoStart", m_generalConfig.autoStart);
    m_settings->setValue("minimizeToTray", m_generalConfig.minimizeToTray);
    m_settings->setValue("language", m_generalConfig.language);
    m_settings->endGroup();
    
    // 保存其他配置组...
}