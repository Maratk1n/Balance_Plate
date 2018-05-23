#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QSerialPort>
#include <QVector>
#include <QVariant>
#include <QListWidget>

class ComPort;

//для справки
//typedef unsigned char           uint8_t;
//typedef unsigned short          uint16_t;
//typedef unsigned int            uint32_t;
//typedef unsigned long int       uint64_t;

struct ProtocolTypes{
    uint8_t uint8;
    uint16_t uint16;
};
extern struct ProtocolTypes protocol_types;
Q_DECLARE_METATYPE(ProtocolTypes)


class Device: public QObject{
    Q_OBJECT

    ComPort *port;
    int command;
    QVector<QVariant> w_data;
    QVector<QVariant> r_data;
    float _delta_t;
    int _fps;
    float dt_arr[30]; //тут копим дельта Т для подсчета fps
    QElapsedTimer timer;
public:
    Device(int command = 0x00);
    ~Device();
    void setPort(ComPort *port);

    void setWDataType(QVariant dataType, int count);
    void setRDataType(QVariant dataType, int count);
    void setWData(int i, QVariant data);
    QVariant getRData(int i);
    void send();
    int fps(){return _fps;}
    float delta_t(){return _delta_t;} //в секундах
private slots:
    void packageAnalysis(QByteArray data);
signals:
    void dataReady();
};

class ComPort : public QObject {
    Q_OBJECT
private:
    QByteArray readBuf;
    QString portName = "COM4";
    QSerialPort serialPort;

    //переменные для поиска посылок
    QByteArray temp_arr;
    QVector<QByteArray> packages;
    //рекурсивная ф-я для поиска посылок
    void searchPackages(QByteArray rx_data);

    int maxWidgetSize = 50;
    int timeoutCount = 0;
    int responseCount = 0;
    int lossesCount = 0;
    static const int FRAME_MAX_LEN = 0x32;
    uint8_t CON_1_ADDR = 0x01;
    QListWidget *widget_pc = nullptr;
    QListWidget *widget_a = nullptr;
    void addLog(QListWidget *widget, const QString &text);
public:
    explicit ComPort(QObject *parent = 0);
    ~ComPort();

    int open();
    void close();
    QByteArray requestResponse(const QByteArray &data);
    void write(const QByteArray &data);
    bool crcCheck(const QByteArray &data);
    bool lengthCheck(const QByteArray &data);
    void setPortName(QString name){
        portName = name;
    }
    QString getPortName(){
        return serialPort.portName();
    }
    void setWidgettPC(QListWidget *widget_pc){
        this->widget_pc = widget_pc;
    }
    void setWidgetA(QListWidget *widget_a){
        this->widget_a = widget_a;
    }
private slots:
    void readData();
signals:
    void packageReady(QByteArray);
};

#endif // PROTOCOL_H
