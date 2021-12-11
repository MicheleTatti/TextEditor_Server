#include "TextFile.h"

TextFile::TextFile(QString filename, QString filePath):filename(filename),filePath(filePath)
{

}


TextFile::TextFile(QString filename, QString filePath, QTcpSocket* connection): filename(filename), filePath(filePath)
{
	connections.push_back(connection);

}

TextFile::~TextFile() {}

QString TextFile::getFilename()
{
	return filename;
}

QString TextFile::getFilePath()
{
	return filePath;
}

QVector<std::shared_ptr<Symbol>> TextFile::getSymbols()
{
	return symbols;
}

void TextFile::addSymbols(QVector<std::shared_ptr<Symbol>> newSymbols) {

	int index = symbols.size();
	int count = 0;

	for (int k = 0; k < newSymbols.size(); k++) {
		int size = symbols.size();

		if (count == 0) {

			if (size == 0) {
				index = 0;
			}

			if (size == 1) {
				if (symbols[0]->getPosition() > newSymbols[k]->getPosition()) {
					index = 0;
				}
				else {
					index = 1;
				}
			}

			if (size > 1) {
				int flag = 0;
				if (newSymbols[k]->getPosition() < symbols[0]->getPosition()) {
					index = 0;
					flag = 1;
				}
				else if (newSymbols[k]->getPosition() > symbols[size - 1]->getPosition()) {
					index = size;
					flag = 1;
				}
				int i;
				int dx, sx;
				dx = size - 1;
				sx = 0;

				while (flag == 0)
				{
					i = (dx + sx) / 2;
					if (symbols[i]->getPosition() < newSymbols[k]->getPosition() && newSymbols[k]->getPosition() < symbols[i+1]->getPosition()) {
						flag = 1;
						index = i+1;
					}
					else if ((dx == sx + 1) || dx == sx) {
						flag = 1;
						index = -1;
					} 
					else {
						if (symbols[i]->getPosition() > newSymbols[k]->getPosition()) {// il nostro simbolo ha pos minore del simbolo indicizzato -> andare a sinistra;
							dx = i;
						}
						else {
							sx = i;
						}
					}
				}
			}
			if(index  != -1){
				count++;
			}
			
		}
		else
		{
			bool successivo = false;
			if (index == -1) {
				count = 0;
				k--;
			}
			else if (newSymbols[k]->getPosition() > newSymbols[k - 1]->getPosition()) {
				if ((index != size && newSymbols[k]->getPosition() < symbols[index]->getPosition()) || (index == size)) {
					count++;
					successivo = true;
				}
			}
			if (!successivo) {

				QVector<std::shared_ptr<Symbol>> inizio = symbols.mid(0, index);
				QVector<std::shared_ptr<Symbol>> fine = symbols.mid(index, symbols.size()-index);
				QVector<std::shared_ptr<Symbol>> added = newSymbols.mid(k - count, count);
				symbols = inizio;
				symbols.append(added);
				symbols.append(fine);
				count = 0;
				k--;
			}
		}
	}
	if (index != -1) {
		QVector<std::shared_ptr<Symbol>> inizio = symbols.mid(0, index);
		QVector<std::shared_ptr<Symbol>> fine = symbols.mid(index, symbols.size() - index);
		QVector<std::shared_ptr<Symbol>> added = newSymbols.mid(newSymbols.size() - count, count);
		symbols = inizio;
		symbols.append(added);
		symbols.append(fine);
	}	
}

QVector<std::shared_ptr<Symbol>> TextFile::removeSymbols(QVector<int> siteIds, QVector<int> counters, QVector<QVector<int>> positions) {

	QVector<std::shared_ptr<Symbol>> sym;
	int index = this->symbols.size();
	int size = this->symbols.size();

	bool symbolInexistent;
	int count = 0;

	for (int k = 0; k < siteIds.size(); k++) {
		if (count == 0) {
			if (size == 1 && positions[k] == this->symbols[0]->getPosition()) {
				if (this->symbols[0]->getSiteId() == siteIds[k] && this->symbols[0]->getCounter() == counters[k]) {
					sym.push_back(this->symbols[0]);
					index = 0;
					count++;
				}
			}
			if (size > 1) {
				int flag = false;;
				if (positions[k] == this->symbols[0]->getPosition()) {
					if (symbols[0]->getSiteId() == siteIds[k] && symbols[0]->getCounter() == counters[k]) {
						index = 0;
						flag = true;
						count++;
					}
					else {
						flag = true;
					}
				}
				else if (positions[k] == this->symbols[size - 1]->getPosition()) {
					if (symbols[size - 1]->getSiteId() == siteIds[k] && symbols[size - 1]->getCounter() == counters[k]) {
						index = size - 1;
						flag = true;
						count++;
					}
					else {
						flag = true;
					}
				}
				int i;
				int dx, sx;
				dx = size - 1;
				sx = 0;

				while (!flag)
				{
					i = (dx + sx) / 2;
					if (this->symbols[i]->getPosition() == positions[k]) {
						if (symbols[i]->getSiteId() == siteIds[k] && symbols[i]->getCounter() == counters[k]) {
							flag = true;
							index = i;
							count++;
						}
						else {
							flag = true;
						}
					}
					else {
						if (dx == sx || dx == sx + 1) {
							flag = true;
						}
						else {
							if (this->symbols[i]->getPosition() > positions[k]) {// il nostro simbolo ha pos minore del simbolo indicizzato -> andare a sinistra;
								dx = i;
							}
							else {
								sx = i;
							}
						}
					}

				}
			}
		}
		else {
			bool successivo = false;
			if ( (index + count) < size && this->symbols[index + count]->getPosition() == positions[k]) {
				if (this->symbols[index + count]->getSiteId() == siteIds[k] && this->symbols[index + count]->getCounter() == counters[k]) {
					count++;
					successivo = true;
				}
			}
			if (!successivo) {
				QVector<std::shared_ptr<Symbol>> inizio = this->symbols.mid(0, index);
				QVector<std::shared_ptr<Symbol>> fine = this->symbols.mid(index + count, symbols.size() - index - count);
				QVector<std::shared_ptr<Symbol>> toErase = this->symbols.mid(index, count);
				this->symbols = inizio;
				this->symbols.append(fine);
				sym.append(toErase);
				size = this->symbols.size();
				count = 0;
				k--;
			}
		}
	}

	if (count != 0) {
		QVector<std::shared_ptr<Symbol>> inizio = this->symbols.mid(0, index);
		QVector<std::shared_ptr<Symbol>> fine = this->symbols.mid(index + count, symbols.size() - index - count);
		QVector<std::shared_ptr<Symbol>> toErase = this->symbols.mid(index, count);
		this->symbols = inizio;
		this->symbols.append(fine);
		sym.append(toErase);
	}

	return sym;
}

std::shared_ptr<Symbol> TextFile::getSymbol(int siteId, int counter) {
	int index = -1;
	for (int i = 0; i < symbols.size(); i++) {
		if (siteId == symbols[i]->getSiteId() && symbols[i]->getCounter() == counter) {
			index = i;
			break;
		}
	}
	if (index != -1) {
		return symbols[index];
	}
	else {
		return nullptr;
	}
}

QVector<QTcpSocket*> TextFile::getConnections()
{
	return connections;
}

void TextFile::addConnection(QTcpSocket* connection)
{
	if(!connections.contains(connection))
		connections.push_back(connection);
}

void TextFile::removeConnection(QTcpSocket* connection)
{
	connections.erase(
		std::remove_if(connections.begin(), connections.end(),
			[connection](QTcpSocket* c) {
				return c == connection;
			}),
		connections.end());
}

void TextFile::pushBackSymbol(std::shared_ptr<Symbol> sym) {

	symbols.push_back(sym);
}

QFile* TextFile::getLogFile() {
	return &this->logFile;
}

void TextFile::openLogFile() {

	QDir d = QDir::current();
	this->logFile.setFileName(d.filePath(filePath.split("/")[0] + "/" + filePath.split("/")[1].split(".")[0] + "_log.txt"));

	if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)){}
}

void TextFile::closeLogFile() {
	logFile.close();
	logFile.remove();
}

int TextFile::searchIndexForNewPos(QVector<int> position) {
	int size = symbols.size();
	int index;


	if (size == 0) {
		index = 0;
	}

	if (size == 1) {
		if (symbols[0]->getPosition() > position) {
			index = 0;
		}
		else {
			index = 1;
		}
	}

	if (size > 1) {
		int flag = 0;
		if (position < symbols[0]->getPosition()) {
			index = 0;
			flag = 1;
		}
		else if (position > symbols[size - 1]->getPosition()) {
			index = size;
			flag = 1;
		}
		int i;
		int dx, sx;
		dx = size - 1;
		sx = 0;

		while (flag == 0)
		{
			i = (dx + sx) / 2;
			if (symbols[i - 1]->getPosition() < position && position < symbols[i]->getPosition()) {
				flag = 1;
				index = i;
			}
			else {
				if (symbols[i - 1]->getPosition() > position) {// il nostro simbolo ha pos minore del simbolo indicizzato -> andare a sinistra;
					dx = i;
				}
				else {
					sx = i;
				}
			}
		}
	}
	return index;
}
