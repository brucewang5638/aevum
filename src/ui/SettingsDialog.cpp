#include "ui/SettingsDialog.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_configManager(nullptr)
{
    setupUI();
    setWindowTitle("工位健康精灵 - 设置");
    setWindowIcon(QIcon(":/icons/app.png"));
    resize(600, 500);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setConfigManager(ConfigManager* configManager)
{
    m_configManager = configManager;
    refreshSettings();
}

void SettingsDialog::refreshSettings()
{
    if (!m_configManager) return;
    
    loadGeneralSettings();
    loadReminderSettings();
    loadScheduleSettings();
    loadAdvancedSettings();
}

void SettingsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    refreshSettings();
}

void SettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget();
    
    createGeneralTab();
    createRemindersTab();
    createScheduleTab();
    createAdvancedTab();
    createAboutTab();
    
    m_tabWidget->addTab(m_generalTab, "基本设置");
    m_tabWidget->addTab(m_remindersTab, "提醒设置");
    m_tabWidget->addTab(m_scheduleTab, "工作时间");
    m_tabWidget->addTab(m_advancedTab, "高级设置");
    m_tabWidget->addTab(m_aboutTab, "关于");
    
    mainLayout->addWidget(m_tabWidget);
    
    // 底部按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_resetBtn = new QPushButton("重置默认");
    m_applyBtn = new QPushButton("应用");
    m_cancelBtn = new QPushButton("取消");
    m_okBtn = new QPushButton("确定");
    
    m_resetBtn->setStyleSheet("QPushButton { color: #f44336; }");
    m_applyBtn->setStyleSheet("QPushButton { background: #2196F3; color: white; padding: 8px 16px; }");
    m_okBtn->setStyleSheet("QPushButton { background: #4CAF50; color: white; padding: 8px 16px; }");
    
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetToDefaults);
    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApplySettings);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okBtn, &QPushButton::clicked, this, [this]() {
        onApplySettings();
        accept();
    });
    
    buttonLayout->addWidget(m_resetBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyBtn);
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_okBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::createGeneralTab()
{
    m_generalTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_generalTab);
    
    // 启动设置组
    QGroupBox* startupGroup = new QGroupBox("启动设置");
    QFormLayout* startupLayout = new QFormLayout(startupGroup);
    
    m_autoStartCheck = new QCheckBox("开机自动启动");
    m_minimizeToTrayCheck = new QCheckBox("启动时最小化到系统托盘");
    
    startupLayout->addRow(m_autoStartCheck);
    startupLayout->addRow(m_minimizeToTrayCheck);
    
    // 界面设置组
    QGroupBox* uiGroup = new QGroupBox("界面设置");
    QFormLayout* uiLayout = new QFormLayout(uiGroup);
    
    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("简体中文", "zh_CN");
    m_languageCombo->addItem("English", "en_US");
    
    uiLayout->addRow("界面语言:", m_languageCombo);
    
    // 通知设置组
    QGroupBox* notificationGroup = new QGroupBox("通知设置");
    QFormLayout* notificationLayout = new QFormLayout(notificationGroup);
    
    m_showNotificationsCheck = new QCheckBox("显示桌面通知");
    m_soundEnabledCheck = new QCheckBox("启用提示音");
    
    m_notificationDurationSpin = new QSpinBox();
    m_notificationDurationSpin->setRange(3, 30);
    m_notificationDurationSpin->setSuffix(" 秒");
    
    notificationLayout->addRow(m_showNotificationsCheck);
    notificationLayout->addRow(m_soundEnabledCheck);
    notificationLayout->addRow("通知持续时间:", m_notificationDurationSpin);
    
    layout->addWidget(startupGroup);
    layout->addWidget(uiGroup);
    layout->addWidget(notificationGroup);
    layout->addStretch();
}

void SettingsDialog::createRemindersTab()
{
    m_remindersTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_remindersTab);
    
    // 提醒类型选择
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("提醒类型:"));
    
    m_reminderTypeCombo = new QComboBox();
    m_reminderTypeCombo->addItem("久坐提醒", static_cast<int>(HealthEngine::ReminderType::SittingTooLong));
    m_reminderTypeCombo->addItem("眼部休息", static_cast<int>(HealthEngine::ReminderType::EyeRest));
    m_reminderTypeCombo->addItem("颈椎运动", static_cast<int>(HealthEngine::ReminderType::NeckExercise));
    m_reminderTypeCombo->addItem("姿势检查", static_cast<int>(HealthEngine::ReminderType::PostureCheck));
    m_reminderTypeCombo->addItem("喝水提醒", static_cast<int>(HealthEngine::ReminderType::Hydration));
    
    connect(m_reminderTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onReminderTypeChanged);
    
    typeLayout->addWidget(m_reminderTypeCombo);
    typeLayout->addStretch();
    
    // 提醒配置组
    QGroupBox* configGroup = new QGroupBox("提醒配置");
    QFormLayout* configLayout = new QFormLayout(configGroup);
    
    m_reminderEnabledCheck = new QCheckBox("启用此提醒");
    
    m_reminderIntervalSpin = new QSpinBox();
    m_reminderIntervalSpin->setRange(5, 180);
    m_reminderIntervalSpin->setSuffix(" 分钟");
    
    m_reminderDurationSpin = new QSpinBox();
    m_reminderDurationSpin->setRange(3, 30);
    m_reminderDurationSpin->setSuffix(" 秒");
    
    m_reminderMessageEdit = new QLineEdit();
    m_reminderSuggestionEdit = new QTextEdit();
    m_reminderSuggestionEdit->setMaximumHeight(80);
    
    m_testNotificationBtn = new QPushButton("测试通知");
    m_testNotificationBtn->setStyleSheet("QPushButton { background: #FF9800; color: white; padding: 6px 12px; }");
    connect(m_testNotificationBtn, &QPushButton::clicked, this, &SettingsDialog::onTestNotification);
    
    configLayout->addRow(m_reminderEnabledCheck);
    configLayout->addRow("提醒间隔:", m_reminderIntervalSpin);
    configLayout->addRow("显示时长:", m_reminderDurationSpin);
    configLayout->addRow("提醒消息:", m_reminderMessageEdit);
    configLayout->addRow("健康建议:", m_reminderSuggestionEdit);
    configLayout->addRow(m_testNotificationBtn);
    
    layout->addLayout(typeLayout);
    layout->addWidget(configGroup);
    layout->addStretch();
}

void SettingsDialog::createScheduleTab()
{
    m_scheduleTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_scheduleTab);
    
    // 工作时间设置组
    QGroupBox* timeGroup = new QGroupBox("工作时间设置");
    QFormLayout* timeLayout = new QFormLayout(timeGroup);
    
    m_respectScheduleCheck = new QCheckBox("仅在工作时间内提醒");
    
    m_workStartTimeEdit = new QTimeEdit();
    m_workStartTimeEdit->setDisplayFormat("hh:mm");
    
    m_workEndTimeEdit = new QTimeEdit();
    m_workEndTimeEdit->setDisplayFormat("hh:mm");
    
    timeLayout->addRow(m_respectScheduleCheck);
    timeLayout->addRow("工作开始时间:", m_workStartTimeEdit);
    timeLayout->addRow("工作结束时间:", m_workEndTimeEdit);
    
    // 工作日设置组
    QGroupBox* daysGroup = new QGroupBox("工作日设置");
    QVBoxLayout* daysLayout = new QVBoxLayout(daysGroup);
    
    m_workDaysList = new QListWidget();
    m_workDaysList->setMaximumHeight(150);
    
    QStringList weekDays = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    for (int i = 0; i < weekDays.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(weekDays[i]);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, i + 1); // 1=Monday, 7=Sunday
        m_workDaysList->addItem(item);
    }
    
    daysLayout->addWidget(new QLabel("选择工作日:"));
    daysLayout->addWidget(m_workDaysList);
    
    layout->addWidget(timeGroup);
    layout->addWidget(daysGroup);
    layout->addStretch();
}

void SettingsDialog::createAdvancedTab()
{
    m_advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_advancedTab);
    
    // 数据设置组
    QGroupBox* dataGroup = new QGroupBox("数据设置");
    QFormLayout* dataLayout = new QFormLayout(dataGroup);
    
    m_collectStatsCheck = new QCheckBox("收集匿名使用统计");
    m_enableLoggingCheck = new QCheckBox("启用日志记录");
    m_smartAdaptationCheck = new QCheckBox("启用智能适应");
    
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({"DEBUG", "INFO", "WARNING", "ERROR"});
    
    m_dataRetentionSpin = new QSpinBox();
    m_dataRetentionSpin->setRange(7, 365);
    m_dataRetentionSpin->setSuffix(" 天");
    
    dataLayout->addRow(m_collectStatsCheck);
    dataLayout->addRow(m_enableLoggingCheck);
    dataLayout->addRow("日志级别:", m_logLevelCombo);
    dataLayout->addRow("数据保留:", m_dataRetentionSpin);
    dataLayout->addRow(m_smartAdaptationCheck);
    
    // 配置管理组
    QGroupBox* configGroup = new QGroupBox("配置管理");
    QVBoxLayout* configLayout = new QVBoxLayout(configGroup);
    
    QHBoxLayout* configButtonLayout = new QHBoxLayout();
    
    m_exportConfigBtn = new QPushButton("导出配置");
    m_importConfigBtn = new QPushButton("导入配置");
    
    m_exportConfigBtn->setStyleSheet("QPushButton { background: #4CAF50; color: white; padding: 6px 12px; }");
    m_importConfigBtn->setStyleSheet("QPushButton { background: #2196F3; color: white; padding: 6px 12px; }");
    
    connect(m_exportConfigBtn, &QPushButton::clicked, this, &SettingsDialog::onExportConfig);
    connect(m_importConfigBtn, &QPushButton::clicked, this, &SettingsDialog::onImportConfig);
    
    configButtonLayout->addWidget(m_exportConfigBtn);
    configButtonLayout->addWidget(m_importConfigBtn);
    configButtonLayout->addStretch();
    
    configLayout->addLayout(configButtonLayout);
    
    layout->addWidget(dataGroup);
    layout->addWidget(configGroup);
    layout->addStretch();
}

void SettingsDialog::createAboutTab()
{
    m_aboutTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_aboutTab);
    
    // 应用信息
    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setAlignment(Qt::AlignCenter);
    
    QLabel* logoLabel = new QLabel();
    logoLabel->setText("🧘‍♂️");
    logoLabel->setStyleSheet("font-size: 64px;");
    logoLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* nameLabel = new QLabel("工位健康精灵");
    nameLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");
    nameLabel->setAlignment(Qt::AlignCenter);
    
    m_versionLabel = new QLabel("版本 1.0.0");
    m_versionLabel->setStyleSheet("font-size: 14px; color: #666;");
    m_versionLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* descLabel = new QLabel("从\"被动记录\"走向\"主动关怀\"的智能健康伙伴");
    descLabel->setStyleSheet("font-size: 12px; color: #888; font-style: italic;");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    
    m_copyrightLabel = new QLabel("© 2024 WorkstationWellness. All rights reserved.");
    m_copyrightLabel->setStyleSheet("font-size: 10px; color: #aaa;");
    m_copyrightLabel->setAlignment(Qt::AlignCenter);
    
    infoLayout->addWidget(logoLabel);
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(m_versionLabel);
    infoLayout->addSpacing(10);
    infoLayout->addWidget(descLabel);
    infoLayout->addSpacing(20);
    infoLayout->addWidget(m_copyrightLabel);
    
    layout->addStretch();
    layout->addLayout(infoLayout);
    layout->addStretch();
}

void SettingsDialog::loadGeneralSettings()
{
    if (!m_configManager) return;
    
    ConfigManager::GeneralConfig config = m_configManager->getGeneralConfig();
    
    m_autoStartCheck->setChecked(config.autoStart);
    m_minimizeToTrayCheck->setChecked(config.minimizeToTray);
    m_showNotificationsCheck->setChecked(config.showNotifications);
    m_soundEnabledCheck->setChecked(config.soundEnabled);
    m_notificationDurationSpin->setValue(config.notificationDuration);
    
    // 设置语言
    int langIndex = m_languageCombo->findData(config.language);
    if (langIndex >= 0) {
        m_languageCombo->setCurrentIndex(langIndex);
    }
}

void SettingsDialog::loadReminderSettings()
{
    if (!m_configManager) return;
    
    onReminderTypeChanged(); // 加载当前选中类型的配置
}

void SettingsDialog::loadScheduleSettings()
{
    if (!m_configManager) return;
    
    ConfigManager::WorkSchedule schedule = m_configManager->getWorkSchedule();
    
    m_respectScheduleCheck->setChecked(schedule.respectSchedule);
    m_workStartTimeEdit->setTime(schedule.workStartTime);
    m_workEndTimeEdit->setTime(schedule.workEndTime);
    
    // 设置工作日
    for (int i = 0; i < m_workDaysList->count(); ++i) {
        QListWidgetItem* item = m_workDaysList->item(i);
        int dayValue = item->data(Qt::UserRole).toInt();
        item->setCheckState(schedule.workDays.contains(dayValue) ? Qt::Checked : Qt::Unchecked);
    }
}

void SettingsDialog::loadAdvancedSettings()
{
    if (!m_configManager) return;
    
    ConfigManager::AdvancedConfig config = m_configManager->getAdvancedConfig();
    
    m_collectStatsCheck->setChecked(config.collectAnonymousStats);
    m_enableLoggingCheck->setChecked(config.enableLogging);
    m_smartAdaptationCheck->setChecked(config.enableSmartAdaptation);
    m_dataRetentionSpin->setValue(config.dataRetentionDays);
    
    int logIndex = m_logLevelCombo->findText(config.logLevel);
    if (logIndex >= 0) {
        m_logLevelCombo->setCurrentIndex(logIndex);
    }
}

void SettingsDialog::saveGeneralSettings()
{
    if (!m_configManager) return;
    
    ConfigManager::GeneralConfig config;
    config.autoStart = m_autoStartCheck->isChecked();
    config.minimizeToTray = m_minimizeToTrayCheck->isChecked();
    config.showNotifications = m_showNotificationsCheck->isChecked();
    config.soundEnabled = m_soundEnabledCheck->isChecked();
    config.notificationDuration = m_notificationDurationSpin->value();
    config.language = m_languageCombo->currentData().toString();
    
    m_configManager->setGeneralConfig(config);
}

void SettingsDialog::saveReminderSettings()
{
    if (!m_configManager) return;
    
    HealthEngine::ReminderType currentType = static_cast<HealthEngine::ReminderType>(
        m_reminderTypeCombo->currentData().toInt());
    
    HealthEngine::ReminderConfig config;
    config.enabled = m_reminderEnabledCheck->isChecked();
    config.intervalMinutes = m_reminderIntervalSpin->value();
    config.durationSeconds = m_reminderDurationSpin->value();
    config.message = m_reminderMessageEdit->text();
    config.suggestion = m_reminderSuggestionEdit->toPlainText();
    
    m_configManager->setReminderConfig(currentType, config);
}

void SettingsDialog::saveScheduleSettings()
{
    if (!m_configManager) return;
    
    ConfigManager::WorkSchedule schedule;
    schedule.respectSchedule = m_respectScheduleCheck->isChecked();
    schedule.workStartTime = m_workStartTimeEdit->time();
    schedule.workEndTime = m_workEndTimeEdit->time();
    
    schedule.workDays.clear();
    for (int i = 0; i < m_workDaysList->count(); ++i) {
        QListWidgetItem* item = m_workDaysList->item(i);
        if (item->checkState() == Qt::Checked) {
            schedule.workDays.append(item->data(Qt::UserRole).toInt());
        }
    }
    
    m_configManager->setWorkSchedule(schedule);
}

void SettingsDialog::saveAdvancedSettings()
{
    if (!m_configManager) return;
    
    ConfigManager::AdvancedConfig config;
    config.collectAnonymousStats = m_collectStatsCheck->isChecked();
    config.enableLogging = m_enableLoggingCheck->isChecked();
    config.enableSmartAdaptation = m_smartAdaptationCheck->isChecked();
    config.dataRetentionDays = m_dataRetentionSpin->value();
    config.logLevel = m_logLevelCombo->currentText();
    
    m_configManager->setAdvancedConfig(config);
}

void SettingsDialog::onApplySettings()
{
    saveGeneralSettings();
    saveReminderSettings();
    saveScheduleSettings();
    saveAdvancedSettings();
    
    Logger::info("设置已保存", "SettingsDialog");
    
    QMessageBox::information(this, "设置", "设置已保存并应用!");
}

void SettingsDialog::onResetToDefaults()
{
    int result = QMessageBox::question(this, "重置设置", 
                                      "确定要重置所有设置为默认值吗？此操作不可撤销。",
                                      QMessageBox::Yes | QMessageBox::No);
    
    if (result == QMessageBox::Yes && m_configManager) {
        m_configManager->resetToDefaults();
        refreshSettings();
        
        QMessageBox::information(this, "重置设置", "所有设置已重置为默认值!");
    }
}

void SettingsDialog::onExportConfig()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
                                                   "导出配置文件", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/wellness_config.json",
                                                   "JSON文件 (*.json)");
    
    if (!fileName.isEmpty() && m_configManager) {
        if (m_configManager->exportConfig(fileName)) {
            QMessageBox::information(this, "导出成功", "配置文件已成功导出!");
        } else {
            QMessageBox::warning(this, "导出失败", "配置文件导出失败!");
        }
    }
}

void SettingsDialog::onImportConfig()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
                                                   "导入配置文件", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                   "JSON文件 (*.json)");
    
    if (!fileName.isEmpty() && m_configManager) {
        if (m_configManager->importConfig(fileName)) {
            refreshSettings();
            QMessageBox::information(this, "导入成功", "配置文件已成功导入!");
        } else {
            QMessageBox::warning(this, "导入失败", "配置文件导入失败!");
        }
    }
}

void SettingsDialog::onTestNotification()
{
    // 这里可以触发一个测试通知
    QMessageBox::information(this, "测试通知", "这是一个测试通知!");
}

void SettingsDialog::onReminderTypeChanged()
{
    if (!m_configManager) return;
    
    HealthEngine::ReminderType currentType = static_cast<HealthEngine::ReminderType>(
        m_reminderTypeCombo->currentData().toInt());
    
    HealthEngine::ReminderConfig config = m_configManager->getReminderConfig(currentType);
    
    m_reminderEnabledCheck->setChecked(config.enabled);
    m_reminderIntervalSpin->setValue(config.intervalMinutes);
    m_reminderDurationSpin->setValue(config.durationSeconds);
    m_reminderMessageEdit->setText(config.message);
    m_reminderSuggestionEdit->setPlainText(config.suggestion);
}

void SettingsDialog::onPreviewSound()
{
    // 播放预览音效
    Logger::info("播放预览音效", "SettingsDialog");
}