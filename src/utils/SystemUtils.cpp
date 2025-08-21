#include "utils/SystemUtils.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QNetworkInterface>
#include <QSysInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winuser.h>
#include <psapi.h>
#include <pdh.h>
#include <shellapi.h>
#elif defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <fstream>
#elif defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <sys/sysctl.h>
#endif

QString SystemUtils::getActiveWindowTitle()
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        WCHAR windowTitle[256];
        GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR));
        return QString::fromWCharArray(windowTitle);
    }
    return QString();
#elif defined(Q_OS_LINUX)
    return getLinuxActiveWindow();
#elif defined(Q_OS_MACOS)
    return getMacActiveWindow();
#else
    return "Unknown Window";
#endif
}

QString SystemUtils::getActiveApplicationName()
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (hProcess) {
            WCHAR processName[MAX_PATH];
            if (GetModuleBaseNameW(hProcess, NULL, processName, MAX_PATH)) {
                CloseHandle(hProcess);
                return QString::fromWCharArray(processName);
            }
            CloseHandle(hProcess);
        }
    }
    return QString();
#elif defined(Q_OS_LINUX)
    // Linux 实现
    Display* display = XOpenDisplay(nullptr);
    if (!display) return QString();
    
    Window focused;
    int revert;
    XGetInputFocus(display, &focused, &revert);
    
    if (focused == PointerRoot || focused == None) {
        XCloseDisplay(display);
        return QString();
    }
    
    char* windowName = nullptr;
    if (XFetchName(display, focused, &windowName) && windowName) {
        QString result = QString::fromLocal8Bit(windowName);
        XFree(windowName);
        XCloseDisplay(display);
        return result;
    }
    
    XCloseDisplay(display);
    return QString();
#else
    return "Unknown Application";
#endif
}

int SystemUtils::getSystemIdleTime()
{
#ifdef Q_OS_WIN
    return getWindowsIdleTime();
#elif defined(Q_OS_LINUX)
    return getLinuxIdleTime();
#elif defined(Q_OS_MACOS)
    return getMacIdleTime();
#else
    return 0;
#endif
}

QRect SystemUtils::getScreenGeometry()
{
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen) {
        return primaryScreen->geometry();
    }
    return QRect();
}

QPoint SystemUtils::getMousePosition()
{
#ifdef Q_OS_WIN
    POINT point;
    GetCursorPos(&point);
    return QPoint(point.x, point.y);
#elif defined(Q_OS_LINUX)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return QPoint();
    
    Window root, child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    
    if (XQueryPointer(display, DefaultRootWindow(display),
                     &root, &child, &root_x, &root_y, &win_x, &win_y, &mask)) {
        XCloseDisplay(display);
        return QPoint(root_x, root_y);
    }
    
    XCloseDisplay(display);
    return QPoint();
#else
    return QPoint();
#endif
}

bool SystemUtils::isFullscreenApplication()
{
    QRect screenGeometry = getScreenGeometry();
    // 简单检测：如果窗口标题包含全屏相关关键词
    QString activeWindow = getActiveWindowTitle().toLower();
    
    return activeWindow.contains("fullscreen") || 
           activeWindow.contains("full screen") ||
           activeWindow.contains("游戏") ||
           activeWindow.contains("视频") ||
           activeWindow.contains("movie") ||
           activeWindow.contains("video");
}

bool SystemUtils::setAutoStart(bool enabled, const QString& appName, const QString& appPath)
{
#ifdef Q_OS_WIN
    QString keyPath = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    QSettings settings(keyPath, QSettings::NativeFormat);
    
    if (enabled) {
        settings.setValue(appName, QDir::toNativeSeparators(appPath));
    } else {
        settings.remove(appName);
    }
    
    return true;
#elif defined(Q_OS_LINUX)
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(autostartDir);
    
    QString desktopFile = autostartDir + "/" + appName + ".desktop";
    
    if (enabled) {
        QFile file(desktopFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=" << appName << "\n";
            out << "Exec=" << appPath << "\n";
            out << "Hidden=false\n";
            out << "NoDisplay=false\n";
            out << "X-GNOME-Autostart-enabled=true\n";
            return true;
        }
    } else {
        return QFile::remove(desktopFile);
    }
    
    return false;
#else
    // macOS 和其他平台的实现
    Q_UNUSED(enabled)
    Q_UNUSED(appName)
    Q_UNUSED(appPath)
    return false;
#endif
}

bool SystemUtils::isAutoStartEnabled(const QString& appName)
{
#ifdef Q_OS_WIN
    QString keyPath = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    QSettings settings(keyPath, QSettings::NativeFormat);
    return settings.contains(appName);
#elif defined(Q_OS_LINUX)
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QString desktopFile = autostartDir + "/" + appName + ".desktop";
    return QFile::exists(desktopFile);
#else
    Q_UNUSED(appName)
    return false;
#endif
}

QString SystemUtils::getSystemVersion()
{
    return QSysInfo::prettyProductName();
}

double SystemUtils::getCpuUsage()
{
#ifdef Q_OS_WIN
    // Windows CPU 使用率获取
    static PDH_HQUERY query;
    static PDH_HCOUNTER counter;
    static bool initialized = false;
    
    if (!initialized) {
        PdhOpenQuery(NULL, 0, &query);
        PdhAddCounter(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter);
        PdhCollectQueryData(query);
        initialized = true;
        return 0.0; // 第一次调用返回0
    }
    
    PDH_FMT_COUNTERVALUE value;
    PdhCollectQueryData(query);
    PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &value);
    
    return value.doubleValue;
#elif defined(Q_OS_LINUX)
    // Linux CPU 使用率获取
    static unsigned long long lastTotal = 0, lastIdle = 0;
    
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
    
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    
    unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long totalDiff = total - lastTotal;
    unsigned long long idleDiff = idle - lastIdle;
    
    double cpuUsage = 0.0;
    if (totalDiff > 0) {
        cpuUsage = 100.0 * (1.0 - (double)idleDiff / totalDiff);
    }
    
    lastTotal = total;
    lastIdle = idle;
    
    return cpuUsage;
#else
    return 0.0;
#endif
}

double SystemUtils::getMemoryUsage()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    return (double)memInfo.dwMemoryLoad;
#elif defined(Q_OS_LINUX)
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    
    unsigned long totalPhysMem = memInfo.totalram * memInfo.mem_unit;
    unsigned long physMemUsed = (memInfo.totalram - memInfo.freeram) * memInfo.mem_unit;
    
    return 100.0 * physMemUsed / totalPhysMem;
#else
    return 0.0;
#endif
}

void SystemUtils::playNotificationSound()
{
#ifdef Q_OS_WIN
    PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
#elif defined(Q_OS_LINUX)
    QProcess::startDetached("paplay", QStringList() << "/usr/share/sounds/alsa/Front_Left.wav");
#else
    // 其他平台或静默
#endif
}

QString SystemUtils::getUserDocumentsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString SystemUtils::getAppDataPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

bool SystemUtils::isNetworkAvailable()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            return true;
        }
    }
    return false;
}

QString SystemUtils::getLocalIPAddress()
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }
    return "127.0.0.1";
}

void SystemUtils::showNativeNotification(const QString& title, 
                                        const QString& message,
                                        int duration)
{
#ifdef Q_OS_WIN
    // Windows 原生通知
    // 这里可以使用 Windows Toast 通知API
    Q_UNUSED(title)
    Q_UNUSED(message)
    Q_UNUSED(duration)
#elif defined(Q_OS_LINUX)
    // Linux 使用 notify-send
    QStringList args;
    args << "-t" << QString::number(duration) << title << message;
    QProcess::startDetached("notify-send", args);
#else
    Q_UNUSED(title)
    Q_UNUSED(message)
    Q_UNUSED(duration)
#endif
}

// 平台特定实现
#ifdef Q_OS_WIN
QString SystemUtils::getWindowsActiveWindow()
{
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        WCHAR windowTitle[256];
        GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR));
        return QString::fromWCharArray(windowTitle);
    }
    return QString();
}

int SystemUtils::getWindowsIdleTime()
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);
    return GetTickCount() - lii.dwTime;
}
#endif

#ifdef Q_OS_LINUX
QString SystemUtils::getLinuxActiveWindow()
{
    Display* display = XOpenDisplay(nullptr);
    if (!display) return QString();
    
    Window focused;
    int revert;
    XGetInputFocus(display, &focused, &revert);
    
    if (focused == PointerRoot || focused == None) {
        XCloseDisplay(display);
        return QString();
    }
    
    char* windowName = nullptr;
    if (XFetchName(display, focused, &windowName) && windowName) {
        QString result = QString::fromLocal8Bit(windowName);
        XFree(windowName);
        XCloseDisplay(display);
        return result;
    }
    
    XCloseDisplay(display);
    return QString();
}

int SystemUtils::getLinuxIdleTime()
{
    Display* display = XOpenDisplay(nullptr);
    if (!display) return 0;
    
    XScreenSaverInfo* info = XScreenSaverAllocInfo();
    if (!info) {
        XCloseDisplay(display);
        return 0;
    }
    
    XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
    int idleTime = info->idle;
    
    XFree(info);
    XCloseDisplay(display);
    
    return idleTime;
}
#endif

#ifdef Q_OS_MACOS
QString SystemUtils::getMacActiveWindow()
{
    // macOS 实现
    return QString();
}

int SystemUtils::getMacIdleTime()
{
    // macOS 实现
    return 0;
}
#endif