#include "QNetMount.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QNetMount w;
    w.show();
    return a.exec();
}
