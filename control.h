#ifndef CONTROL_H
#define CONTROL_H

#include <QWidget>
#include <QDebug>

//вычисление интеграла методом трапеции
float integral(float *sum, float prev, float curr, float delta, int rank = 1);
//вычисление производной
float derivative_1(float f_curr, float f_prev, float delta);
float derivative_2(float f_curr, float f_prev, float f_prev_2, float delta);

class Pid{
    float sum = 0.0;
    float prev_err = 0.0; //предыдущее значение ошибки
    float data_arr[5];
    float average_filter(float data){
        float sum;
        for (int i = 1; i < sizeof(data_arr)/sizeof(*data_arr); i++){
            if (data_arr[i] != 0.0)
                data_arr[i-1] = data_arr[i];
            else
                data_arr[i-1] = data;
            sum += data_arr[i-1];
        }
        data_arr[sizeof(data_arr)/sizeof(*data_arr)-1] = data;
        sum += data;
        return sum/(sizeof(data_arr)/sizeof(*data_arr));
    }
public:

    //коэффициенты для модели шарика с сервой
    float Kp = -0.920003565525112;
    float Ki = 0.013470463627951;
    float Kd = -0.697099216214480;

    Pid(){
        for (int i = 0; i < sizeof(data_arr)/sizeof(*data_arr); i++)
            data_arr[i] = 0.0;
    }
    Pid(float Kp, float Ki, float Kd):Kp(Kp), Ki(Ki), Kd(Kd){
        for (int i = 0; i < sizeof(data_arr)/sizeof(*data_arr); i++)
            data_arr[i] = 0.0;
    }

    /* Возвращает сигнал управления (угол платформы)
     * Аргументы: error - ошибка управления
     */
    float calc(float error, float delta_t){
        float cur_i = integral(&sum, prev_err, error, delta_t);
        float cur_d = derivative_1(error, prev_err, delta_t);
        prev_err = error;
        qDebug() << "sum: " << sum;
        return average_filter(Ki*cur_i + Kd*cur_d + Kp*error);
        //return Ki*cur_i + Kd*cur_d + Kp*error;
    }
    void reset(){
        sum = 0.0;
        prev_err = 0.0;
        for (int i = 0; i < sizeof(data_arr)/sizeof(*data_arr); i++)
            data_arr[i] = 0.0;
    }
};

namespace Ui {
class Control;
}

class Control : public QWidget
{
    Q_OBJECT

public:
    explicit Control(QWidget *parent = 0);
    explicit Control(Pid *pid_x, Pid *pid_y, QWidget *parent = 0):pid_x(pid_x), pid_y(pid_y), QWidget(parent){}
    ~Control();
    void setPidX(Pid *pid_x){this->pid_x = pid_x;}
    void setPidY(Pid *pid_y){this->pid_y = pid_y;}

private:
    float map(float x, float in_min, float in_max, float out_min, float out_max){ //ф-я перевода из одной шкалы в другую
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
    Ui::Control *ui;
    Pid *pid_x;
    Pid *pid_y;
};

#endif // CONTROL_H
