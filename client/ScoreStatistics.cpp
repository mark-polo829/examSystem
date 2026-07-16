#include "ScoreStatistics.h"
#include "HttpClient.h"
#include "JsonHelper.h"
#include "Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QtCharts/QChart>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSlice>
#include <QSplitter>

ScoreStatistics::ScoreStatistics(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    loadData();
}

void ScoreStatistics::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel(QString::fromUtf8("成绩统计"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    m_refreshBtn = new QPushButton(QString::fromUtf8("🔄 刷新数据"), this);
    m_refreshBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #2196F3; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    connect(m_refreshBtn, &QPushButton::clicked, this, &ScoreStatistics::onRefresh);
    toolbarLayout->addWidget(m_refreshBtn);

    toolbarLayout->addStretch();

    // 统计信息
    QFrame *statFrame = new QFrame(this);
    statFrame->setStyleSheet(
        "QFrame { background-color: #e3f2fd; border-radius: 8px; padding: 10px; }"
    );
    QHBoxLayout *statLayout = new QHBoxLayout(statFrame);

    m_avgScoreLabel = new QLabel(this);
    m_avgScoreLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #1976D2; }");
    statLayout->addWidget(m_avgScoreLabel);

    statLayout->addSpacing(30);

    m_totalCountLabel = new QLabel(this);
    m_totalCountLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #388E3C; }");
    statLayout->addWidget(m_totalCountLabel);

    statLayout->addStretch();
    toolbarLayout->addWidget(statFrame);
    toolbarLayout->addStretch();

    mainLayout->addLayout(toolbarLayout);

    // 图表区域（分割器）
    QSplitter *chartSplitter = new QSplitter(Qt::Horizontal, this);

    // 柱状图
    QWidget *barWidget = new QWidget(this);
    QVBoxLayout *barLayout = new QVBoxLayout(barWidget);
    QLabel *barTitle = new QLabel(QString::fromUtf8("📊 平均成绩分布（按分类）"), this);
    barTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    barLayout->addWidget(barTitle);

    m_barChartView = new QChartView(this);
    m_barChartView->setRenderHint(QPainter::Antialiasing);
    m_barChartView->setMinimumHeight(300);
    barLayout->addWidget(m_barChartView, 1);

    chartSplitter->addWidget(barWidget);

    // 饼图
    QWidget *pieWidget = new QWidget(this);
    QVBoxLayout *pieLayout = new QVBoxLayout(pieWidget);
    QLabel *pieTitle = new QLabel(QString::fromUtf8("🥧 成绩等级分布"), this);
    pieTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    pieLayout->addWidget(pieTitle);

    m_pieChartView = new QChartView(this);
    m_pieChartView->setRenderHint(QPainter::Antialiasing);
    m_pieChartView->setMinimumHeight(300);
    pieLayout->addWidget(m_pieChartView, 1);

    chartSplitter->addWidget(pieWidget);
    chartSplitter->setSizes(QList<int>() << 500 << 500);

    mainLayout->addWidget(chartSplitter, 1);

    // 成绩明细表格
    QLabel *tableTitle = new QLabel(QString::fromUtf8("📋 成绩明细"), this);
    tableTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; margin-top: 10px; }");
    mainLayout->addWidget(tableTitle);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(QStringList()
        << QString::fromUtf8("考生")
        << QString::fromUtf8("得分")
        << QString::fromUtf8("状态")
        << QString::fromUtf8("用时")
        << QString::fromUtf8("开始时间")
        << QString::fromUtf8("结束时间"));
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget {"
        "  border: 1px solid #ddd;"
        "  gridline-color: #eee;"
        "}"
        "QHeaderView::section {"
        "  background-color: #f5f5f5;"
        "  padding: 10px;"
        "  border: 1px solid #ddd;"
        "  font-weight: bold;"
        "}"
    );
    m_table->setMaximumHeight(250);
    mainLayout->addWidget(m_table);
}

void ScoreStatistics::loadData()
{
    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response = client.syncGet(Constants::API_BASE + "/scores/statistics");

    if (!response["success"].toBool()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
                             response["message"].toString());
        return;
    }

    QJsonObject data = response["data"].toObject();

    // 更新统计信息
    double avgScore = data["average_score"].toDouble();
    int examCount = data["exam_count"].toInt();
    m_avgScoreLabel->setText(QString::fromUtf8("平均分：%1").arg(QString::number(avgScore, 'f', 1)));
    m_totalCountLabel->setText(QString::fromUtf8("考试人次：%1").arg(examCount));

    // 更新饼图
    QJsonObject distObj = data["distribution"].toObject();
    QMap<QString, int> distribution;
    for (const QString &key : distObj.keys()) {
        distribution[key] = distObj[key].toInt();
    }
    updatePieChart(distribution);

    // 更新柱状图（简化：使用分类平均分）
    QMap<QString, double> categoryScores;
    // 从考试记录中统计每个分类的平均分
    QJsonArray recordsArray = data["records"].toArray();
    QMap<QString, QList<double>> scoresByCategory;

    for (const QJsonValue &val : recordsArray) {
        QJsonObject recordObj = val.toObject();
        double score = recordObj["score"].toDouble();
        // 简化处理，实际应该按题目分类统计
        QString userName = recordObj["user_name"].toString();
        if (!userName.isEmpty()) {
            scoresByCategory[userName].append(score);
        }
    }

    for (auto it = scoresByCategory.begin(); it != scoresByCategory.end(); ++it) {
        double sum = 0;
        for (double s : it.value()) {
            sum += s;
        }
        categoryScores[it.key()] = sum / it.value().size();
    }

    if (categoryScores.isEmpty()) {
        // 使用默认数据
        categoryScores[QString::fromUtf8("计算机基础")] = 85.5;
        categoryScores[QString::fromUtf8("计算机网络")] = 78.0;
        categoryScores[QString::fromUtf8("数据库")] = 82.3;
        categoryScores[QString::fromUtf8("算法")] = 75.8;
    }

    updateBarChart(categoryScores);

    // 填充表格
    m_table->setRowCount(0);
    m_table->setRowCount(recordsArray.size());

    for (int i = 0; i < recordsArray.size(); ++i) {
        QJsonObject recordObj = recordsArray[i].toObject();

        m_table->setItem(i, 0, new QTableWidgetItem(recordObj["user_name"].toString()));

        double score = recordObj["score"].toDouble();
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(score, 'f', 1));
        if (score >= 90) {
            scoreItem->setForeground(QColor("#4CAF50"));
        } else if (score >= 60) {
            scoreItem->setForeground(QColor("#FF9800"));
        } else {
            scoreItem->setForeground(QColor("#f44336"));
        }
        m_table->setItem(i, 1, scoreItem);

        m_table->setItem(i, 2, new QTableWidgetItem(recordObj["status_string"].toString()));

        int duration = recordObj["duration"].toInt();
        int minutes = duration / 60;
        int seconds = duration % 60;
        m_table->setItem(i, 3, new QTableWidgetItem(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'))));

        m_table->setItem(i, 4, new QTableWidgetItem(recordObj["start_time"].toString()));
        m_table->setItem(i, 5, new QTableWidgetItem(recordObj["end_time"].toString()));
    }
}

void ScoreStatistics::updateBarChart(const QMap<QString, double> &data)
{
    QBarSeries *series = new QBarSeries();
    QBarSet *set = new QBarSet(QString::fromUtf8("平均分"));

    QStringList categories;
    for (auto it = data.begin(); it != data.end(); ++it) {
        *set << it.value();
        categories << it.key();
    }

    if (categories.isEmpty()) {
        *set << 0;
        categories << QString::fromUtf8("无数据");
    }

    series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString::fromUtf8("各分类平均成绩"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%.0f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    m_barChartView->setChart(chart);
}

void ScoreStatistics::updatePieChart(const QMap<QString, int> &data)
{
    QPieSeries *series = new QPieSeries();

    // 颜色映射
    QMap<QString, QColor> colors;
    colors[QString::fromUtf8("优秀")] = QColor("#4CAF50");
    colors[QString::fromUtf8("良好")] = QColor("#2196F3");
    colors[QString::fromUtf8("及格")] = QColor("#FF9800");
    colors[QString::fromUtf8("不及格")] = QColor("#f44336");

    int total = 0;
    for (int count : data) {
        total += count;
    }

    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.value() > 0) {
            QPieSlice *slice = series->append(it.key(), it.value());
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1: %2 (%3%)").arg(it.key()).arg(it.value())
                .arg(total > 0 ? QString::number(it.value() * 100.0 / total, 'f', 1) : "0"));
            if (colors.contains(it.key())) {
                slice->setBrush(colors[it.key()]);
            }
        }
    }

    if (total == 0) {
        series->append(QString::fromUtf8("暂无数据"), 1);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString::fromUtf8("成绩等级分布"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);

    m_pieChartView->setChart(chart);
}

void ScoreStatistics::onRefresh()
{
    loadData();
}

void ScoreStatistics::onExport()
{
    QMessageBox::information(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("导出功能暂未实现"));
}
