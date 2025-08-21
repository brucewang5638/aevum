#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>

#include "core/ConfigManager.h"
#include "core/HealthEngine.h"

/**
 * @brief 设置对话框
 * 
 * 提供用户配置界面，包括基本设置、提醒配置、工作时间等
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    /**
     * @brief 设置配置管理器
     */
    void setConfigManager(ConfigManager* configManager);

    /**
     * @brief 刷新界面显示
     */
    void refreshSettings();

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onApplySettings();
    void onResetToDefaults();
    void onExportConfig();
    void onImportConfig();
    void onTestNotification();
    void onReminderTypeChanged();
    void onPreviewSound();

private:
    void setupUI();
    void createGeneralTab();
    void createRemindersTab();
    void createScheduleTab();
    void createAdvancedTab();
    void createAboutTab();
    
    void loadGeneralSettings();
    void loadReminderSettings();
    void loadScheduleSettings();
    void loadAdvancedSettings();
    
    void saveGeneralSettings();
    void saveReminderSettings();
    void saveScheduleSettings();
    void saveAdvancedSettings();

    // UI 组件
    QTabWidget* m_tabWidget;
    
    // 基本设置页
    QWidget* m_generalTab;
    QCheckBox* m_autoStartCheck;
    QCheckBox* m_minimizeToTrayCheck;
    QComboBox* m_languageCombo;
    QCheckBox* m_soundEnabledCheck;
    QCheckBox* m_showNotificationsCheck;
    QSpinBox* m_notificationDurationSpin;
    
    // 提醒设置页
    QWidget* m_remindersTab;
    QComboBox* m_reminderTypeCombo;
    QCheckBox* m_reminderEnabledCheck;
    QSpinBox* m_reminderIntervalSpin;
    QSpinBox* m_reminderDurationSpin;
    QLineEdit* m_reminderMessageEdit;
    QTextEdit* m_reminderSuggestionEdit;
    QPushButton* m_testNotificationBtn;
    
    // 工作时间页
    QWidget* m_scheduleTab;
    QTimeEdit* m_workStartTimeEdit;
    QTimeEdit* m_workEndTimeEdit;
    QListWidget* m_workDaysList;
    QCheckBox* m_respectScheduleCheck;
    
    // 高级设置页
    QWidget* m_advancedTab;
    QCheckBox* m_collectStatsCheck;
    QCheckBox* m_enableLoggingCheck;
    QComboBox* m_logLevelCombo;
    QSpinBox* m_dataRetentionSpin;
    QCheckBox* m_smartAdaptationCheck;
    QPushButton* m_exportConfigBtn;
    QPushButton* m_importConfigBtn;
    
    // 关于页
    QWidget* m_aboutTab;
    QLabel* m_versionLabel;
    QLabel* m_copyrightLabel;
    
    // 按钮
    QPushButton* m_applyBtn;
    QPushButton* m_resetBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_okBtn;

    ConfigManager* m_configManager;
    QMap<HealthEngine::ReminderType, HealthEngine::ReminderConfig> m_reminderConfigs;
};