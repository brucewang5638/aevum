#include "core/ActivityMonitor.h"
#include "utils/Logger.h"
#include "utils/SystemUtils.h"
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winuser.h>
#elif defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/Xutil.h>
#elif defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

class ActivityMonitor::Private
{
public:
#ifdef Q_OS_WIN
    // Windows 平台相关数据
    LASTINPUTINFO lastInputInfo;
#elif defined(Q_OS_LINUX)
    // Linux 平台相关数据
    Display* display;
    XScreenSaverInfo* screenSaverInfo;
#endif
    
    int lastMouseClicks = 0;
    int lastKeystrokes = 0;
};

ActivityMonitor::ActivityMonitor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_lastActivityTime(QDateTime::currentDateTime())
    , m_sessionStartTime(QDateTime::currentDateTime())
    , m_isActive(false)
    , m_todayActiveMinutes(0)
    , d(std::make_unique<Private>())
{
    m_timer->setInterval(1000); // 每秒检查一次
    connect(m_timer, &QTimer::timeout, this, &ActivityMonitor::checkActivity);
    
    initializeSystemHooks();
}

ActivityMonitor::~ActivityMonitor()
{
    cleanupSystemHooks();
}

void ActivityMonitor::start()
{
    Logger::info("开始监测用户活动");
    m_sessionStartTime = QDateTime::currentDateTime();
    m_timer->start();
}

void ActivityMonitor::stop()
{
    Logger::info("停止监测用户活动");
    m_timer->stop();
}

bool ActivityMonitor::isUserActive() const
{
    return m_isActive;
}

QDateTime ActivityMonitor::getLastActivityTime() const
{
    return m_lastActivityTime;
}

int ActivityMonitor::getTodayActiveMinutes() const
{
    return m_todayActiveMinutes;
}

void ActivityMonitor::checkActivity()
{
    ActivityData data;
    data.timestamp = QDateTime::currentDateTime();
    data.mouseClicks = getCurrentMouseClicks();
    data.keystrokes = getCurrentKeystrokes();
    data.activeWindow = getCurrentActiveWindow();
    
    // 检查是否有新的活动
    bool hasNewActivity = (data.mouseClicks > d->lastMouseClicks) || 
                         (data.keystrokes > d->lastKeystrokes);
    
    if (hasNewActivity) {
        m_lastActivityTime = data.timestamp;
        if (!m_isActive) {
            m_isActive = true;
            emit userBecameActive();
        }
        data.isActive = true;
        emit activityDetected(data);
    } else {
        // 检查是否超过一定时间无活动（例如30秒）
        if (m_isActive && m_lastActivityTime.secsTo(data.timestamp) > 30) {
            m_isActive = false;
            emit userBecameInactive();
        }
        data.isActive = m_isActive;
    }
    
    d->lastMouseClicks = data.mouseClicks;
    d->lastKeystrokes = data.keystrokes;
    
    // 更新今日活跃时间
    if (data.isActive) {
        m_todayActiveMinutes++;
    }
}

void ActivityMonitor::initializeSystemHooks()
{
#ifdef Q_OS_WIN
    d->lastInputInfo.cbSize = sizeof(LASTINPUTINFO);
#elif defined(Q_OS_LINUX)
    d->display = XOpenDisplay(nullptr);
    if (d->display) {
        d->screenSaverInfo = XScreenSaverAllocInfo();
    }
#endif
}

void ActivityMonitor::cleanupSystemHooks()
{
#ifdef Q_OS_LINUX
    if (d->display) {
        if (d->screenSaverInfo) {
            XFree(d->screenSaverInfo);
        }
        XCloseDisplay(d->display);
    }
#endif
}

int ActivityMonitor::getCurrentMouseClicks()
{
#ifdef Q_OS_WIN
    // Windows: 获取鼠标点击计数
    static DWORD lastMouseClickTime = 0;
    static int clickCount = 0;
    
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    DWORD currentTime = GetTickCount();
    if (currentTime - lastMouseClickTime > 100) { // 100ms间隔检测
        // 检查鼠标按键状态
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000 || 
            GetAsyncKeyState(VK_RBUTTON) & 0x8000 ||
            GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
            clickCount++;
            lastMouseClickTime = currentTime;
        }
    }
    return clickCount;
#elif defined(Q_OS_LINUX)
    // Linux: 使用X11获取鼠标状态
    if (!d->display) return d->lastMouseClicks;
    
    Window root, child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    
    if (XQueryPointer(d->display, DefaultRootWindow(d->display),
                     &root, &child, &root_x, &root_y, &win_x, &win_y, &mask)) {
        static int lastClickCount = 0;
        if (mask & (Button1Mask | Button2Mask | Button3Mask)) {
            lastClickCount++;
        }
        return lastClickCount;
    }
    return d->lastMouseClicks;
#else
    // 其他平台或模拟数据
    static int counter = 0;
    return ++counter;
#endif
}

int ActivityMonitor::getCurrentKeystrokes()
{
#ifdef Q_OS_WIN
    // Windows: 检测键盘活动
    static int keystrokeCount = 0;
    static DWORD lastKeystrokeTime = 0;
    
    DWORD currentTime = GetTickCount();
    if (currentTime - lastKeystrokeTime > 50) { // 50ms间隔检测
        // 检查常用按键状态
        for (int key = 8; key <= 255; key++) {
            if (GetAsyncKeyState(key) & 0x8000) {
                keystrokeCount++;
                lastKeystrokeTime = currentTime;
                break;
            }
        }
    }
    return keystrokeCount;
#elif defined(Q_OS_LINUX)
    // Linux: 使用X11检测键盘活动
    if (!d->display) return d->lastKeystrokes;
    
    char keys[32];
    XQueryKeymap(d->display, keys);
    
    static int lastKeystrokeCount = 0;
    int activeKeys = 0;
    for (int i = 0; i < 32; i++) {
        if (keys[i] != 0) {
            activeKeys++;
        }
    }
    
    if (activeKeys > 0) {
        lastKeystrokeCount++;
    }
    return lastKeystrokeCount;
#else
    // 其他平台或模拟数据
    static int counter = 0;
    return ++counter;
#endif
}

QString ActivityMonitor::getCurrentActiveWindow()
{
    return SystemUtils::getActiveWindowTitle();
}