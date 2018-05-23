#include "graphworker.h"
#include "ui_graphworker.h"
#include <QFileDialog>

GraphWorker::GraphWorker(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphWorker)
{
    ui->setupUi(this);
    splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    layout = new QVBoxLayout();
    layout->addWidget(splitter);
    this->setLayout(layout);
    this->setAttribute( Qt::WA_QuitOnClose, false );
}

GraphWorker::~GraphWorker()
{
    delete ui;

    delete splitter;
    delete layout;

}

void GraphWorker::setData(int num_pl, int num_gr, float x, float y)
{
    if (widgets.size() <= num_pl || widgets[num_pl]->x_data.size() <= num_gr)
        return;
    if (widgets[num_pl]->y_data[num_gr].isEmpty()){
        max_y[num_pl] = y;
        min_y[num_pl] = y;
        if (!widgets[num_pl]->curves.isEmpty()){
            max_x[num_pl] = x;
            min_x[num_pl] = x;
        }
        else{
            widgets[num_pl]->xAxis->setRange(0, max_time); //по умолчанию шкала = max_time, дальше будет смещаться
        }
    }

    if ((widgets[num_pl]->x_data[num_gr].size() > 0) && widgets[num_pl]->curves.isEmpty()){ //смотрим нужно ли считать время
        widgets[num_pl]->y_data[num_gr].append(y);
        widgets[num_pl]->x_data[num_gr].append(x+widgets[num_pl]->x_data[num_gr][widgets[num_pl]->x_data[num_gr].size()-1]);
    }
    else{ //время считать не нужно, т.к. рисуем траекторию
        widgets[num_pl]->x_data[num_gr].append(x);
        widgets[num_pl]->y_data[num_gr].append(y);
    }

    if ((widgets[num_pl]->x_data[num_gr].last() > max_time) && widgets[num_pl]->curves.isEmpty()){ //если нужно сдвигать шкалу сдвигать шкалу
        //пересчитываем макс-мин шкалу
        if (max_y[num_pl] == widgets[num_pl]->y_data[num_gr].first()){
            max_y[num_pl] = *std::max_element(widgets[num_pl]->y_data[num_gr].begin()+1, widgets[num_pl]->y_data[num_gr].end());
            widgets[num_pl]->yAxis->setRange(min_y[num_pl] - 0.05*fabs(min_y[num_pl]), max_y[num_pl] + 0.05*fabs(max_y[num_pl]));
        }
        if (min_y[num_pl] == widgets[num_pl]->y_data[num_gr].first()){
            min_y[num_pl] = *std::min_element(widgets[num_pl]->y_data[num_gr].begin()+1, widgets[num_pl]->y_data[num_gr].end());
            widgets[num_pl]->yAxis->setRange(min_y[num_pl] - 0.05*fabs(min_y[num_pl]), max_y[num_pl] + 0.05*fabs(max_y[num_pl]));
        }
        widgets[num_pl]->x_data[num_gr].removeFirst();
        widgets[num_pl]->y_data[num_gr].removeFirst();
        widgets[num_pl]->xAxis->setRange(widgets[num_pl]->x_data[num_gr].first(), widgets[num_pl]->x_data[num_gr].last());
    }

    if (widgets[num_pl]->curves.isEmpty())
        widgets[num_pl]->graph(num_gr)->setData(widgets[num_pl]->x_data[num_gr], widgets[num_pl]->y_data[num_gr]);
    else{
        widgets[num_pl]->curves[num_gr]->setData(widgets[num_pl]->x_data[num_gr], widgets[num_pl]->y_data[num_gr]);
    }

    //устанавливаем границы для y
    if (y > max_y[num_pl]){
        max_y[num_pl] = y;
        widgets[num_pl]->yAxis->setRange(min_y[num_pl] - 0.05*fabs(min_y[num_pl]), max_y[num_pl] + 0.05*fabs(max_y[num_pl]));
    }
    if (y < min_y[num_pl]){
        min_y[num_pl] = y;
        widgets[num_pl]->yAxis->setRange(min_y[num_pl] - 0.05*fabs(min_y[num_pl]), max_y[num_pl] + 0.05*fabs(max_y[num_pl]));
    }
    //устанавливаем границы для x, только если траектория
    if (!widgets[num_pl]->curves.isEmpty()){
        if (x > max_x[num_pl]){
            max_x[num_pl] = x;
            widgets[num_pl]->xAxis->setRange(min_x[num_pl] - 0.05*fabs(min_x[num_pl]), max_x[num_pl] + 0.05*fabs(max_x[num_pl]));
        }
        if (x < min_x[num_pl]){
            min_x[num_pl] = x;
            widgets[num_pl]->xAxis->setRange(min_x[num_pl] - 0.05*fabs(min_x[num_pl]), max_x[num_pl] + 0.05*fabs(max_x[num_pl]));
        }
    }

    widgets[num_pl]->replot();
}

int GraphWorker::addPlot(const QString &title, const QString &x, const QString &y, int amount_gr, bool xy_plot)
{
    static int i = 0;
    i++;
    widgets.append(new PlotWidget);

    splitter->addWidget(widgets[i-1]);

    widgets[i-1]->x_data.resize(amount_gr);
    widgets[i-1]->y_data.resize(amount_gr);

    for(int k = 0; k < amount_gr; k++){
        QPen pen;
        pen.setColor(static_cast<Qt::GlobalColor>(colors[k]));
        pen.setWidthF(2);
        if (xy_plot){
            widgets[i-1]->curves.append(new QCPCurve(widgets[i-1]->xAxis, widgets[i-1]->yAxis));
            //widgets[i-1]->curves[k]->setPen(QPen(static_cast<Qt::GlobalColor>(colors[k])));
            widgets[i-1]->curves[k]->setPen(pen);
        }
        else{
            widgets[i-1]->addGraph();
            //widgets[i-1]->graph(k)->setPen(QPen(static_cast<Qt::GlobalColor>(colors[k])));
            widgets[i-1]->graph(k)->setPen(pen);
        }
    }


    widgets[i-1]->xAxis->setLabel(x);
    //widgets[i-1]->xAxis->setTicker(QSharedPointer<QCPAxisTickerPi>(new QCPAxisTickerPi));
    widgets[i-1]->yAxis->setLabel(y);
    widgets[i-1]->xAxis->grid()->setVisible(false);

    //title
    widgets[i-1]->plotLayout()->insertRow(0);
    widgets[i-1]->plotLayout()->addElement(0, 0, new QCPTextElement(widgets[i-1], title, QFont("sans", 17, QFont::Cursive)));

    //легенды
    widgets[i-1]->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(15);
    widgets[i-1]->legend->setFont(legendFont);
    widgets[i-1]->legend->setBrush(QBrush(QColor(255,255,255,230)));

    //здесь храним максимум и минимум каждого поля
    max_y.resize(i);
    min_y.resize(i);
    max_x.resize(i);
    min_x.resize(i);

    return i; //возвращаем размер (сколько уже полей)
}

void GraphWorker::setGraphLegend(int num_pl, int num_gr, const QString &legend)
{
    if (widgets.size() <= num_pl || widgets[num_pl]->x_data.size() <= num_gr)
        return;
    if (!widgets[num_pl]->curves.isEmpty()){
        widgets[num_pl]->curves[num_gr]->setName(legend);
    }
    else{
        widgets[num_pl]->graph(num_gr)->setName(legend);
    }
}



PlotWidget::PlotWidget(QWidget *parent):QCustomPlot(parent)
{
    QAction *save = new QAction("save", this);
    connect(save, SIGNAL(triggered(bool)), this, SLOT(saveGraph()));
    QAction *clear = new QAction("clear", this);
    connect(clear, static_cast<void (QAction::*)(bool)>(&QAction::triggered), this, [this](){
        for (int i = 0; i < x_data.size(); i++){
            x_data[i].clear();
            y_data[i].clear();
            if (curves.isEmpty())
                graph(i)->setData(x_data[i], y_data[i]);
            else
                curves[i]->setData(x_data[i], y_data[i]);
        }
        replot(true);
    });
    menu.addAction(save);
    menu.addAction(clear);

    connect(this, static_cast<void (QCustomPlot::*)(QMouseEvent*)>(&QCustomPlot::mousePress), this, [this](QMouseEvent* event){
        if (event->button() == Qt::RightButton){
            menu.exec(event->globalPos());
        }
    });
}

PlotWidget::~PlotWidget()
{
}

void PlotWidget::replot(bool anyway, QCustomPlot::RefreshPriority refreshPriority)
{
    counter++;
    if ((counter > draw_count) || anyway){
        QCustomPlot::replot(refreshPriority);
        counter = 0;
    }
}

void PlotWidget::saveGraph()
{
    QPixmap pixmap(this->size());
    this->render(&pixmap);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранение графика"),
                                 "C:/untitled.jpeg",
                                 tr("Images (*.jpeg)"));
    if (!fileName.isEmpty())
        pixmap.toImage().save( fileName, "JPEG" , 100);
}
