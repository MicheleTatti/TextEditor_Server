#include <QtCore/QCoreApplication>
#include "Server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Server s;
    QObject::connect(&s, &Server::closed, &a, &QCoreApplication::quit);//per chiudere l'app
    return a.exec();
}
