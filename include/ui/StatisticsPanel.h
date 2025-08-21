
#pragma once

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDate>
#include "core/DataAnalyzer.h"

/**
 * @brief 统计数据显示面板
 * 
 * 显示用户的日度、周度活动统计和健康报告
 */
class StatisticsPanel : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsPanel(DataAnalyzer* analyzer, QWidget *parent = nullptr);
    ~StatisticsPanel();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onDateChanged(const QDate& date);
    void refreshReport();

private:
    void setupUi();
    void loadReportForDate(const QDate& date);
    void updateLabels(const DataAnalyzer::DailyReport& report);

    DataAnalyzer* m_analyzer; // 数据分析器的指针

    // UI 元素
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    class QCalendarWidget* m_calendar;
    QLabel* m_reportDateLabel;
    QLabel* m_totalActiveLabel;
    QLabel* m_totalBreaksLabel;
    QLabel* m_longestSessionLabel;
    QLabel* m_healthScoreLabel;
    QPushButton* m_refreshButton;
};
