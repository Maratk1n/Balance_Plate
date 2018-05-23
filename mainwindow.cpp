#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QStringList>
#include <QDebug>
#include <QMetaType>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QListWidget>
#include <QList>
#include <math.h>

#define MANUAL_MODE_STYLE                    "background-color: rgb(170, 0, 255);"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    status = new QLabel(this);
    ui->statusBar->addWidget(status);

    servos.setPort(&port);
    touch_device.setPort(&port);
    manualMode.setPort(&port);
    mpu_device.setPort(&port);
    servo_speed.setPort(&port);

    servos.setWDataType(QVariant(QMetaType::UShort, nullptr), 4); //запись 4 сервиков
    servos.setRDataType(QVariant(QMetaType::UChar, nullptr), 4); //чтение обратной связи с 1 и 2 сервы
    servo_speed.setWDataType(QVariant(QMetaType::UChar, nullptr), 1);
    touch_device.setRDataType(QVariant(QMetaType::UShort, nullptr), 4);
    manualMode.setWDataType(QVariant(QMetaType::UChar, nullptr), 1);
    mpu_device.setRDataType(QVariant(QMetaType::Float, nullptr), 10);

    connect(ui->ServoSlider1, SIGNAL(valueChanged(int)), this, SLOT(servo_ValueChanged(int)));
    connect(ui->ServoSlider2, SIGNAL(valueChanged(int)), this, SLOT(servo_ValueChanged(int)));
    connect(ui->ServoSlider3, SIGNAL(valueChanged(int)), this, SLOT(servo_ValueChanged(int)));
    connect(ui->ServoSlider4, SIGNAL(valueChanged(int)), this, SLOT(servo_ValueChanged(int)));
    connect(ui->sliderValue1, SIGNAL(editingFinished()), this, SLOT(servo_ValueChanged()));
    connect(ui->sliderValue2, SIGNAL(editingFinished()), this, SLOT(servo_ValueChanged()));
    connect(ui->sliderValue3, SIGNAL(editingFinished()), this, SLOT(servo_ValueChanged()));
    connect(ui->sliderValue4, SIGNAL(editingFinished()), this, SLOT(servo_ValueChanged()));

    ui->ServoSlider1->setValue(ui->ServoSlider1->minimum() + (ui->ServoSlider1->maximum()-ui->ServoSlider1->minimum())/2);
    ui->ServoSlider2->setValue(ui->ServoSlider2->minimum() + (ui->ServoSlider2->maximum()-ui->ServoSlider2->minimum())/2);
    ui->ServoSlider3->setValue(ui->ServoSlider3->minimum() + (ui->ServoSlider3->maximum()-ui->ServoSlider3->minimum())/2);
    ui->ServoSlider4->setValue(ui->ServoSlider4->minimum() + (ui->ServoSlider4->maximum()-ui->ServoSlider4->minimum())/2);

    ui->servosSpeed->setValue((int)map(0.15, 0.07, 2.0, 0, 99));

    connect(ui->actionConnect, static_cast<void (QAction::*)(bool)>(&QAction::triggered), this, [this](){
        switch (port.open()) {
        case 0:
            status->setText("Порт '" + port.getPortName()+"' не удалось открыть");
            break;
        case 1:
            status->setText("Порт '" + port.getPortName()+"' открыт");
            break;
        case 2:
            status->setText("Порт '" + port.getPortName()+"' уже открыт");
            break;
        default:
            break;
        }
    });
    connect(ui->actionDisconnect, static_cast<void (QAction::*)(bool)>(&QAction::triggered), this, [this](){
        port.close();
        status->setText("Порт '" + port.getPortName()+"' закрыт");
    });
    connect(ui->openGraph, static_cast<void (QAction::*)(bool)>(&QAction::triggered), this, [this](){
        graphWorker.show();
    });

    connect(&touch_device, SIGNAL(dataReady()), this, SLOT(touch_data_read()));

    connect(&mpu_device, SIGNAL(dataReady()), this, SLOT(mpu_data_read()));

    connect(&servos, static_cast<void (Device::*)()>(&Device::dataReady), this, [this](){
//        graphWorker.setData(4, 0, servos.delta_t(), servos.getRData(2).toFloat());
//        graphWorker.setData(4, 1, servos.delta_t(), servos.getRData(3).toFloat());
        static bool check = true;
        static int servom[2][2];
        for (int i = 0; i < 2; i++){
            for (int k = 0; k < 2; k++){
                if (check)
                    servom[i][k] = servos.getRData(i).toInt();
            }
        }
        check = false;

        //graphWorker.setData(3, 0, servos.delta_t(), servos.getRData(0).toInt());
        //graphWorker.setData(3, 1, servos.delta_t(), ui->ServoSlider1->sliderPosition());

        for (int i = 0; i < 2; i++){
            if (servom[i][0] > servos.getRData(i).toInt())
                servom[i][0] = servos.getRData(i).toInt();
            if (servom[i][1] < servos.getRData(i).toInt())
                servom[i][1] = servos.getRData(i).toInt();
        }
    });


    ui->groupBox->setEnabled(false);

    QString portName;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        portName = info.portName();
        QAction *act = new QAction(portName, this);
        act->setCheckable(true);
        connect(act, SIGNAL(triggered(bool)), this, SLOT(portChanged()));
        ui->portCom->setTitle(QString("Порт: '"+portName+"'"));
        ui->portCom->addAction(act);
    }
    if (!ui->portCom->actions().isEmpty()){
        ui->portCom->actions().at(0)->trigger();
        port.setPortName(ui->portCom->actions().at(0)->text());
    }

    port.setWidgettPC(ui->listWidget_pc);
    port.setWidgetA(ui->listWidget_a);

    //графики
    graphWorker.addPlot("График координаты шарика", "X, mm", "Y, mm", 1, true);
    graphWorker.setGraphLegend(0,0,"Траектория");

    graphWorker.addPlot("График зависимости углов от времени", "Время, [с]", "Угол, [°]", 3);
    graphWorker.setGraphLegend(1,0,"Фильтр");
    graphWorker.setGraphLegend(1,1,"Акселерометр");
    graphWorker.setGraphLegend(1,2,"Гироскоп");

    /*
    graphWorker.addPlot("График функции y=sin(x) и ее производной", "X", "Y", 2);
    graphWorker.setGraphLegend(2,0,"y=sin(x)");
    graphWorker.setGraphLegend(2,1,"y=sin''(x)");
    */
    graphWorker.addPlot("Координата тача", "t", "S, [мм]", 2);
    graphWorker.setGraphLegend(2,0,"X");
    graphWorker.setGraphLegend(2,1,"Y");

    graphWorker.addPlot(QString("Работа ПИД-регулятора для оси X"), "Время, [с]", " ", 2);
    graphWorker.setGraphLegend(3,0,"Ошибка (e)");
    graphWorker.setGraphLegend(3,1,"Угол сервопривода (u)");

    control.setPidX(&pid_x);
    control.setPidY(&pid_y);
    //control.show();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete status;
    delete file;
    delete stream;
}

void MainWindow::servo_ValueChanged(int value)
{
    QObject *obj = sender();
    if (obj == ui->ServoSlider1){
        ui->sliderValue1->setValue(value);
        servos.setWData(0, value);
    }
    if (obj == ui->ServoSlider2){
        ui->sliderValue2->setValue(value);
        servos.setWData(1, value);
    }
    if (obj == ui->ServoSlider3){
        ui->sliderValue3->setValue(value);
        servos.setWData(2, value);
    }
    if (obj == ui->ServoSlider4){
        ui->sliderValue4->setValue(value);
        servos.setWData(3, value);
    }
    if (obj == ui->sliderValue1){
        ui->ServoSlider1->setSliderPosition(ui->sliderValue1->value());
        servos.setWData(0, ui->sliderValue1->value());
    }
    if (obj == ui->sliderValue2){
        ui->ServoSlider2->setSliderPosition(ui->sliderValue2->value());
        servos.setWData(1, ui->sliderValue2->value());
    }
    if (obj == ui->sliderValue3){
        ui->ServoSlider3->setSliderPosition(ui->sliderValue3->value());
        servos.setWData(2, ui->sliderValue3->value());
    }
    if (obj == ui->sliderValue4){
        ui->ServoSlider4->setSliderPosition(ui->sliderValue4->value());
        servos.setWData(3, ui->sliderValue4->value());
    }
    servos.send();
}
void MainWindow::portChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action){
        foreach (QAction *act, ui->portCom->actions()) {
            act->setChecked(false);
        }
        action->setChecked(true);
        port.setPortName(action->text());
        ui->portCom->setTitle(QString("Порт: '"+action->text()+"'"));
    }
}

void MainWindow::on_manualMode_Sliders_clicked()
{
    if (manualModeState == 1){
        manualModeState = 0;
        ui->groupBox->setEnabled(false);
        ui->manualMode_Sliders->setStyleSheet("");
        return;
    }
    manualModeState = 1;
    ui->groupBox->setEnabled(true);
    ui->manualMode_Sliders->setStyleSheet(MANUAL_MODE_STYLE);
}

void MainWindow::on_manualMode_Joysticks_clicked()
{
    if (manualModeState == 2){
        manualModeState = 0;
        //ui->groupBox->setEnabled(false);
        ui->manualMode_Joysticks->setStyleSheet("");
        manualMode.setWData(0, 0x00);
        manualMode.send();
        return;
    }
    manualModeState = 2;
    //ui->groupBox->setEnabled(true);
    manualMode.setWData(0, 0xFF);
    manualMode.send();
    ui->manualMode_Joysticks->setStyleSheet(MANUAL_MODE_STYLE);
}


void MainWindow::on_pushButton_clicked()
{
    Device servo_calibr{0x05};
    servo_calibr.setPort(&port);
    servo_calibr.send();
    ui->ServoSlider3->setValue(ui->ServoSlider3->minimum() + (ui->ServoSlider3->maximum()-ui->ServoSlider3->minimum())/2);
    ui->ServoSlider4->setValue(ui->ServoSlider4->minimum() + (ui->ServoSlider4->maximum()-ui->ServoSlider4->minimum())/2);
}

void MainWindow::mpu_data_read()
{
    static float delta_t = 0.0; //дельта Т в секундах
    static float angle_gy = 0.0; //здесь храним интегралы (углы) гироскопа
    static float angle_gx = 0.0;
    for (int i = 0; i < 10; i++)
        mpu[i] = mpu_device.getRData(i).toFloat();
    //альфа-бета фильтр
    if (filter_data.timer.elapsed() > 1000){
        filter_data.timer.start();
        filter_data.x_cur = 0.0f;
        filter_data.y_cur = 0.0f;
        filter_data.gx_prev = 0.0f;
        filter_data.gy_prev = 0.0f;
        angle_gx = 0.0f;
        angle_gy = 0.0f;
        filter_data.K = 0.09;
    }
    delta_t = 0.001*(float)filter_data.timer.restart();
    filter_data.delta_t = delta_t;

    angle_gx = integral(&angle_gx, filter_data.gx_prev, mpu[4], delta_t);
    //angle_gx += mpu[4]*delta_t;
    angle_gy = integral(&angle_gy, filter_data.gy_prev, mpu[5], delta_t);
    filter_data.x_cur = (1.0f-filter_data.K) * integral(&filter_data.x_cur, filter_data.gx_prev, mpu[4], delta_t) + filter_data.K*(90.0f-acosf(mpu[0])/_d2r);
    filter_data.y_cur = (1.0f-filter_data.K) * integral(&filter_data.y_cur, filter_data.gy_prev, mpu[5], delta_t) + filter_data.K*(90.0f-acosf(mpu[1])/_d2r);
    filter_data.gx_prev = mpu[4];
    filter_data.gy_prev = mpu[5];
    //вывод данных
    ui->ax->setText(QString::number(mpu[0], 'f', 2)); ui->ay->setText(QString::number(mpu[1], 'f', 2)); ui->az->setText(QString::number(mpu[2], 'f', 2));
    ui->gx->setText(QString::number(mpu[4], 'f', 2)); ui->gy->setText(QString::number(mpu[5], 'f', 2)); ui->gz->setText(QString::number(mpu[6], 'f', 2));
    ui->mx->setText(QString::number(mpu[7], 'f', 2)); ui->my->setText(QString::number(mpu[8], 'f', 2)); ui->mz->setText(QString::number(mpu[9], 'f', 2));
    ui->temp->setText(QString("Temperature [°C]: %1;").arg(QString::number(mpu[3], 'f', 2)));
    ui->angles->setText(QString("Angles [deg]: %1\t %2;").arg(QString::number(filter_data.x_cur, 'f', 2)).arg(QString::number(filter_data.y_cur, 'f', 2)));
    //graphWorker.setData(0, 0, delta_t, mpu[3]);

    graphWorker.setData(1, 0, delta_t, filter_data.x_cur);
    graphWorker.setData(1, 1, delta_t, 90.0f-acosf(mpu[0])/_d2r);
    graphWorker.setData(1, 2, delta_t, angle_gx);

    //graphWorker.setData(3, 0, delta_t, 90.0f-acosf(mpu[0])/_d2r);
    ui->fps_mpu->setText(QString("MPU: %1").arg(mpu_device.fps()));
}

void MainWindow::touch_data_read()
{
    static QElapsedTimer timer;
    static float delta_t = 0.0;
    if (timer.elapsed() > 500){
        timer.start();
        touch.x = touch_device.getRData(0).toFloat();
        touch.y = touch_device.getRData(1).toFloat();
        pid_x.reset();
        pid_y.reset();
    }

    ui->touch_x->setText(QString("X: %1 мм").arg((int)x_to_mm(touch_device.getRData(0).toFloat())));
    ui->touch_y->setText(QString("Y: %1 мм").arg((int)y_to_mm(touch_device.getRData(1).toFloat())));

    if (fabs(touch_device.getRData(0).toFloat() - touch.x) > 300){
        return;
    }
    if (fabs(touch_device.getRData(1).toFloat() - touch.y) > 300){
        return;
    }

    touch.x = touch_device.getRData(0).toFloat();
    touch.y = touch_device.getRData(1).toFloat();

    //graphWorker.setData(0, 0, touch.x, touch.y);

    delta_t = (float)timer.restart()*0.001;
    graphWorker.setData(2, 0, delta_t, touch.x);
    graphWorker.setData(2, 1, delta_t, touch.y);

    ui->fps_touch->setText(QString("TOUCH: %1").arg(touch_device.fps()));

    //управление
    float pid_ux = pid_x.calc(0.0-x_to_m(touch.x), delta_t);
    float pid_uy = pid_y.calc(0.0-y_to_m(touch.y), delta_t);
    graphWorker.setData(3, 0, delta_t, 0.0-x_to_m(touch.x));
    //graphWorker.setData(3, 1, delta_t, limiter_servo(platf2servo(pid_ux*_r2d)));
    graphWorker.setData(3, 1, delta_t, limiter_platf(pid_ux*_r2d)*0.0332);
    servos.setWData(0, limiter_servo(platf2servo(pid_ux*_r2d)));
    servos.setWData(1, limiter_servo(platf2servo(pid_uy*_r2d)));
    servos.send();

    if (file != nullptr){
        if (file->isOpen()){
            *stream << QString("%1\t%2\t%3\t%4\t%5").arg(x_to_m(touch.x)).arg(y_to_m(touch.y)).
                       arg(limiter_servo(platf2servo(pid_ux*_r2d))).
                       arg(limiter_servo(platf2servo(pid_uy*_r2d))).
                       arg(delta_t) << endl;
        }
    }

}


void MainWindow::on_servosSpeed_valueChanged(int value)
{
    ui->servoSpeedLbl->setText(QString("T<sub>p</sub>: ") + QString::number(map(value, 0, 99, 0.07, 2.0), 'f', 2));
    servo_speed.setWData(0, value);
    servo_speed.send();
}

void MainWindow::on_pushButton_2_clicked()
{
    if (file == nullptr){
        file = new QFile(filename);
        stream = new QTextStream(file);
        if (file->open(QIODevice::WriteOnly)){
            qDebug() << "start";
        }
        else
            qDebug() << "fail";
        ui->pushButton_2->setText("Стоп");
    }
    else{
        file->close();
        file = nullptr;
        stream = nullptr;
        ui->pushButton_2->setText("Старт");
    }
}
