#pragma once
#include <QtNetwork/qtcpsocket.h>

#include "User.h"
class UserConn :
    public User
{
public:
    UserConn(QString username, QString password, QString nickname, int siteId, QTcpSocket* socket, QString filename);

    QString getFilename();
    QTcpSocket* getSocket();
    void setFilename(QString filename);
private:
    QTcpSocket* socket;         //socket di connessione dell'utente
    QString filename;           //nome file associato alla connessione, se viene aperto un file
};

