
#include "ui/StatisticsPanel.h"
#include <QCalendarWidget>
#include <QFormLayout>
#include <QGroupBox>
#include <QEvent>

StatisticsPanel::StatisticsPanel(DataAnalyzer* analyzer, QWidget *parent)
    : QDialog(parent), m_analyzer(analyzer)
{
    setupUi();
    connect(m_calendar, &QCalendarWidget::selectionChanged, this, &StatisticsPanel::refreshReport);
    connect(m_refreshButton, &QPushButton::clicked, this, &StatisticsPanel::refreshReport);

    // 加载当天的数据
    loadReportForDate(QDate::currentDate());
}

StatisticsPanel::~StatisticsPanel()
{
}

void StatisticsPanel::setupUi()
{
    setWindowTitle(tr("健康数据统计"));
    setMinimumSize(400, 500);

    m_mainLayout = new QVBoxLayout(this);

    m_titleLabel = new QLabel(tr("健康数据统计"), this);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_calendar = new QCalendarWidget(this);
    m_calendar->setSelectedDate(QDate::currentDate());
    m_calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    QGroupBox *reportGroup = new QGroupBox(tr("每日报告"), this);
    QFormLayout *formLayout = new QFormLayout(reportGroup);

    m_reportDateLabel = new QLabel(this);
    m_totalActiveLabel = new QLabel(this);
    m_totalBreaksLabel = new QLabel(this);
    m_longestSessionLabel = new QLabel(this);
    m_healthScoreLabel = new QLabel(this);

    formLayout->addRow(tr("日期:"), m_reportDateLabel);
    formLayout->addRow(tr("总计专注时间:"), m_totalActiveLabel);
    formLayout->addRow(tr("总计休息次数:"), m_totalBreaksLabel);
    formLayout->addRow(tr("最长连续专注:"), m_longestSessionLabel);
    formLayout->addRow(tr("健康得分:"), m_healthScoreLabel);

    m_refreshButton = new QPushButton(tr("刷新数据"), this);

    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_calendar);
    m_mainLayout->addWidget(reportGroup);
    m_mainLayout->addWidget(m_refreshButton, 0, Qt::AlignRight);

    setLayout(m_mainLayout);
}

void StatisticsPanel::loadReportForDate(const QDate& date)
{
    if (!m_analyzer) return;

    DataAnalyzer::DailyReport report = m_analyzer->getDailyReport(date);
    updateLabels(report);
}

void StatisticsPanel::updateLabels(const DataAnalyzer::DailyReport& report)
{
    m_reportDateLabel->setText(report.date.toString(Qt::TextDate));
    
    int hours = report.totalActiveMinutes / 60;
    int minutes = report.totalActiveMinutes % 60;
    m_totalActiveLabel->setText(tr("%1 小时 %2 分钟").arg(hours).arg(minutes));

    m_totalBreaksLabel->setText(tr("%1 次").arg(report.totalBreaks));
    m_longestSessionLabel->setText(tr("%1 分钟").arg(report.longestSittingSession));
    
    m_healthScoreLabel->setText(QString::number(report.healthScore, 'f', 1));
    if (report.healthScore >= 85) {
        m_healthScoreLabel->setStyleSheet("color: green;");
    } else if (report.healthScore >= 60) {
        m_healthScoreLabel->setStyleSheet("color: orange;");
    } else {
        m_healthScoreLabel->setStyleSheet("color: red;");
    }
}

void StatisticsPanel::onDateChanged(const QDate& date)
{
    loadReportForDate(date);
}

void StatisticsPanel::refreshReport()
{
    loadReportForDate(m_calendar->selectedDate());
}

// 用于主题或语言切换时更新UI文本
void StatisticsPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        setWindowTitle(tr("健康数据统计"));
        m_titleLabel->setText(tr("健康数据统计"));
        qobject_cast<QGroupBox*>(m_totalActiveLabel->parentWidget()->parentWidget())->setTitle(tr("每日报告"));
        m_refreshButton->setText(tr("刷新数据"));
        // Note: Form layout labels need to be reset manually if needed
    }
    QDialog::changeEvent(event);
}
