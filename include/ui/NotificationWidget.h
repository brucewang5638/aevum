#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>

#include "core/HealthEngine.h"

/**
 * @brief 自定义通知弹窗
 * 
 * 显示健康提醒的自定义弹窗，支持动画效果和交互操作
 */
class NotificationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationWidget(QWidget *parent = nullptr);
    ~NotificationWidget();

    /**
     * @brief 显示健康提醒通知
     */
    void showReminder(HealthEngine::ReminderType type,
                     const QString& message,
                     const QString& suggestion);

    /**
     * @brief 设置通知持续时间
     */
    void setDuration(int seconds);

    /**
     * @brief 设置通知位置
     */
    void setPosition(Qt::Corner corner = Qt::BottomRightCorner);

signals:
    /**
     * @brief 用户点击"立即休息"时发出
     */
    void takeBreakClicked(HealthEngine::ReminderType type);

    /**
     * @brief 用户点击"稍后提醒"时发出
     */
    void snoozeClicked(int minutes);

    /**
     * @brief 通知被关闭时发出
     */
    void notificationClosed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onTakeBreakClicked();
    void onSnoozeClicked();
    void onCloseClicked();
    void onAutoClose();
    void onCountdownUpdate();

private:
    void setupUI();
    void setupAnimations();
    void updateReminderContent(HealthEngine::ReminderType type,
                              const QString& message,
                              const QString& suggestion);
    void positionWindow();
    void startShowAnimation();
    void startHideAnimation();
    QString getReminderIcon(HealthEngine::ReminderType type);
    QString getReminderColor(HealthEngine::ReminderType type);

    // UI 组件
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QHBoxLayout* m_buttonLayout;
    
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QLabel* m_suggestionLabel;
    
    QPushButton* m_takeBreakBtn;
    QPushButton* m_snoozeBtn;
    QPushButton* m_closeBtn;
    
    QProgressBar* m_countdownBar;
    QLabel* m_countdownLabel;

    // 动画
    QPropertyAnimation* m_showAnimation;
    QPropertyAnimation* m_hideAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;

    // 计时器
    QTimer* m_autoCloseTimer;
    QTimer* m_countdownTimer;

    // 状态
    HealthEngine::ReminderType m_currentType;
    int m_duration;
    int m_remainingTime;
    Qt::Corner m_position;
    bool m_isShowing;
    bool m_mouseOver;
};