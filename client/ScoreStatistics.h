#ifndef SCORE_STATISTICS_H
#define SCORE_STATISTICS_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QPieSeries>

QT_CHARTS_USE_NAMESPACE

class ScoreStatistics : public QWidget
{
    Q_OBJECT
public:
    explicit ScoreStatistics(int userId, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onExport();

private:
    void setupUI();
    void loadData();
    void updateBarChart(const QMap<QString, double> &data);
    void updatePieChart(const QMap<QString, int> &data);

    int m_userId;

    QComboBox *m_examCombo;
    QPushButton *m_refreshBtn;
    QPushButton *m_exportBtn;

    QTableWidget *m_table;
    QChartView *m_barChartView;
    QChartView *m_pieChartView;

    QLabel *m_avgScoreLabel;
    QLabel *m_totalCountLabel;
};

#endif // SCORE_STATISTICS_H
