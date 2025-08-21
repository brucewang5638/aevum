#include "ui/NotificationWidget.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QGraphicsDropShadowEffect>

NotificationWidget::NotificationWidget(QWidget *parent)
    : QWidget(parent)
    , m_duration(5)
    , m_remainingTime(5)
    , m_position(Qt::BottomRightCorner)
    , m_isShowing(false)
    , m_mouseOver(false)
{
    setupUI();
    setupAnimations();
    
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    
    // 设置固定大小
    setFixedSize(350, 120);
    
    // 初始化计时器
    m_autoCloseTimer = new QTimer(this);
    m_countdownTimer = new QTimer(this);
    
    m_autoCloseTimer->setSingleShot(true);
    m_countdownTimer->setInterval(1000); // 每秒更新一次
    
    connect(m_autoCloseTimer, &QTimer::timeout, this, &NotificationWidget::onAutoClose);
    connect(m_countdownTimer, &QTimer::timeout, this, &NotificationWidget::onCountdownUpdate);
}

NotificationWidget::~NotificationWidget()
{
}

void NotificationWidget::showReminder(HealthEngine::ReminderType type,
                                     const QString& message,
                                     const QString& suggestion)
{
    if (m_isShowing) {
        return; // 避免重复显示
    }
    
    m_currentType = type;
    updateReminderContent(type, message, suggestion);
    positionWindow();
    
    m_remainingTime = m_duration;
    m_countdownBar->setMaximum(m_duration);
    m_countdownBar->setValue(m_duration);
    
    // 开始显示动画
    m_isShowing = true;
    show();
    startShowAnimation();
    
    // 启动倒计时
    m_autoCloseTimer->start(m_duration * 1000);
    m_countdownTimer->start();
    
    Logger::info(QString("显示健康提醒通知: %1").arg(message), "NotificationWidget");
}

void NotificationWidget::setDuration(int seconds)
{
    m_duration = seconds;
}

void NotificationWidget::setPosition(Qt::Corner corner)
{
    m_position = corner;
}

void NotificationWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(5, 5, -5, -5), 10, 10);
    
    // 背景颜色根据提醒类型变化
    QString color = getReminderColor(m_currentType);
    QColor backgroundColor = QColor(color);
    backgroundColor.setAlpha(240);
    
    painter.fillPath(path, backgroundColor);
    
    // 绘制边框
    QPen pen(QColor("#ffffff"), 2);
    painter.setPen(pen);
    painter.drawPath(path);
}

void NotificationWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 点击通知区域暂停自动关闭
        if (m_autoCloseTimer->isActive()) {
            m_autoCloseTimer->stop();
            m_countdownTimer->stop();
        }
    }
    QWidget::mousePressEvent(event);
}

void NotificationWidget::enterEvent(QEnterEvent* event)
{
    Q_UNUSED(event)
    m_mouseOver = true;
    
    // 鼠标悬停时暂停倒计时
    if (m_autoCloseTimer->isActive()) {
        m_autoCloseTimer->stop();
        m_countdownTimer->stop();
    }
}

void NotificationWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    m_mouseOver = false;
    
    // 鼠标离开时恢复倒计时
    if (m_remainingTime > 0 && !m_autoCloseTimer->isActive()) {
        m_autoCloseTimer->start(m_remainingTime * 1000);
        m_countdownTimer->start();
    }
}

void NotificationWidget::onTakeBreakClicked()
{
    emit takeBreakClicked(m_currentType);
    onCloseClicked();
}

void NotificationWidget::onSnoozeClicked()
{
    emit snoozeClicked(10); // 暂停10分钟
    onCloseClicked();
}

void NotificationWidget::onCloseClicked()
{
    m_autoCloseTimer->stop();
    m_countdownTimer->stop();
    
    startHideAnimation();
    emit notificationClosed();
}

void NotificationWidget::onAutoClose()
{
    m_countdownTimer->stop();
    startHideAnimation();
    emit notificationClosed();
}

void NotificationWidget::onCountdownUpdate()
{
    m_remainingTime--;
    m_countdownBar->setValue(m_remainingTime);
    m_countdownLabel->setText(QString("%1秒").arg(m_remainingTime));
    
    if (m_remainingTime <= 0) {
        m_countdownTimer->stop();
    }
}

void NotificationWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    m_mainLayout->setSpacing(8);
    
    // 头部布局（图标和标题）
    m_headerLayout = new QHBoxLayout();
    
    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(32, 32);
    m_iconLabel->setScaledContents(true);
    
    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #333;");
    
    m_closeBtn = new QPushButton("×");
    m_closeBtn->setFixedSize(20, 20);
    m_closeBtn->setStyleSheet("QPushButton { border: none; background: transparent; font-size: 16px; font-weight: bold; color: #666; }"
                             "QPushButton:hover { color: #000; }");
    connect(m_closeBtn, &QPushButton::clicked, this, &NotificationWidget::onCloseClicked);
    
    m_headerLayout->addWidget(m_iconLabel);
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_closeBtn);
    
    // 消息标签
    m_messageLabel = new QLabel();
    m_messageLabel->setStyleSheet("font-size: 12px; color: #555;");
    m_messageLabel->setWordWrap(true);
    
    // 建议标签
    m_suggestionLabel = new QLabel();
    m_suggestionLabel->setStyleSheet("font-size: 11px; color: #777; font-style: italic;");
    m_suggestionLabel->setWordWrap(true);
    
    // 按钮布局
    m_buttonLayout = new QHBoxLayout();
    
    m_takeBreakBtn = new QPushButton("立即休息");
    m_takeBreakBtn->setStyleSheet("QPushButton { background: #4CAF50; color: white; border: none; padding: 6px 12px; border-radius: 4px; }"
                                 "QPushButton:hover { background: #45a049; }");
    connect(m_takeBreakBtn, &QPushButton::clicked, this, &NotificationWidget::onTakeBreakClicked);
    
    m_snoozeBtn = new QPushButton("稍后提醒");
    m_snoozeBtn->setStyleSheet("QPushButton { background: #ff9800; color: white; border: none; padding: 6px 12px; border-radius: 4px; }"
                              "QPushButton:hover { background: #f57c00; }");
    connect(m_snoozeBtn, &QPushButton::clicked, this, &NotificationWidget::onSnoozeClicked);
    
    // 倒计时进度条和标签
    m_countdownBar = new QProgressBar();
    m_countdownBar->setFixedHeight(4);
    m_countdownBar->setTextVisible(false);
    m_countdownBar->setStyleSheet("QProgressBar { border: none; background: #e0e0e0; border-radius: 2px; }"
                                 "QProgressBar::chunk { background: #2196F3; border-radius: 2px; }");
    
    m_countdownLabel = new QLabel();
    m_countdownLabel->setStyleSheet("font-size: 10px; color: #999;");
    m_countdownLabel->setAlignment(Qt::AlignRight);
    
    m_buttonLayout->addWidget(m_takeBreakBtn);
    m_buttonLayout->addWidget(m_snoozeBtn);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_countdownLabel);
    
    // 添加到主布局
    m_mainLayout->addLayout(m_headerLayout);
    m_mainLayout->addWidget(m_messageLabel);
    m_mainLayout->addWidget(m_suggestionLabel);
    m_mainLayout->addLayout(m_buttonLayout);
    m_mainLayout->addWidget(m_countdownBar);
    
    // 添加阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 5);
    setGraphicsEffect(shadow);
}

void NotificationWidget::setupAnimations()
{
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    
    m_showAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_showAnimation->setDuration(300);
    m_showAnimation->setStartValue(0.0);
    m_showAnimation->setEndValue(1.0);
    
    m_hideAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_hideAnimation->setDuration(200);
    m_hideAnimation->setStartValue(1.0);
    m_hideAnimation->setEndValue(0.0);
    
    connect(m_hideAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        m_isShowing = false;
    });
}

void NotificationWidget::updateReminderContent(HealthEngine::ReminderType type,
                                              const QString& message,
                                              const QString& suggestion)
{
    QString icon = getReminderIcon(type);
    m_iconLabel->setText(icon);
    m_iconLabel->setStyleSheet(QString("font-size: 24px; color: %1;").arg(getReminderColor(type)));
    
    QString title;
    switch (type) {
    case HealthEngine::ReminderType::SittingTooLong:
        title = "久坐提醒";
        break;
    case HealthEngine::ReminderType::EyeRest:
        title = "眼部休息";
        break;
    case HealthEngine::ReminderType::NeckExercise:
        title = "颈椎运动";
        break;
    case HealthEngine::ReminderType::PostureCheck:
        title = "姿势检查";
        break;
    case HealthEngine::ReminderType::Hydration:
        title = "补充水分";
        break;
    }
    
    m_titleLabel->setText(title);
    m_messageLabel->setText(message);
    m_suggestionLabel->setText(suggestion);
}

void NotificationWidget::positionWindow()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    
    int x, y;
    int margin = 20;
    
    switch (m_position) {
    case Qt::TopLeftCorner:
        x = screenGeometry.left() + margin;
        y = screenGeometry.top() + margin;
        break;
    case Qt::TopRightCorner:
        x = screenGeometry.right() - width() - margin;
        y = screenGeometry.top() + margin;
        break;
    case Qt::BottomLeftCorner:
        x = screenGeometry.left() + margin;
        y = screenGeometry.bottom() - height() - margin;
        break;
    case Qt::BottomRightCorner:
    default:
        x = screenGeometry.right() - width() - margin;
        y = screenGeometry.bottom() - height() - margin;
        break;
    }
    
    move(x, y);
}

void NotificationWidget::startShowAnimation()
{
    m_showAnimation->start();
}

void NotificationWidget::startHideAnimation()
{
    m_hideAnimation->start();
}

QString NotificationWidget::getReminderIcon(HealthEngine::ReminderType type)
{
    switch (type) {
    case HealthEngine::ReminderType::SittingTooLong:
        return "🪑";
    case HealthEngine::ReminderType::EyeRest:
        return "👁️";
    case HealthEngine::ReminderType::NeckExercise:
        return "🦴";
    case HealthEngine::ReminderType::PostureCheck:
        return "🧘";
    case HealthEngine::ReminderType::Hydration:
        return "💧";
    default:
        return "💡";
    }
}

QString NotificationWidget::getReminderColor(HealthEngine::ReminderType type)
{
    switch (type) {
    case HealthEngine::ReminderType::SittingTooLong:
        return "#FF9800"; // Orange
    case HealthEngine::ReminderType::EyeRest:
        return "#2196F3"; // Blue
    case HealthEngine::ReminderType::NeckExercise:
        return "#4CAF50"; // Green
    case HealthEngine::ReminderType::PostureCheck:
        return "#9C27B0"; // Purple
    case HealthEngine::ReminderType::Hydration:
        return "#00BCD4"; // Cyan
    default:
        return "#607D8B"; // Blue Grey
    }
}