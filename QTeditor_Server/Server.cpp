
#include "Server.h"
#include "UserConn.h"
#include "User.h"

Server::~Server()
{
}

/*	Funzione che si attiva alla disconnessione di un client
*/
void Server::onDisconnected()
{
	QTcpSocket* socket = static_cast<QTcpSocket*>(QObject::sender());
	QString filename = connections.find(socket).value()->getFilename();
	qDebug() << filename;
	if (filename.compare("") != 0 && fileOwnersMap.contains(filename))
	{ //se c'è un file associato a quella connessione
		TextFile* f = files.find(filename).value();
		if (f->getConnections().size() == 1)
		{ //se ultimo connesso posso togliere dalla memoria il file e salvarlo in un file di testo
			saveFile(f);
		}
		f->removeConnection(socket); //rimozione utente dai connessi al file
		std::cout << "UTENTI CONNESSI A " << filename.toStdString() << ":\t" << f->getConnections().size() << std::endl;
		sendClient(connections[socket]->getSiteId(), connections[socket]->getNickname(), socket, false);
	}
	connections.remove(socket);
	std::cout << "UTENTI CONNESSI:\t" << connections.size() << std::endl;
}

/*	Salva un file aperto nel server in un file di testo
*/
void Server::saveFile(TextFile* f)
{
	QString filePath = f->getFilePath();
	QString username = fileOwnersMap[filePath].first();
	QDir d = QDir::current();

	QFile file(d.filePath(filePath));
	if (file.open(QIODevice::WriteOnly))
	{
		QTextStream stream(&file);
		int pos = 1;
		int size = files.find(filePath).value()->getSymbols().size();
		stream << size << endl;
		for (auto s : files.find(filePath).value()->getSymbols())
		{
			/*qDebug() << pos + 1 << " " << s->getCounter() << " " << s->getSiteId() << " " << s->getValue() << " " << s->isBold() << " " << s->isItalic() << " " << s->isUnderlined() << " " << s->getAlignment()
				<< " " << s->getTextSize() << " " << s->getColor().name() << " " << QString::fromStdString(s->getFont().toStdString()) << endl;
*/
			stream << pos++ << " " << s->getCounter() << " " << s->getSiteId() << " " << s->getValue() << " ";
			if (s->isBold())
			{
				stream << 1 << " ";
			}
			else
			{
				stream << 0 << " ";
			}
			if (s->isItalic())
			{
				stream << 1 << " ";
			}
			else
			{
				stream << 0 << " ";
			}
			if (s->isUnderlined())
			{
				stream << 1 << " ";
			}
			else
			{
				stream << 0 << " ";
			}
			stream << s->getAlignment() << " " << s->getTextSize() << " " << s->getColor().name() << " " << QString::fromStdString(s->getFont().toStdString()) << endl;
		}
	}
	file.close();
	deleteLog(f);
}

/* Gestisce la comunicazione in entrata da parte del client, ha diversi casi per tutte le necessità
*/
void Server::onReadyRead()
{
	QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
	auto myClient = connections.find(sender);
	//se esiste nel nostro elenco di client connessi riceviamo, altrimenti no
	while (sender->bytesAvailable() != 0) {
		qDebug() << "Entrato nel while iniziale";
		if (myClient != connections.end())
		{
			int operation, dim;
			QByteArray in_buf;
			QDataStream in(&in_buf, QIODevice::ReadOnly);
			int byteReceived = 0;
			//Fondamentale per far arrivare tutti i dati al server prima di processare un'operazione
			if (sender->bytesAvailable() < sizeof(int)) {
				sender->waitForReadyRead();
			}

			in_buf = sender->read((qint64)sizeof(int));
			in >> dim;

			while (byteReceived < dim) {
				if (!sender->bytesAvailable()) {
					sender->waitForReadyRead();
				}
				in_buf.append(sender->read((qint64)dim - (qint64)byteReceived));
				byteReceived = in_buf.size() - sizeof(int);
			}
			;
			in >> operation;
			switch (operation)
			{
			//caso per il login
			case 0:
			{
				qDebug() << "Entrato nel case 0";

				QString username, password;
				in >> username >> password;
				bool success = login(username, password, sender);
				//se ho successo ritorno l'elenco di file, altrimenti un messaggio di fail
				/*if(success)
				sendFiles(sender);*/
				break;
			}
			//caso per la registrazione
			case 1:
			{
				qDebug() << "Entrato nel case 1";
				QString username, password, nickname;
				in >> username >> password >> nickname;
				//Gestione caratteri errati dentro l'username
				if (username.contains("/") || username.contains("\\") || username.contains(":") ||
					username.contains("*") || username.contains("?") || username.contains("\"") ||
					username.contains("<") || username.contains(">") || username.contains("|") || username.contains(" "))
				{
					QByteArray buf;
					QDataStream out(&buf, QIODevice::WriteOnly);
					QByteArray bufOut;
					QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

					out << 150;
					out_stream << buf;
					sender->write(bufOut);
					break;
				}
				registration(username, password, nickname, sender);
				break;
			}
			//caso per  l'inserimento o cancellazione di simboli da parte del client
			case 3:
			{
				int insert, numSym;
				QString filename;
				QString creatore;
				QString filePath;
				in >> insert >> filename >> creatore >> numSym;
				QVector<std::shared_ptr<Symbol>> symbolsToSend;

				filePath = creatore + "/" + filename;
				int siteId, counter;
				QVector<int> pos;
				std::shared_ptr<Symbol> newSym;
				if (files.contains(filePath)) {
					auto start = std::chrono::high_resolution_clock::now();
					if (insert == 1)
					{
						qDebug() << "Entrato nel case 3: Insert";
						for (int i = 0; i < numSym; i++) {
							in >> siteId >> counter >> pos;
							newSym = insertSymbol(filePath, sender, &in, siteId, counter, pos);
							if (newSym != nullptr) {
								symbolsToSend.push_back(newSym);
							}
						}
						files.find(filePath).value()->addSymbols(symbolsToSend);
					}
					else
					{
						qDebug() << "Entrato nel case 3: Remove";
						QVector<int> siteIds, counters;
						QVector<QVector<int>> poses;
						for (int i = 0; i < numSym; i++) {
							in >> siteId >> counter >> pos;
							siteIds.push_back(siteId);
							counters.push_back(counter);
							poses.push_back(pos);
							/*newSym = deleteSymbol(filePath, siteId, counter, pos, sender);
							if (newSym != nullptr) {
								symbolsToSend.push_back(newSym);
							}*/
						}
						symbolsToSend = deleteSymbols(filePath, siteIds, counters, poses, sender);
					}
					int siteIdSender = myClient.value()->getSiteId();
					auto finish = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> elapsed = finish - start;
					std::cout << "Elapsed time: " << elapsed.count() << " s\n";
					//mando in out
					qDebug() << "Mando in out";
					for (QTcpSocket* sock : connections.keys())
					{
						if (fileOwnersMap[filePath].contains(connections[sock]->getUsername()) && sock != sender && connections[sock]->getFilename() == filePath)
						{
							if (!symbolsToSend.isEmpty()) {
								sendSymbols(symbolsToSend.size(), symbolsToSend, insert, sock, filePath, siteIdSender); //false per dire che � una cancellazione
							}	
						}
					}
					qDebug() << "Finito";
				}
				break;
			}
			//caso per gestirerichiesta di un file da parte di un client
			case 4:
			{
				qDebug() << "Entrato nel case 4";
				QString filename;
				int siteIdTmp;
				QString creatore;

				if (filename.contains("/") || filename.contains("\\") || filename.contains(":") ||
					filename.contains("*") || filename.contains("?") || filename.contains("\"") ||
					filename.contains("<") || filename.contains(">") || filename.contains("|") || filename.contains(" "))
				{
					QByteArray buf;
					QDataStream out(&buf, QIODevice::WriteOnly);
					QByteArray bufOut;
					QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

					out << 150;
					out_stream << buf;
					sender->write(bufOut);
					break;
				}

				in >> filename >> creatore >> siteIdTmp;

				QString filePath = creatore + "/" + filename;

				sendFile(filename, filePath, sender, siteIdTmp);

				break;
			}
			//Caso per segnalare la disconnessione di un client da un file
			case 5:
			{
				qDebug() << "Entrato nel case 5";
				QString filename;
				QString creatore;
				QString filePath;

				in >> filename >> creatore;

				filePath = creatore + "/" + filename;
				myClient.value()->setFilename("");
				if (files.contains(filePath)) {
					files[filePath]->removeConnection(sender);//rimozione utente dai connessi al file
					for (auto conn : files[filePath]->getConnections())
					{
						sendClient(myClient.value()->getSiteId(), myClient.value()->getNickname(), conn, false);
					}
				}
				//se era l'unico connesso al file bisogna salvarlo in un txt
				saveIfLast(filePath);
				break;
			}
			//caso per mandare l'elenco di file di un utente
			case 6:
			{
				qDebug() << "Entrato nel case 6";

				sendFiles(sender);
				break;
			}
			//caso per gestire la condivisione di ownership di un file
			case 7:
			{
				qDebug() << "Entrato nel case 7";

				/*
				Implemento la share ownership
			*/

				int op;

				in >> op;

				if (op == 1)
				{

					QString uri;
					in >> uri;

					shareOwnership(uri, sender);
				}
				else if (op == 2)
				{

					QString filename;
					QString creatore;
					in >> filename >> creatore;

					QString filePath = creatore + "/" + filename;

					// Condivido l'URI solo se l'utente che me lo chiede ne ha il diritto
					if (fileOwnersMap[filePath].contains(myClient.value()->getUsername()))
					{
						requestURI(filePath, sender);
					}
					else
					{
						/*
						ERRORE
					*/
					}
				}
				else
				{
					/*
					ERRORE
				*/
				}

				break;
			}
			//caso per gestire la cancellazione di un file
			case 9:
			{
				qDebug() << "Entrato nel case 9";

				// Eliminare un file
				QString filename;
				QString username;
				in >> filename >> username;
				eraseFile(filename, username, sender);
				break;
			}
			//caso per il cambio nickname/immagine profilo
			case 10:
			{
				qDebug() << "Entrato nel case 10";

				int op;
				in >> op;
				if (op == 1) {
					QString username;
					QString nickname;
					QImage image;
					in >> username >> nickname >> image;
					changeProfile(username, nickname, image, sender);
				}
				else if (op == 2) {
					QString username;
					QString nickname;
					in >> username >> nickname;
					changeProfile(username, nickname, sender);
				}
				break;
			}
			//caso per la gestione di cambio posizione di cursore di un client
			case 11:
			{
				qDebug() << "Entrato nel case 11";

				int index;
				in >> index;
				cursorPositionChanged(index, myClient.value()->getFilename(), sender);
				break;
			}
			default:
				break;
			}
			sender->flush();
		}
	}
}

/* salvo il file se non ci sono più connessi
*/
void Server::saveIfLast(QString filename)
{
	bool salva = true;
	for (auto client : connections)
	{
		if (client->getFilename() == filename)
		{
			salva = false;
		}
	}
	if (salva)
	{
		if (files.contains(filename))
		{
			saveFile(files[filename]);
		}
	}
}

/* Manda un file richiesto da un client
*/
void Server::sendFile(QString filename, QString filePath, QTcpSocket* socket, int siteId)
{
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

	bool flag = false;

	if (files.contains(filePath))
	{

		TextFile* tf = files[filePath];
		out << 4 /*# operazione*/ << tf->getSymbols().size(); //mando il numero di simboli in arrivo
		//mando tutti i simboli
		for (auto s : tf->getSymbols())
		{
			sendSymbol(s, true, socket, &out);
		}

		int size = tf->getConnections().size();
		//mando tutti gli utenti attualmente connessi
		out << size;
		for (auto conn : tf->getConnections())
		{
			out << connections[conn]->getSiteId() << connections[conn]->getNickname();
		}

		QVector<QString> vect = fileOwnersMap[filePath];
		//mando tutti gli editor di un certo file
		out << vect.size();

		for (QString username : vect)
		{
			out << subs[username]->getSiteId() << subs.find(username).value()->getNickname();
		}
		//mando a tutti i client con lo stesso file aperto un avviso che c'� un nuovo connesso
		for (auto conn : tf->getConnections())
		{
			if (connections.contains(socket)) {
				sendClient(connections[socket]->getSiteId(), connections[socket]->getNickname(), conn, true);
			}
		}
		out_stream << buf;
		socket->write(bufOut);
	}
	else
	{
		//creo un nuovo file
		if (connections.contains(socket))
		{
			TextFile* tf = new TextFile(filename, filePath, socket);
			files.insert(filePath, tf);
			addNewFile(filePath, connections[socket]->getUsername());
		}
	}

	//Controllo se il file era già aperto da qualcuno, nel caso in cui sia il primo ad aprirlo, apro il file di log
	bool open = true;
	for (auto client : connections)
	{
		if (client->getFilename() == filename)
		{
			open = false;
		}
	}
	if (open) {
		files[filePath]->openLogFile();
	}

	//setto il filename dentro la UserConn corrispondente e dentro il campo connection di un file aggiungo la connessione attuale
	if (connections.contains(socket) && !flag)
	{
		files.find(filePath).value()->addConnection(socket);
		connections[socket]->setFilename(filePath);
	}
}

/* Mando un simbolo dell'apertura file (sendFile)
*/
void Server::sendSymbol(std::shared_ptr<class Symbol> symbol, bool insert, QTcpSocket* socket, QDataStream *out)
{
	int ins;
	//if (socket->state() != QAbstractSocket::ConnectedState) return;
	if (insert){
		ins = 1;
	}
	else{
		ins = 0;
	}
	*out << ins;
	*out << symbol->getPosition() << symbol->getCounter() << symbol->getSiteId() << symbol->getValue()
		<< symbol->isBold() << symbol->isItalic() << symbol->isUnderlined() << symbol->getAlignment()
		<< symbol->getTextSize() << symbol->getColor().name() << symbol->getFont();

}

/* Mando i dati del client e segnalo una nuova connessione
*/
void Server::sendClient(int siteId, QString nickname, QTcpSocket* socket, bool insert)
{
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

	out << 8 << siteId << nickname; //8 lo uso come flag per indicare un nuovo connesso
	if (insert)
	{
		out << 1; //deve aggiungere la persona
	}
	else
	{
		out << 0; //deve rimuovere la persona
	}
	out_stream << buf;
	socket->write(bufOut);
	socket->flush();
}

/* Inserisce un simbolo in una certa posizione del file
*/
std::shared_ptr<Symbol> Server::insertSymbol(QString filepath, QTcpSocket* sender, QDataStream* in, int siteId, int counter, QVector<int> pos)
{
	auto tmp = connections.find(sender);
	auto tmpFile = files.find(filepath);
	//controlli
	if (tmp != connections.end() && tmp.value()->getSiteId() == siteId && tmp.value()->getFilename() == filepath && tmpFile != files.end())
	{
		QChar value;
		bool bold, italic, underlined;
		int alignment, textSize;
		QColor color;
		QString font;
		*in >> value >> bold >> italic >> underlined >> alignment >> textSize >> color >> font;
		Symbol sym(pos, counter, siteId, value, bold, italic, underlined, alignment, textSize, color, font);
		std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>(sym);
		//tmpFile.value()->addSymbol(symbol);
		writeLog(filepath, symbol, true);
		return symbol;
	}
}

QVector<std::shared_ptr<Symbol>> Server::deleteSymbols(QString filepath, QVector<int> siteIds, QVector<int> counters, QVector<QVector<int>> poses, QTcpSocket* sender)
{
	QVector<std::shared_ptr<Symbol>> sym;
	//controlli
	if (connections.contains(sender) && files.contains(filepath))
	{
		UserConn* user = connections[sender];
		TextFile* file = files[filepath];
		if (user->getFilename() == filepath) {
			sym = file->removeSymbols(siteIds, counters, poses);
			for (std::shared_ptr<Symbol> s : sym) {
				writeLog(filepath, s, false);
			}
			return sym;
		}
	}
}
/* cancella un simbolo dal file
*/
/*std::shared_ptr<Symbol> Server::deleteSymbol(QString filepath, int siteId, int counter, QVector<int> pos, QTcpSocket* sender)
{
	//auto tmp = connections.find(sender);
	//auto tmpFile = files.find(filename);
	std::shared_ptr<Symbol> sym;
	//controlli
	if (connections.contains(sender) && files.contains(filepath))
	{
		UserConn* user = connections[sender];
		TextFile* file = files[filepath];
		if (user->getFilename() == filepath) {
			//std::shared_ptr<Symbol> sym = tmpFile.value()->getSymbol(siteId, counter);
			sym = file->removeSymbol(siteId, counter, pos);
			//salvo l'operazione nel log
			writeLog(filepath, sym, false);
			return sym;
		}
	}
	return nullptr;
}*/
/* Mando simboli da inserire o cancellare a un certo client
*/
void Server::sendSymbols(int n_sym, QVector<std::shared_ptr<Symbol>> symbols, bool insert, QTcpSocket* socket, QString filename, int siteIdSender)
{
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

	int ins;
	//if (socket->state() != QAbstractSocket::ConnectedState) 		return;
	if (insert){
		ins = 1;
	}
	else{
		ins = 0;
	}
	out << 3 /*numero operazione (inserimento-cancellazione)*/ << ins;
	if (ins == 0) {
		out << siteIdSender; //nel caso di cancellazione ho bisogno di sapere (per i cursori) chi cancella il carattere, non può essere dedotto dal simbolo
	}
	out << n_sym;
	for (int i = 0; i < n_sym; i++)
	{
		out << symbols[i]->getSiteId() << symbols[i]->getCounter() << symbols[i]->getPosition() << symbols[i]->getValue()
			<< symbols[i]->isBold() << symbols[i]->isItalic() << symbols[i]->isUnderlined() << symbols[i]->getAlignment()
			<< symbols[i]->getTextSize() << symbols[i]->getColor().name() << symbols[i]->getFont();
	}
	out_stream << buf;
	socket->write(bufOut);
	socket->flush();
}

/*void Server::changeCredentials(QString username, QString old_password, QString new_password, QString nickname, QTcpSocket* receiver)
{
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

	int flag = 1;
	UserConn* tmp = connections.find(receiver).value();
	if (tmp->getUsername() == username)
	{
		if (old_password == tmp->getPassword())
		{
			//vuole modificare la password
			if (old_password != new_password)
			{
				tmp->setPassword(new_password);
			}
			//vuole modificare il nickname
			if (tmp->getNickname() != nickname)
			{
				tmp->setNickname(nickname);
			}
		}
		else
		{
			flag = 0; //fallita autenticazione
		}
	}
	else
	{
		flag = 0; //fallita autenticazione
	}
	out << flag << -1; //ritorno 0 se fallita, 1 se riuscita
	out_stream << buf;
	receiver->write(bufOut);
	//receiver->flush();
}*/

/* Funzione che riceve nickname e image cambiati dal client e restituisce una risposta se presente un errore, invia le modifiche a tutti i
*  client interessati
*/
void Server::changeProfile(QString username, QString nickname, QImage image, QTcpSocket* sender) {

	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);
	bool nick = false;
	if (subs[username]->getNickname() != nickname) {
		for (User* u : subs.values())
		{
			if (u->getNickname() == nickname)
			{
				nick = true;
				break;
			}
		}
	}
	if (nick)
	{
		out << 10 << 2 << subs[username]->getNickname(); // Nickname già esistente
		out_stream << buf;
		sender->write(bufOut);
		return;
	}
	for (QTcpSocket* sock : connections.keys())
	{
		for (QString file : filesForUser[username])
		{
			if (fileOwnersMap[file][0] == username && filesForUser[connections[sock]->getUsername()].contains(file))
			{
				out << 10 << 1 << subs[username]->getNickname() << nickname;
				out_stream << buf;
				sock->write(bufOut);
				break;
			}
		}
	}

	subs[username]->setNickname(nickname);
	subs[username]->setImage(image);
	rewriteUsersFile();
	if (connections.contains(sender)) {
		connections[sender]->setNickname(nickname);
	}
	QDir d = QDir::current();
	if (!d.exists(username + "/image")) {
		d.mkdir(username + "/image");
	}
	image.save(username + "/image/image.png", "PNG");
}

/* Funzione che riceve nickname e tutti i dati dal client e restituisce una risposta se presente un errore, invia le modifiche a tutti i
*  client interessati
*/
void Server::changeProfile(QString username, QString nickname, QTcpSocket* sender) {
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);
	bool nick = false;
	for (User* u : subs.values())
	{
		if (u->getNickname() == nickname)
		{
			nick = true;
			break;
		}
	}
	if (nick)
	{
		out << 10 << 2 << subs[username]->getNickname(); // Nickname già esistente
		out_stream << buf;
		sender->write(bufOut);
		return;
	}
	for (QTcpSocket* sock : connections.keys())
	{
		for (QString file : filesForUser[username])
		{
			if (fileOwnersMap[file][0] == username && filesForUser[connections[sock]->getUsername()].contains(file))
			{
				out << 10 << 1 << subs[username]->getNickname() << nickname;
				out_stream << buf;
				sock->write(bufOut);
				break;
			}
		}
	}

	subs[username]->setNickname(nickname);
	rewriteUsersFile();

	if (connections.contains(sender)) {
		connections[sender]->setNickname(nickname);
	}

}

/* Funzione responsabile della registrazione
*/
void Server::registration(QString username, QString password, QString nickname, QTcpSocket* sender)
{
	QDir d;
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

	if (!subs.contains(username))
	{
		User* user = new User(username, password, nickname, siteIdCounter++);
		UserConn* conn = new UserConn(username, password, nickname, user->getSiteId(), sender, QString(""));

		/*
			creazione cartella per utente
		*/
		bool nick = false;
		for (User* u : subs.values())
		{
			if (u->getNickname() == nickname)
			{
				nick = true;
				break;
			}
		}
		if (nick)
		{
			out << 1 /*#operazione*/ << 3; //operazione fallita nickname già esistente e termine
		}
		else
		{
			if (d.mkdir(user->getUsername()))
			{
				//  successo
				subs.insert(username, user);
				addNewUserToFile(user);
				connections.insert(sender, conn);
				out << 1 /*#operazione*/ << 1 /*successo*/ << user->getSiteId() << user->getUsername() << user->getNickname(); //operazione riuscita e termine
			}
			else
			{
				out << 1 /*#operazione*/ << 0; //operazione fallita e termine
			}
		}
	}
	else
	{
		out << 1 /*#operazione*/ << 2; //operazione fallita username esistente e termine
	}
	out_stream << buf;
	sender->write(bufOut);
	//sender->flush();
}

/* Mando l'elenco dei file editabili di un certo utente
*/
void Server::sendFiles(QTcpSocket* receiver)
{
	UserConn* conn = connections.find(receiver).value();
	QString username = conn->getUsername();
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

	out << 6; //invio codice operazione

	if (isAuthenticated(receiver))
	{			  // controllare se � loggato
		out << 1; //operazione riuscita
		QVector<QString> tmp = filesForUser[conn->getUsername()];
		//mando siteId
		out << subs.find(username).value()->getSiteId();

		//gestione se non ho file? Questo da nullpointer exception forse
		if (filesForUser.contains(username))
		{
			//mando numero di nomi di file in arrivo
			out << filesForUser[username].size();
			//mando i nomi dei file disponibili
			for (auto filePath : filesForUser[username])
			{

				// out << nome del file << username del creatore << nickname creatore
				out << filePath.split("/")[1] << filePath.split("/")[0] << subs.find(filePath.split("/")[0]).value()->getNickname();
			}
		}
		else
		{
			out << 0; //mando 0, ovvero la quantit� di nomi di file in arrivo
		}
	}
	else
	{
		out << 0; //operazione fallita e fine trasmissione
	}
	out_stream << buf;
	receiver->write(bufOut);
	receiver->flush();
}

/* Funzione per il login di un utente
*/
bool Server::login(QString username, QString password, QTcpSocket* sender)
{
	auto tmp = subs.find(username);
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

	out << 0; //invio codice operazione
	if (tmp != subs.end())
	{
		QString pwd = tmp.value()->getPassword();
		if (pwd == password)
		{ //new branch comment
			UserConn* conn = connections.find(sender).value();
			conn->setUsername(username);
			conn->setPassword(password);
			conn->setNickname(tmp.value()->getNickname());
			conn->setSiteId(tmp.value()->getSiteId());
			if (subs[username]->getHaveImage()) {
				out << 1 << 2 << username << tmp.value()->getNickname() << subs[username]->getImage(); //operazione riuscita e nickname + foto
			}
			else {
				out << 1 << 1 << username << tmp.value()->getNickname(); //operazione riuscita e nickname
			}
			out_stream << buf;
			sender->write(bufOut);
			//sender->flush();
			return true;
		}
		else
		{
			out << 0;
			out_stream << buf;
			sender->write(bufOut);
			//sender->flush();
			return false;
		}
	}
	else
	{
		out << 0;
		out_stream << buf;
		sender->write(bufOut);
		//sender->flush();
		return false;
	}
}
/* Carica dal file subscribers.txt i registrati
*/
void Server::load_subs()
{
	QFile fin("subscribers.txt");
	QString username, password, nickname;
	int siteId;

	std::cout << "Loading subscription...\n";

	if (fin.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream in(&fin);
		while (!in.atEnd())
		{

			QString line = in.readLine();

			username = line.split(" ")[0];
			password = line.split(" ")[1];
			nickname = line.split(" ")[2];
			siteId = line.split(" ")[3].toInt();

			QDir dir;
			if (dir.exists(username + "/image/image.png")) {
				QImage image;
				image.load(username + "/image/image.png");
				User* user = new User(username, password, nickname, siteId, image);
				subs.insert(username, user);
			}
			else {
				User* user = new User(username, password, nickname, siteId);
				subs.insert(username, user);
			}

			siteIdCounter++;
		}
		fin.close();
		std::cout << "Loaded subscription!\n";
	}
	else
		std::cout << "File subscribers.txt non aperto" << std::endl;
}

/* Carica l'elenco dei file con i rispettivi owner e creatori dal file all_files.txt
*/
void Server::load_files()
{
	QString filePath;
	QFile fin("all_files.txt");

	std::cout << "Loading files..\n";

	if (fin.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream in(&fin);
		while (!in.atEnd())
		{ //stile file: nome_file owner1 owner2 ... per ogni riga
			QString line = in.readLine();
			QStringList words = line.split(" ");
			QVector<QString> utenti;
			filePath = words[0];
			/*for (auto str : words) {     DA CANCELLARE--> SE NOME UTENTE = FILENAME, NON FUNZIONA.
				if (str != filename) {
					utenti.append(str);
					filesForUser[str].append(filename);
				}
			}*/
			for (int i = 1; i < words.size(); i++)
			{ //dall'indice 1 in poi ci sono elencati gli utenti che possono vedere il file.
				utenti.append(words[i]);
				filesForUser[words[i]].append(filePath);
			}

			fileOwnersMap.insert(filePath, utenti);
			QString filename = filePath.split("/")[1];
			TextFile* f = new TextFile(filename, filePath);
			load_file(f);
			files.insert(filePath, f);
		}
		fin.close();
	}
	else
		std::cout << "File 'all_files.txt' not opened" << std::endl;

	QString uri;
	QFile fin2("file_uri.txt");

	if (fin2.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream in(&fin2);
		while (!in.atEnd())
		{
			//stile file; nome_file uri_file
			QString line = in.readLine();
			QStringList words = line.split(" ");
			filePath = words[0];
			uri = words[1];
			fileUri.insert(filePath, uri);
		}
		fin2.close();
	}
}

/* Carica in memoria un singolo file
*/
void Server::load_file(TextFile* f)
{
	QString filePath = f->getFilePath();
	QString username = fileOwnersMap[filePath].first();
	QDir d = QDir::current();

	if (!d.exists(username))
	{
		qWarning() << "Impossibile trovare una cartella associata all'utente!";
		d.mkdir(username);
		qWarning() << "Il file potrebbe essere stato rimosso";

		/*
		   Eliminare o evitare l'inserimento nelle strutture

		*/
	}
	else
	{

		int nRows;
		QFile fin(d.filePath(filePath));
		if (fin.open(QIODevice::ReadOnly))
		{
			QTextStream in(&fin);
			in >> nRows;
			for (int i = 0; i < nRows; i++)
			{
				int siteId, counter, pos;
				int bold, italic, underlined, alignment, textSize;
				QString colorName;
				QString font;
				in >> pos >> counter >> siteId;
				QVector<int> vect;
				vect.push_back(pos);
				QChar value;
				in >> value; //salto lo spazio che separa pos da value
				in >> value;
				in >> bold >> italic >> underlined >> alignment >> textSize >> colorName >> font;
				QColor color;
				color.setNamedColor(colorName);
				Symbol sym(vect, counter, siteId, value, bold == 1, italic == 1, underlined == 1, alignment, textSize, color, font);

				//StyleSymbol sym((style == 1), vect, counter, siteId, (bold==1), (italic==1), (underlined==1), alignment, textSize, color, font);
				f->pushBackSymbol(std::make_shared<Symbol>(sym));
			}
			fin.close();
		}
		if (readFromLog(f))
		{
			qDebug() << "Il file " << f->getFilename() << " � stato ripristinato partendo dal log";
		}
	}
}

/*
   Funzione per permette l'accettazione di un invito a collaborare
*/
void Server::shareOwnership(QString uri, QTcpSocket* sender)
{
	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);


	UserConn* tmp = connections.find(sender).value();

	bool flag = false;
	for (QString filePath : fileUri.keys())
	{
		if (fileUri[filePath] == uri)
		{
			if (fileOwnersMap[filePath].contains(tmp->getUsername())) {
				out << 7 << 4; // Il client può già vedere il file, errore.
				out_stream << buf;
				sender->write(bufOut);
				return;
			}
			fileOwnersMap[filePath].append(tmp->getUsername());
			filesForUser[tmp->getUsername()].append(filePath);
			saveAllFilesStatus();
			out << 7 << 1 << files[filePath]->getFilename() << subs[fileOwnersMap[filePath].first()]->getUsername() << subs[fileOwnersMap[filePath].first()]->getNickname(); // File condiviso correttamente, comunico al client che può aggiornare la lista dei file
			flag = true;
			break;
		}
	}

	if (!flag)
	{
		out << 7 << 3; // Uri non esisitente, client visualizzerà errore
	}
	out_stream << buf;
	sender->write(bufOut);
}

/* Funzione per la richiesta di URI
*/
void Server::requestURI(QString filePath, QTcpSocket* sender)
{
	UserConn* tmp = connections.find(sender).value();
	//hashFunction(filename, tmp->getUsername(), tmp->getSiteId());

	QByteArray buf;
	QDataStream out(&buf, QIODevice::WriteOnly);
	QByteArray bufOut;
	QDataStream out_stream(&bufOut, QIODevice::WriteOnly);


	out << 7 << 2; //ripsonde al caso 7 (shareOwnership) operazione 2 richiestaURI

	if (fileUri.contains(filePath))
	{
		out << fileUri[filePath];
	}
	else
	{
		qDebug() << "Errore: impossibile trovare URI corrispondente al nome file"; // l'URI viene creata alla creazione del file, e memorizzata all'interno di fileUri e nel file file_uri.txt
	}
	out_stream << buf;
	sender->write(bufOut);
}

/* Gestione della cancellazione di un file
*/
void Server::eraseFile(QString filename, QString username, QTcpSocket* sender)
{

	/// files -> bio/ciao.txt - utenti connessi
	/// connections -> utenti - file a cui sono connessi

	QString filePath = username + "/" + filename;

	QVector<QString> fileUsers;

	// Elimino il file da fileOwnersMap e da all_file.txt
	if (fileOwnersMap.contains(filePath))
	{
		if (fileOwnersMap[filePath][0] == username)
		{
			// salvo in fileUsers tutti gli utenti che possono modificare il file
			fileUsers = fileOwnersMap[filePath];
			fileOwnersMap.remove(filePath);
			saveAllFilesStatus();
		}
		else
		{
			QByteArray buf;
			QDataStream out(&buf, QIODevice::WriteOnly);
			QByteArray bufOut;
			QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

			out << 9 << 2; // Errore, non è il creatore
			out_stream << buf;
			sender->write(bufOut);
			return;
		}
	}
	else
	{
		QByteArray buf;
		QDataStream out(&buf, QIODevice::WriteOnly);
		QByteArray bufOut;
		QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

		out << 9 << 2; // Errore, file inesistente
		out_stream << buf;
		sender->write(bufOut);
		return;
	}

	// Elimino il file da fileForUsers
	for (QString user : fileUsers)
	{
		if (filesForUser.keys().contains(user))
		{
			if (filesForUser[user].removeOne(filePath))
			{
			}
		}
	}

	// Elimino il file da fileUri e file_uri.txt
	if (fileUri.contains(filePath))
	{
		fileUri.remove(filePath);
		saveURIFileStatus();
	}

	// Faccio eliminare il file a tutti i client connessi che possono vederlo. Elimino poi il file da connections se era aperto nel loro client
	for (QTcpSocket* sock : connections.keys())
	{
		if (fileUsers.contains(connections[sock]->getUsername()))
		{
			QByteArray buf;
			QDataStream out(&buf, QIODevice::WriteOnly);
			QByteArray bufOut;
			QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

			out << 9 << 1 << filename << username; // Gli dico di cancellare il file
			out_stream << buf;
			sock->write(bufOut);

			if (connections.find(sock).value()->getFilename().compare(filePath) == 0) {
				connections.find(sock).value()->setFilename("");
			}
		}
	}

	// Elimino il file da files
	files.remove(filePath);

	/// RIMUOVO IL FILE DALLA CARTELLA
	QFile f(filePath);
	f.remove();
}

/* Costruttore del server, carica registrati e file e si mette in attesa di connessioni
*/
Server::Server(QObject* parent) : QObject(parent)
{
	server = new QTcpServer(this);

	load_subs();

	load_files();

	connect(server, &QTcpServer::newConnection, this, &Server::onNewConnection);
	server->listen(QHostAddress::Any, 49002);
	if (server->isListening())
	{
		std::cout << "Server is listening on port: " << server->serverPort() << std::endl;
	}
}
/* Slot relativa a ogni nuova connessione, setta le connessioni per gestire input successivi e inserisce il client nell'elenco dei connessi
*/
void Server::onNewConnection()
{
	QTcpSocket* socket = server->nextPendingConnection();

	connect(socket, &QTcpSocket::disconnected, this, &Server::onDisconnected);
	connect(socket, &QTcpSocket::readyRead, this, &Server::onReadyRead);

	//addConnection
	UserConn* connection = new UserConn("", "", "", -1, socket, ""); //usr,pwd,nickname,siteId,socket,filename
	connections.insert(socket, connection);							 //mappa client connessi

	std::cout << "# of connected users :\t" << connections.size() << std::endl;
}

/* Aggiunge un utente al file "subscribers.txt" dove sono elencati gli utenti.
*/
void Server::addNewUserToFile(User* user)
{
	QFile file("subscribers.txt");
	if (file.open(QIODevice::WriteOnly | QIODevice::Append))
	{
		QTextStream output(&file);
		QVector<QString> userFiles;
		output << user->getUsername() << " " << user->getPassword() << " " << user->getNickname() << " " << user->getSiteId() << "\n";
		filesForUser.insert(user->getUsername(), userFiles);
	}
	file.close();
}

/*  Riscrive il file dei registrati
*/
void Server::rewriteUsersFile()
{
	QFile file("subscribers.txt");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream output(&file);
		for (User* user : subs.values())
		{
			output << user->getUsername() << " " << user->getPassword() << " " << user->getNickname() << " " << user->getSiteId() << "\n";
		}
	}
	file.close();
}

/* Aggiunge un file con relativo creatore a all_files.txt
*/
void Server::addNewFile(QString filePath, QString user)
{
	QFile file("all_files.txt");

	if (file.open(QIODevice::ReadOnly | QIODevice::Append))
	{

		QTextStream output(&file);
		QVector<QString> utenti;
		QVector<QString> newFiles;
		utenti.append(user);

		output << filePath << " " << user << "\n";
		fileOwnersMap.insert(filePath, utenti);

		if (filesForUser.keys().contains(user))
		{
			filesForUser[user].append(filePath);
		}
		else
		{
			newFiles.append(filePath);
			filesForUser.insert(user, newFiles);
		}
	}

	QFile file2("file_uri.txt");
	if (file2.open(QIODevice::ReadOnly | QIODevice::Append))
	{

		QTextStream output(&file2);
		QString rand = genRandom();
		while (fileUri.values().contains(rand))
		{
			rand = genRandom();
		}
		output << filePath << " " << rand << "\n";
		fileUri.insert(filePath, rand);
	}

	file.close();
	file2.close();
}

/* Controlla se il client è autenticato
*/
bool Server::isAuthenticated(QTcpSocket* socket)
{
	UserConn* conn = connections.find(socket).value();
	if (conn->getUsername() != "")
	{
		return true;
	}
	else
	{
		return false;
	}
}

/* Scrive su all_files.txt tutti i file con i rispettivi utenti (per primo il creatore)
*/
void Server::saveAllFilesStatus()
{

	QFile file("all_files.txt");
	if (file.open(QIODevice::WriteOnly))
	{

		QTextStream output(&file);
		for (QString filePath : fileOwnersMap.keys())
		{
			output << filePath;
			for (QString utente : fileOwnersMap[filePath])
			{

				output << " " << utente;
			}
			output << "\n";
		}
	}

	file.close();
}

/* Scrive su file_uri.txt nome file e uri per ogni file
*/
void Server::saveURIFileStatus()
{

	QFile file("file_uri.txt");
	if (file.open(QIODevice::WriteOnly))
	{

		QTextStream output(&file);
		for (QString filePath : fileUri.keys())
		{
			output << filePath << " " << fileUri[filePath] << "\n";
		}
	}

	file.close();
}

/* Scrive un log per ogni file nella cartella apposita dell'utente, con le informazioni su ogni cancellazione e inserimento
*  pronto per essere usato come restore in caso di shutdown improvviso del server
*/
void Server::writeLog(QString filePath, std::shared_ptr<Symbol> s, bool insert)
{

	QString filename = filePath.split("/")[1];
	QString userFolder = filePath.split("/")[0];

	QString fileLogName = filename.split(".")[0] + "_log.txt";

	QDir d = QDir::current();

	if (!d.exists(userFolder))
	{
		qWarning() << "Impossibile trovare una cartella associata all'utente!"
			<< "\n"
			<< "Creazione cartella";
		/*
				TODO
				crea cartello o mando messaggio di errore?

		*/
	}

	//QFile file(d.filePath(userFolder + "/" + fileLogName));

	//if (file.open(QIODevice::WriteOnly | QIODevice::Append))
	//{

		QTextStream stream(files[filePath]->getLogFile());
		//QTextStream stream(&file);
		stream << endl;
		if (insert)
		{
			stream << 1;
		}
		else
		{
			stream << 0;
		}
		stream << " " << s->getPosition().size() << " ";
		for (int valuePos : s->getPosition())
		{
			stream << valuePos << " ";
		}

		stream << s->getCounter() << " " << s->getSiteId() << " " << s->getValue() << " ";
		if (s->isBold())
		{
			stream << 1 << " ";
		}
		else
		{
			stream << 0 << " ";
		}
		if (s->isItalic())
		{
			stream << 1 << " ";
		}
		else
		{
			stream << 0 << " ";
		}
		if (s->isUnderlined())
		{
			stream << 1 << " ";
		}
		else
		{
			stream << 0 << " ";
		}
		stream << s->getAlignment() << " " << s->getTextSize() << " " << s->getColor().name() << " " << QString::fromStdString(s->getFont().toStdString());

		//file.close();
	//}
}

/* Funzione che legge dal log in caso di shutdown e ricostruisce un file con i dati non salvati nel txt originale
*/
bool Server::readFromLog(TextFile* f)
{
	/*
	   selezionare la cartella corrente

	*/

	QString fileLogName = f->getFilePath().split("/")[0] + "/" + f->getFilePath().split("/")[1].split(".")[0] + "_log.txt";
	QFile fin(fileLogName);
	if (fin.open(QIODevice::ReadOnly))
	{
		QTextStream in(&fin);
		while (!in.atEnd())
		{
			int insert, sizeVect;
			int siteId, counter, pos;
			int bold, italic, underlined, alignment, textSize;
			QString colorName;
			QString font;
			QVector<int> vect;
			in >> insert >> sizeVect;
			for (int i = 0; i < sizeVect; i++)
			{
				in >> pos;
				vect.push_back(pos);
			}

			in >> counter >> siteId;
			QChar value;
			in >> value; //salto lo spazio che separa pos da value
			in >> value;
			in >> bold >> italic >> underlined >> alignment >> textSize >> colorName >> font;
			QColor color;
			color.setNamedColor(colorName);
			Symbol sym(vect, counter, siteId, value, bold == 1, italic == 1, underlined == 1, alignment, textSize, color, font);

			if (insert == 1) {
				QVector<std::shared_ptr<Symbol>> syms;
				syms.append(std::make_shared<Symbol>(sym));
				f->addSymbols(syms);
			}
			else if(insert == 0) {
				QVector<int> siteIds;
				QVector<int> counters;
				QVector<QVector<int>> vects;
				siteIds.append(siteId);
				counters.append(counter);
				vects.append(vect);
				f->removeSymbols(siteIds, counters, vects);
			}
		}
		fin.close();
		//saveFile(f); //il log viene rimosso nell saveFile
		return true;
	}
	else
	{
		return false;
	}
}

/* Cancella il log di un determinato file
*/
void Server::deleteLog(TextFile* f)
{
	f->closeLogFile();
}


/* Genera una stringa randomica per l'URI
*/
QString Server::genRandom()
{ // Random string generator function.
    
	QVector<char> caratteri = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r',
		's','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','.',',',':',';','_','-','!','?',
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R',
		'S','T','U','V','W','X','Y','Z'
	};
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution(0, 69);
	char randChar;
	QString s;

	for (int i = 0; i < 32; i++)
	{
		randChar = caratteri[distribution(generator)];		
		s.append(randChar);
	}

	return s;
}

/*La posizione del cursore del sender è cambiata, quindi bisogna avvisare tutti gli
* altri client che stanno utilizzando a quel file (recivers)*/
void Server::cursorPositionChanged(int index, QString filename, QTcpSocket* sender)
{
	if (!isAuthenticated(sender))
	{
		return;
	}
	if (files.contains(filename)) {
		TextFile* tf = files[filename];
		if (connections.contains(sender)) {
			int siteIdSender = connections[sender]->getSiteId();
			for (auto reciver : tf->getConnections())
			{
				if (reciver != sender && connections[reciver]->getFilename() == filename)
				{
					QByteArray buf;
					QDataStream out(&buf, QIODevice::WriteOnly);
					QByteArray bufOut;
					QDataStream out_stream(&bufOut, QIODevice::WriteOnly);

					out << 11 << filename << index << siteIdSender;
					out_stream << buf;
					reciver->write(bufOut);
					reciver->flush();
				}
			}
		}
	}

	return;
}
