#pragma once

#include <qvector.h>
#include <QtGui>


class Symbol
{
public:
	Symbol(QVector<int>& position, int counter, int siteId, QChar value, bool bold, bool italic, bool underlined, int alignment,
		int textSize, QColor color, QString font);
	virtual ~Symbol();
	QVector<int>& getPosition();
	void setPosition(QVector<int> position);
	int getCounter();
	int getSiteId();
	QChar getValue();
	bool isBold();
	bool isItalic();
	bool isUnderlined();
	int getAlignment();
	int getTextSize();
	QColor& getColor();
	QString& getFont();
	bool equals(Symbol *gs);

private:
	QVector<int> position;	//Posizione del simbolo secondo protocollo CRDT
	int counter;			//contatore di quanti caratteri sono stati inseriti dall'utente prima di questo sul file
	int siteId;				//identificativo dell'utente
	QChar value;			//carattere del simbolo
	bool bold;				//true se grassetto, altrimenti false
	bool italic;			//true se corsivo, altrimenti false
	bool underlined;		//true se sottolineato, altrimenti false
	int alignment;			//segna con un indice da 1 a 4 il tipo di allineamento
	int textSize;			//dimensione del carattere
	QColor color;			//colore del carattere
	QString font;			//famiglia del font
};

