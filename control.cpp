#include "control.h"
#include "ui_control.h"

/****
 * Возвращает посчитанное значение интеграла
 * Аргументы:
 * *sum - указатель на переменную, в которой будет суммироваться интеграл
 * prev - предыдущее значение функции в точке
 * curr - текущее значение в точке
 * delta - шаг интегрирования (например, промежуток времени)
 * rank - порядок производной
 */
float integral(float *sum, float prev, float curr, float delta, int rank)
{
    *sum += 0.5*(prev+curr)*delta;
    return *sum;
}

/***
 * Функция для нахождения производной первого порядка:
 * f_curr - текущее значение функции в точке
 * f_prev - предыдущее значение функции в точке
 * delta - шаг сетки (например, промежуток времени)
 */
float derivative_1(float f_curr, float f_prev, float delta)
{
    if (delta == 0.0){
        return 0.0;
    }
    return (f_curr - f_prev) / delta;
}
/***
 * Функция для нахождения производной второго порядка:
 * f_curr - текущее значение функции в точке
 * f_prev - предыдущее значение функции в точке
 * f_prev_2 - значение функции в точке два шага назад
 * delta - шаг сетки (например, промежуток времени)
 */
float derivative_2(float f_curr, float f_prev, float f_prev_2, float delta)
{
    if (delta == 0.0){
        return 0.0;
    }
    return (f_curr - 2*f_prev + f_prev_2)/(delta*delta);
}

Control::Control(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Control)
{
    ui->setupUi(this);
    this->setAttribute( Qt::WA_QuitOnClose, false );
    connect(ui->horizontalSlider_Kp, static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged), this, [this](int value){
        pid_x->Kp = map(value, 0, 2000, -0.5, 1.0);
        ui->label_Kp->setText(QString("K<sub>p</sub>: ") + QString::number(pid_x->Kp, 'f', 3));
    });
    connect(ui->horizontalSlider_Ki, static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged), this, [this](int value){
        pid_x->Ki = map(value, 0, 2000, -0.5, 1.0);
        ui->label_Ki->setText(QString("K<sub>i</sub>: ") + QString::number(pid_x->Ki, 'f', 3));
    });
    connect(ui->horizontalSlider_Kd, static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged), this, [this](int value){
        pid_x->Kd = map(value, 0, 2000, -0.5, 1.0);
        ui->label_Kd->setText(QString("K<sub>d</sub>: ") + QString::number(pid_x->Kd, 'f', 3));
    });
}

Control::~Control()
{
    delete ui;
}
