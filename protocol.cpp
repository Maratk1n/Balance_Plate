#include "protocol.h"
#include <QDebug>
#define REQATTEMPTS     4
#define WAITFORREADY    200

#include <QSize>
#include <QMetaType>
#include <QTextCodec>
#include <QScrollBar>
#include <QDateTime>

struct ProtocolTypes protocol_types;

Device::Device(int command)
{
    this->command = command;
    for (int i = 0; i < sizeof(dt_arr)/sizeof(*dt_arr); i++)
        dt_arr[i] = 0.0;
}

Device::~Device()
{
}

void Device::setPort(ComPort *port)
{
    this->port = port;
    connect(this->port, SIGNAL(packageReady(QByteArray)), this, SLOT(packageAnalysis(QByteArray)));
}

void Device::setWDataType(QVariant dataType, int count)
{
    while (count){
        w_data.append(dataType);
        count--;
    }
}

void Device::setRDataType(QVariant dataType, int count)
{
    while (count){
        r_data.append(dataType);
        count--;
    }
}

void Device::setWData(int i, QVariant data)
{
    if (w_data.size() > i){
        if (data.convert(w_data[i].type())){
            w_data[i] = data;
        }
    }
}

QVariant Device::getRData(int i)
{
    if (r_data.size() > i){
        return r_data[i];
    }
    else{
        return QVariant();
    }
}

void Device::send()
{
    int sizeOfData = 0;
    for (int i = 0; i < w_data.size(); i++){
        sizeOfData += QMetaType::sizeOf(w_data[i].type());
    }
    QByteArray tx_data;
    tx_data.append(0xAB);
    tx_data.append(0x01);
    tx_data.append(command);
    tx_data.append(sizeOfData);

    for (int i = 0; i < w_data.size(); i++){
        uint8_t array[QMetaType::sizeOf(w_data[i].type())];
        switch (static_cast<QMetaType::Type>(w_data[i].type())) {
        case QMetaType::UShort: case QMetaType::UChar: case QMetaType::Short:{ //uint16_t uint8_t int16_t
            uint16_t data = w_data[i].toInt();
            //std::copy(&data, &data, array);
            memcpy(array, &data, sizeof(array));
            break;
        }
        case QMetaType::Float:{ //float
            float data = w_data[i].toFloat();
            memcpy(array, &data, sizeof(array));
            break;
        }
        default:
            break;
        }
        for (int k = 0; k < sizeof(array); k++){
            tx_data.append(array[k]);
        }
    }
    uint8_t crc = 0x00;
    for (int i = tx_data.size()-1; i > 0; i--)
        crc -= tx_data[i];
    tx_data.append(crc);
    port->write(tx_data);
}

void Device::packageAnalysis(QByteArray data)
{
    if (timer.elapsed() > 5000)
        timer.restart();

    if (data[2] == command){ //если команда посылки принадлежит текущему девайсу
        int currentPos = 4; //текущая позиция в посылке
        for (int i = 0; i < r_data.size(); i++){
            uint8_t array[QMetaType::sizeOf(r_data[i].type())];
            for (int k = 0; k < sizeof(array); k++){
                array[k] = data[currentPos++];
            }
            switch (static_cast<QMetaType::Type>(r_data[i].type())) {
            case QMetaType::UChar: { //uint8_t
                uint8_t data;
                memcpy(&data, array, sizeof(array));
                r_data[i] = QVariant::fromValue(data);
                break;
            }
            case QMetaType::UShort: { //uint16_t
                uint16_t data;
                memcpy(&data, array, sizeof(array));
                r_data[i] = QVariant::fromValue(data);
                break;
            }
            case QMetaType::Short: { //int16_t
                int16_t data;
                memcpy(&data, array, sizeof(array));
                r_data[i] = QVariant::fromValue(data);
                break;
            }
            case QMetaType::Float:{ //float
                float data;
                memcpy(&data, array, sizeof(array));
                r_data[i] = QVariant::fromValue(data);
                break;
            }
            default:
                break;
            }
        }

        //считаем fps, delta_t
        _delta_t = (float)timer.restart()*0.001;
        float dt_sum;
        for (int i = 1; i < sizeof(dt_arr)/sizeof(*dt_arr); i++){
            dt_arr[i-1] = dt_arr[i];
            dt_sum += dt_arr[i-1];
        }
        dt_arr[sizeof(dt_arr)/sizeof(*dt_arr)-1] = _delta_t;
        dt_sum += _delta_t;
        _fps = (int)(1.0/(dt_sum/(sizeof(dt_arr)/sizeof(*dt_arr))));

        emit dataReady();
    }
}


/*****************************************
 *          Класс для работы с COM портом
 * **************************************/

ComPort::ComPort(QObject *parent)
{
    connect(&serialPort, SIGNAL(readyRead()), this, SLOT(readData()));
    serialPort.setReadBufferSize(64);
}

ComPort::~ComPort()
{
    close();
    delete widget_pc;
    delete widget_a;
}

int ComPort::open()
{
    if (serialPort.isOpen())
        return 2;
    serialPort.setPortName(portName);
    serialPort.setBaudRate(115200);
    return (serialPort.open(QIODevice::ReadWrite));
}
void ComPort::close()
{
    serialPort.close();
}
void ComPort::write(const QByteArray &data)
{
    QString text = QDateTime::currentDateTime().toString("hh:mm:ss.z");
    if (!serialPort.isWritable()){
        text += "\tПорт " + portName + " не пишется!";
    }
    else{
        text += "\tPC->A\t"+QTextCodec::codecForMib(106)->toUnicode(data.toHex()).toUpper();
        serialPort.write(data);
    }
    addLog(widget_pc, text);
}
void ComPort::readData()
{

    //читаем данные с порта
    QByteArray rx_data = serialPort.readAll();
    //ищем пакеты
    searchPackages(rx_data);

    rx_data.clear();

    for (int i = 0; i < packages.size(); i++){
        if (crcCheck(packages[i])){ //если CRC сошлось
            QString text = QDateTime::currentDateTime().toString("hh:mm:ss.z")+"\tA->PC\t"+QTextCodec::codecForMib(106)->toUnicode(packages[i].toHex()).toUpper();
            addLog(widget_a, text);
            emit packageReady(packages[i]);
        }
    }
    packages.resize(0);
}
void ComPort::searchPackages(QByteArray rx_data)
{
    for (int i = 0; i < rx_data.size(); i++){
        if (i == (rx_data.size()-1)){ //если не нашли голову
            temp_arr.append(rx_data);
            if (lengthCheck(temp_arr)){
                packages.append(temp_arr);
                temp_arr.clear(); //чистим временный массив
            }
        }
        else{
            if ((rx_data[i] == 0xAB) && (rx_data[i+1] == 0x01)){ //нашли голову

                temp_arr.append(rx_data.mid(0, i));
                if (lengthCheck(temp_arr)){
                    packages.append(temp_arr);
                }
                temp_arr.clear(); //чистим временный массив

                if (rx_data.size() > (i+3)){
                    if (((uint8_t)rx_data[i+3] + 5 + i) < (rx_data.size()-i)){ //если в посылке больше или равно чем надо
                        packages.append(rx_data.mid(i, (uint8_t)rx_data[i+3]+5));
                        if (!rx_data.mid((uint8_t)rx_data[i+3]+5+i).isEmpty())
                            searchPackages(rx_data.mid((uint8_t)rx_data[i+3]+5+i));
                        return;
                    }
                }
                temp_arr.append(rx_data.mid(i)); //сохраняем все во временный массив и уходим
                return;
            }
        }
    }
}
bool ComPort::crcCheck(const QByteArray &data)
{
    if (data.isEmpty())
        return false;
    quint8 crcResult = 0x00;
    quint8 crcData = data[data.size() - 1];
    for(int i = 1; i < data.size() - 1; i++)
    {
        crcResult -= data[i];
    }
    return ((crcData == crcResult) && !data.isEmpty());
}

bool ComPort::lengthCheck(const QByteArray &data)
{
    if (data.size()>3){
        if (data.size() == (data[3]+5))
            return true;
    }
    return false;
}

void ComPort::addLog(QListWidget *widget, const QString &text)
{
    if (widget != nullptr){
        if (widget->count() > maxWidgetSize){
            delete widget->takeItem(0);
        }
        widget->addItem(text);
        widget->scrollToBottom();
    }
}

