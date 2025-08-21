#include "core/HealthEngine.h"
#include "utils/Logger.h"
#include <QDateTime>

HealthEngine::HealthEngine(QObject *parent)
    : QObject(parent)
    , m_sessionStartTime(QDateTime::currentDateTime())
    , m_lastSittingBreak(QDateTime::currentDateTime())
    , m_lastEyeBreak(QDateTime::currentDateTime())
    , m_lastNeckExercise(QDateTime::currentDateTime())
    , m_sittingTimer(new QTimer(this))
    , m_eyeRestTimer(new QTimer(this))
    , m_neckTimer(new QTimer(this))
    , m_statsTimer(new QTimer(this))
    , m_remindersPaused(false)
    , m_continuousSittingMinutes(0)
    , m_isCurrentlyActive(false)
{
    initializeDefaultConfigs();
    
    // 设置计时器
    m_sittingTimer->setInterval(60000); // 每分钟检查一次
    m_eyeRestTimer->setInterval(60000);
    m_neckTimer->setInterval(60000);
    m_statsTimer->setInterval(300000); // 每5分钟更新一次统计
    
    // 连接信号槽
    connect(m_sittingTimer, &QTimer::timeout, this, &HealthEngine::checkSittingTime);
    connect(m_eyeRestTimer, &QTimer::timeout, this, &HealthEngine::checkEyeRest);
    connect(m_neckTimer, &QTimer::timeout, this, &HealthEngine::checkNeckExercise);
    connect(m_statsTimer, &QTimer::timeout, this, &HealthEngine::updateHealthScore);
}

HealthEngine::~HealthEngine()
{
}

void HealthEngine::start()
{
    Logger::info("健康引擎启动", "HealthEngine");
    
    m_sessionStartTime = QDateTime::currentDateTime();
    m_lastSittingBreak = QDateTime::currentDateTime();
    m_lastEyeBreak = QDateTime::currentDateTime();
    m_lastNeckExercise = QDateTime::currentDateTime();
    
    // 重置统计数据
    m_todayStats = HealthStats();
    m_todayStats.healthScore = 100.0;
    
    // 启动所有计时器
    m_sittingTimer->start();
    m_eyeRestTimer->start();
    m_neckTimer->start();
    m_statsTimer->start();
}

void HealthEngine::stop()
{
    Logger::info("健康引擎停止", "HealthEngine");
    
    m_sittingTimer->stop();
    m_eyeRestTimer->stop();
    m_neckTimer->stop();
    m_statsTimer->stop();
}

void HealthEngine::configureReminder(ReminderType type, const ReminderConfig& config)
{
    m_configs[type] = config;
    Logger::info(QString("配置提醒类型 %1: 间隔%2分钟").arg(static_cast<int>(type)).arg(config.intervalMinutes), "HealthEngine");
}

HealthEngine::ReminderConfig HealthEngine::getReminderConfig(ReminderType type) const
{
    return m_configs.value(type, ReminderConfig());
}

HealthEngine::HealthStats HealthEngine::getTodayStats() const
{
    return m_todayStats;
}

void HealthEngine::takeBreak(ReminderType type)
{
    QDateTime now = QDateTime::currentDateTime();
    
    switch (type) {
    case ReminderType::SittingTooLong:
        m_lastSittingBreak = now;
        m_continuousSittingMinutes = 0;
        m_todayStats.eyeBreaksTaken++;
        break;
    case ReminderType::EyeRest:
        m_lastEyeBreak = now;
        m_todayStats.eyeBreaksTaken++;
        break;
    case ReminderType::NeckExercise:
        m_lastNeckExercise = now;
        m_todayStats.neckExercisesDone++;
        break;
    default:
        break;
    }
    
    logHealthEvent(type, "manual_break_taken");
    updateHealthScore();
    
    Logger::info(QString("用户手动进行了休息: %1").arg(static_cast<int>(type)), "HealthEngine");
}

void HealthEngine::pauseReminders(int minutes)
{
    m_remindersPaused = true;
    m_pauseEndTime = QDateTime::currentDateTime().addSecs(minutes * 60);
    
    Logger::info(QString("提醒已暂停 %1 分钟").arg(minutes), "HealthEngine");
}

void HealthEngine::onActivityDetected(const ActivityMonitor::ActivityData& data)
{
    m_isCurrentlyActive = data.isActive;
    
    if (data.isActive) {
        m_continuousSittingMinutes++;
        m_todayStats.totalSittingMinutes++;
        
        // 更新最长连续坐立时间
        if (m_continuousSittingMinutes > m_todayStats.longestSittingSession) {
            m_todayStats.longestSittingSession = m_continuousSittingMinutes;
        }
    }
}

void HealthEngine::checkSittingTime()
{
    if (m_remindersPaused && QDateTime::currentDateTime() < m_pauseEndTime) {
        return;
    }
    
    ReminderConfig config = m_configs[ReminderType::SittingTooLong];
    if (!config.enabled) {
        return;
    }
    
    int minutesSinceLastBreak = m_lastSittingBreak.secsTo(QDateTime::currentDateTime()) / 60;
    
    if (minutesSinceLastBreak >= config.intervalMinutes && m_isCurrentlyActive) {
        emit reminderTriggered(ReminderType::SittingTooLong, config.message, config.suggestion);
        logHealthEvent(ReminderType::SittingTooLong, "reminder_triggered");
        
        Logger::info(QString("触发久坐提醒，已连续坐立 %1 分钟").arg(minutesSinceLastBreak), "HealthEngine");
    }
}

void HealthEngine::checkEyeRest()
{
    if (m_remindersPaused && QDateTime::currentDateTime() < m_pauseEndTime) {
        return;
    }
    
    ReminderConfig config = m_configs[ReminderType::EyeRest];
    if (!config.enabled) {
        return;
    }
    
    int minutesSinceLastBreak = m_lastEyeBreak.secsTo(QDateTime::currentDateTime()) / 60;
    
    if (minutesSinceLastBreak >= config.intervalMinutes && m_isCurrentlyActive) {
        emit reminderTriggered(ReminderType::EyeRest, config.message, config.suggestion);
        logHealthEvent(ReminderType::EyeRest, "reminder_triggered");
        
        Logger::info(QString("触发眼部休息提醒，距离上次休息 %1 分钟").arg(minutesSinceLastBreak), "HealthEngine");
    }
}

void HealthEngine::checkNeckExercise()
{
    if (m_remindersPaused && QDateTime::currentDateTime() < m_pauseEndTime) {
        return;
    }
    
    ReminderConfig config = m_configs[ReminderType::NeckExercise];
    if (!config.enabled) {
        return;
    }
    
    int minutesSinceLastExercise = m_lastNeckExercise.secsTo(QDateTime::currentDateTime()) / 60;
    
    if (minutesSinceLastExercise >= config.intervalMinutes && m_isCurrentlyActive) {
        emit reminderTriggered(ReminderType::NeckExercise, config.message, config.suggestion);
        logHealthEvent(ReminderType::NeckExercise, "reminder_triggered");
        
        Logger::info(QString("触发颈椎运动提醒，距离上次运动 %1 分钟").arg(minutesSinceLastExercise), "HealthEngine");
    }
}

void HealthEngine::updateHealthScore()
{
    double score = calculateHealthScore();
    m_todayStats.healthScore = score;
    
    emit statsUpdated(m_todayStats);
    
    Logger::debug(QString("健康评分更新: %1").arg(score), "HealthEngine");
}

void HealthEngine::initializeDefaultConfigs()
{
    // 久坐提醒配置
    ReminderConfig sittingConfig;
    sittingConfig.enabled = true;
    sittingConfig.intervalMinutes = 30;
    sittingConfig.durationSeconds = 10;
    sittingConfig.message = "您已连续工作30分钟了";
    sittingConfig.suggestion = "请起身活动一下，伸展腰背，促进血液循环。建议走动2-3分钟。";
    m_configs[ReminderType::SittingTooLong] = sittingConfig;
    
    // 眼部休息配置
    ReminderConfig eyeConfig;
    eyeConfig.enabled = true;
    eyeConfig.intervalMinutes = 20;
    eyeConfig.durationSeconds = 8;
    eyeConfig.message = "该让眼睛休息一下了";
    eyeConfig.suggestion = "请看向20英尺(6米)外的物体20秒钟，或闭眼休息片刻，缓解眼部疲劳。";
    m_configs[ReminderType::EyeRest] = eyeConfig;
    
    // 颈椎运动配置
    ReminderConfig neckConfig;
    neckConfig.enabled = true;
    neckConfig.intervalMinutes = 45;
    neckConfig.durationSeconds = 12;
    neckConfig.message = "关爱您的颈椎健康";
    neckConfig.suggestion = "请缓慢地左右转动头部，上下点头，前后伸展颈部，每个动作保持5秒。";
    m_configs[ReminderType::NeckExercise] = neckConfig;
    
    // 姿势检查配置
    ReminderConfig postureConfig;
    postureConfig.enabled = true;
    postureConfig.intervalMinutes = 60;
    postureConfig.durationSeconds = 6;
    postureConfig.message = "检查一下您的坐姿";
    postureConfig.suggestion = "保持背部挺直，双脚平放地面，显示器顶部与眼睛水平。";
    m_configs[ReminderType::PostureCheck] = postureConfig;
    
    // 喝水提醒配置
    ReminderConfig hydrationConfig;
    hydrationConfig.enabled = true;
    hydrationConfig.intervalMinutes = 90;
    hydrationConfig.durationSeconds = 5;
    hydrationConfig.message = "别忘记补充水分";
    hydrationConfig.suggestion = "请喝一杯水，保持身体水分充足，有助于提高工作效率。";
    m_configs[ReminderType::Hydration] = hydrationConfig;
}

void HealthEngine::logHealthEvent(ReminderType type, const QString& action)
{
    // 这里可以记录健康事件到数据分析模块
    QString eventType;
    switch (type) {
    case ReminderType::SittingTooLong: eventType = "sitting"; break;
    case ReminderType::EyeRest: eventType = "eye_rest"; break;
    case ReminderType::NeckExercise: eventType = "neck_exercise"; break;
    case ReminderType::PostureCheck: eventType = "posture"; break;
    case ReminderType::Hydration: eventType = "hydration"; break;
    }
    
    Logger::debug(QString("健康事件: %1 - %2").arg(eventType).arg(action), "HealthEngine");
}

double HealthEngine::calculateHealthScore() const
{
    double score = 100.0;
    
    // 根据连续坐立时间扣分
    if (m_continuousSittingMinutes > 60) {
        score -= (m_continuousSittingMinutes - 60) * 0.5; // 超过1小时每分钟扣0.5分
    }
    
    // 根据今日最长连续坐立时间扣分
    if (m_todayStats.longestSittingSession > 120) {
        score -= (m_todayStats.longestSittingSession - 120) * 0.3;
    }
    
    // 根据休息次数加分
    int totalBreaks = m_todayStats.eyeBreaksTaken + m_todayStats.neckExercisesDone;
    int expectedBreaks = m_todayStats.totalSittingMinutes / 30; // 期望每30分钟休息一次
    
    if (totalBreaks < expectedBreaks) {
        score -= (expectedBreaks - totalBreaks) * 2.0; // 缺少休息扣分
    }
    
    // 确保分数在0-100范围内
    score = qMax(0.0, qMin(100.0, score));
    
    return score;
}