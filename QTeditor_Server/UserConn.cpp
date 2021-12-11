#include "UserConn.h"

UserConn::UserConn(QString username, QString password, QString nickname, int siteId, QTcpSocket* socket, QString filename):
    User(username, password, nickname, siteId), socket(socket), filename(filename)
{
}

QString UserConn::getFilename()
{
    return filename;
}

QTcpSocket* UserConn::getSocket()
{
    return socket;
}

void UserConn::setFilename(QString filename)
{
    this->filename = filename;
}
