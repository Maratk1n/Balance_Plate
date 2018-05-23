#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationDisplayName("Balance Plate");
    QFont font = app.font();
    font.setPointSize(14);
    app.setFont(font);
    MainWindow w;
    w.show();

    return app.exec();
}
