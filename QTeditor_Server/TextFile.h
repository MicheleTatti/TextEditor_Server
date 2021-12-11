#pragma once
#include <iostream>
#include <vector>
#include <QtNetwork/qtcpsocket.h>
#include "Symbol.h"

class TextFile
{
public:
	TextFile(QString filename, QString filePath);
	TextFile(QString filename, QString filePath, QTcpSocket* connection);
	~TextFile();
	QString getFilename();
	QString getFilePath();
	QVector<std::shared_ptr<Symbol>> getSymbols();
	void addSymbols(QVector<std::shared_ptr<Symbol>> symbol);	// add a blocco
	QVector<std::shared_ptr<Symbol>> removeSymbols(QVector<int> siteIds, QVector<int> counters, QVector<QVector<int>> positions); // remove a blocco
	std::shared_ptr<Symbol> getSymbol(int siteId, int counter);
	QVector<QTcpSocket*> getConnections();
	void addConnection(QTcpSocket* connection);
	void removeConnection(QTcpSocket* connection);
	void pushBackSymbol(std::shared_ptr<Symbol> symbol);
	QFile* getLogFile();
	void openLogFile();
	void closeLogFile();

private:
	QString filename;							//nome del file
	QString filePath;							//path del file (utente/nomeFile)
	QVector<std::shared_ptr<Symbol>> symbols;	//vettore di simboli del file
	QVector<QTcpSocket*> connections;			//vettore di socket (utenti) connessi al file
	QFile logFile;								//file di log relativo

	int searchIndexForNewPos(QVector<int> position);
};

