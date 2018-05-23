#ifndef GRAPHWORKER_H
#define GRAPHWORKER_H

#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include "libs/qcustomplot.h"

class PlotWidget;

namespace Ui {
class GraphWorker;
}

class GraphWorker : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWorker(QWidget *parent = 0);
    ~GraphWorker();
    void setData(int num_pl, int num_gr, float x, float y); //номер плота, номер графика, данные
    int addPlot(const QString &title, const QString &x, const QString &y, int amount_gr, bool xy_plot = false); //amount_gr - кол-во графиков на одном плоте
    void setGraphLegend(int num_pl, int num_gr, const QString &legend); //номер плота, номер графика, имя графика

private:
    Ui::GraphWorker *ui;

    QSplitter *splitter;
    QVector<PlotWidget*> widgets;
    QVBoxLayout *layout;

    QVector<float> max_y;
    QVector<float> min_y;
    QVector<float> max_x;
    QVector<float> min_x;

    //тут храним цвета
    int colors[6] = {Qt::red, Qt::darkGreen, Qt::blue, Qt::magenta, Qt::yellow, Qt::darkGray};
    const int max_time = 60; //ширина поля в секундах
};

class PlotWidget : public QCustomPlot
{
    Q_OBJECT
    QMenu menu;
    QAction *save;
    QAction *clear;
    int draw_count = 3000;
    int counter = 0;
public:
    PlotWidget(QWidget *parent = 0);
    ~PlotWidget();
    QVector<QCPCurve*> curves;
    QVector<QVector<double>> x_data;
    QVector<QVector<double>> y_data;
    void replot(bool anyway = false, RefreshPriority refreshPriority = QCustomPlot::rpRefreshHint);
private slots:
    void saveGraph();
};

#endif // GRAPHWORKER_H
