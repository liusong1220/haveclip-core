/*
  HaveClip

  Copyright (C) 2013 Jakub Skokan <aither@havefun.cz>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QImage>
#include <QTextCodec>
#include <QDateTime>
#include <QDataStream>

#include "Sender.h"
#include "Conversations/ClipboardUpdate.h"

Sender::Sender(ConnectionManager::Encryption enc, Node *node, QObject *parent) :
	Communicator(parent),
	m_node(node)
{
	encryption = enc;
}

void Sender::distribute(ClipboardItem *content)
{
	m_conversation = new Conversations::ClipboardUpdate(Communicator::Send, content, this);

	connectToPeer();
}

void Sender::onError(QAbstractSocket::SocketError socketError)
{
	qDebug() << "Unable to reach" << m_node->host() << ":" << socketError;
	this->deleteLater();
}

void Sender::onSslError(const QList<QSslError> &errors)
{
	QList<QSslError::SslError> recoverable;
	recoverable << QSslError::SelfSignedCertificate
		<< QSslError::CertificateUntrusted
		<< QSslError::HostNameMismatch
		<< QSslError::CertificateExpired;

	bool exception = true;

	foreach(QSslError e, errors)
	{
		if(!recoverable.contains(e.error()))
		{
			qDebug() << "Unrecoverable SSL error" << e;
			emit sslFatalError(errors);
			return;
		}

		if(e.certificate() != m_node->certificate())
		{
			exception = false;
			break;
		}
	}

	if(exception)
	{
		qDebug() << "SSL errors ignored because of exception";
		ignoreSslErrors();

	} else {
		emit untrustedCertificateError(m_node, errors);
	}
}

Node* Sender::node()
{
	return m_node;
}

void Sender::connectToPeer()
{
	conversationSignals();

	if(encryption != ConnectionManager::None)
	{
		setPeerVerifyMode(QSslSocket::VerifyNone);

		connect(this, SIGNAL(encrypted()), this, SLOT(onConnect()));

		switch(encryption)
		{
		case ConnectionManager::Ssl:
			setProtocol(QSsl::SslV3);
			break;
		case ConnectionManager::Tls:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
			setProtocol(QSsl::TlsV1_0);
#else
			setProtocol(QSsl::TlsV1);
#endif
			break;
		}

		connectToHostEncrypted(m_node->host(), m_node->port());

	} else {
		connect(this, SIGNAL(connected()), this, SLOT(onConnect()));

		connectToHost(m_node->host(), m_node->port());
	}

	/**
	  We cannot write data immediately due to a bug in Qt which will cause application to end up
	  in infinite loop when connection fails.
	  */
}
