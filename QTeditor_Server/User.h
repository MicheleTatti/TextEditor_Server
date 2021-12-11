#pragma once
#include <iostream>
#include <QtCore>
#include <qimage.h>

class User
{
public:
	User(QString username, QString password, QString nickname, int siteId);
	User(QString username, QString password, QString nickname, int siteId, QImage image);
	~User();
	QString getUsername();
	void setUsername(QString user);
	QString getPassword();
	void setPassword(QString password);
	QString getNickname();
	void setNickname(QString nickname);
	int getSiteId();
	void setSiteId(int siteId);
	QImage getImage();
	void setImage(QImage image);
	bool getHaveImage();
private:
	QString username;			//identificativo univoco dell'utente
	QString password;			//password dell'utente
	QString nickname;			//nickname dell'utente
	QImage image;				//immagine di profilo dell'utente
	bool haveImage;				//flag che indica se l'utente possiede una immagine profilo
	int siteId;					//identificativo univoco dell'utente (non scelto da quest'ultimo però, assegnato dal server)
};	