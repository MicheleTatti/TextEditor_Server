#include "User.h"

User::User(QString username, QString password, QString nickname, int siteId)
	: username(username), password(password), nickname(nickname), siteId(siteId)
{
	this->haveImage = false;
}

User::User(QString username, QString password, QString nickname, int siteId, QImage image)
	: username(username), password(password), nickname(nickname), siteId(siteId), image(image)
{
	this->haveImage = true;
}

User::~User()
{
}

QString User::getUsername() {
	return username;
}

void User::setUsername(QString username)
{
	this->username = username;
}

QString User::getPassword() {
	return password;
}

void User::setPassword(QString password)
{
	this->password = password;
}

QString User::getNickname()
{
	return nickname;
}

void User::setNickname(QString nickname)
{
	this->nickname = nickname;
}

int User::getSiteId() {
	return siteId;
}

void User::setSiteId(int siteId)
{
	this->siteId = siteId;
}

QImage User::getImage() {
	return this->image;
}

void User::setImage(QImage image) {
	this->image = image;
	haveImage = true;
}

bool User::getHaveImage() {
	return haveImage;
}