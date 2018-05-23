#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//#include <QtCore/QtGlobal>
#include <QMainWindow>
#include "protocol.h"
#include "graphworker.h"
#include "control.h"
#include <QElapsedTimer>
#include <QThread>

class QLabel;
class Device;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    /****** Функции для всяких конвертаций *******/
    float map(float x, float in_min, float in_max, float out_min, float out_max){ //ф-я перевода из одной шкалы в другую
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
    float servo2platf(float angle){ //перевод из угла сервопривода в угол платформы
        return -0.19333*angle + 17.3999;
    }
    float platf2servo(float angle){ //перевод из угла платформы в угол сервопривода
        return -5.1724*angle + 90.0;
    }
    float x_to_mm(float x_touch){ //перевод из коорд тача X (0..4000) в мм
        return map(x_touch, 43, 3956, -166.5, 166.5);
        //return map(x_touch, 130, 4040, -1.0, 1.0);
    }
    float y_to_mm(float y_touch){ //перевод из коорд тача Y (0..4000) в мм
        return map(y_touch, 128, 3969, -133.5, 133.5);
    }
    float x_to_m(float x_touch){ //перевод из коорд тача X (0..4000) в м
        return 0.001*x_to_mm(x_touch);
    }
    float y_to_m(float y_touch){ //перевод из коорд тача Y (0..4000) в м
        return 0.001*y_to_mm(y_touch);
    }
    int limiter_servo(int angle){ //ограничитель для угла сервы
        static int min = 64;
        static int max = 116;
        if (angle < min)
            return min;
        else if(angle > max)
            return max;
        else
            return angle;
    }
    int limiter_platf(int angle){ //ограничитель для угла платформы
        static int min = -5;
        static int max = 5;
        if (angle < min)
            return min;
        else if(angle > max)
            return max;
        else
            return angle;
    }

    /**** системы управления ****/
    Pid pid_x;
    Pid pid_y;
    Control control;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void servo_ValueChanged(int value = 90);

    void on_manualMode_Sliders_clicked();

    void on_manualMode_Joysticks_clicked();

    void portChanged();

    void on_pushButton_clicked();

    void mpu_data_read();

    void touch_data_read();

    void on_servosSpeed_valueChanged(int value);

    void on_pushButton_2_clicked();

private:
    GraphWorker graphWorker;
    uint8_t manualModeState = 0;
    ComPort port{this};
    Device servos{0x01};
    Device touch_device{0x02};
    Device manualMode{0x03};
    Device servo_speed{0x04};
    Device mpu_device{0x06};
    QLabel *status;
    Ui::MainWindow *ui;

    QString filename = "axis_x.mat";
    QFile *file = nullptr;
    QTextStream *stream = nullptr;
    struct Touch
    {
        float x;
        float y;
    }touch;
    float limiter(float value, float min, float max){
        if (value < min)
            return min;
        if (value > max)
            return max;
        return value;
    }

    /******* MPU9250 *******/
    float mpu[9];
    // constants
    float G = 9.807f;
    float _d2r = 3.14159265359f/180.0f; //градусы в радианы
    float _r2d = 180.0f/3.14159265359f; //радианы в градусы
    // фильтр
    struct filter
    {
        float x_cur, y_cur;
        float K;
        QElapsedTimer timer;
        float delta_t;
        float gx_prev;
        float gy_prev;
    }filter_data;
};

#endif // MAINWINDOW_H
