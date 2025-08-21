#pragma once

#include <QString>
#include <QDateTime>
#include <QRect>

/**
 * @brief 系统工具类
 * 
 * 提供跨平台的系统相关功能，如窗口检测、系统信息获取等
 */
class SystemUtils
{
public:
    /**
     * @brief 获取当前活跃窗口标题
     */
    static QString getActiveWindowTitle();

    /**
     * @brief 获取当前活跃应用程序名称
     */
    static QString getActiveApplicationName();

    /**
     * @brief 获取系统空闲时间（毫秒）
     */
    static int getSystemIdleTime();

    /**
     * @brief 获取屏幕分辨率
     */
    static QRect getScreenGeometry();

    /**
     * @brief 获取鼠标位置
     */
    static QPoint getMousePosition();

    /**
     * @brief 检查是否为全屏应用
     */
    static bool isFullscreenApplication();

    /**
     * @brief 设置开机自启动
     */
    static bool setAutoStart(bool enabled, const QString& appName, const QString& appPath);

    /**
     * @brief 检查是否已设置开机自启动
     */
    static bool isAutoStartEnabled(const QString& appName);

    /**
     * @brief 获取系统版本信息
     */
    static QString getSystemVersion();

    /**
     * @brief 获取CPU使用率
     */
    static double getCpuUsage();

    /**
     * @brief 获取内存使用率
     */
    static double getMemoryUsage();

    /**
     * @brief 播放系统提示音
     */
    static void playNotificationSound();

    /**
     * @brief 获取用户文档目录
     */
    static QString getUserDocumentsPath();

    /**
     * @brief 获取应用数据目录
     */
    static QString getAppDataPath();

    /**
     * @brief 检查网络连接状态
     */
    static bool isNetworkAvailable();

    /**
     * @brief 获取本地IP地址
     */
    static QString getLocalIPAddress();

    /**
     * @brief 显示桌面通知（系统原生）
     */
    static void showNativeNotification(const QString& title, 
                                      const QString& message,
                                      int duration = 5000);

private:
    SystemUtils() = delete; // 工具类，禁止实例化

#ifdef Q_OS_WIN
    static QString getWindowsActiveWindow();
    static int getWindowsIdleTime();
#elif defined(Q_OS_LINUX)
    static QString getLinuxActiveWindow();
    static int getLinuxIdleTime();
#elif defined(Q_OS_MACOS)
    static QString getMacActiveWindow();
    static int getMacIdleTime();
#endif
};