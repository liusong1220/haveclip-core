#include <QBuffer>
#include <QImage>
#include <QDomDocument>
#include <QTextCodec>
#include <QDateTime>
#include <QMessageBox>

#include "Sender.h"

Sender::Sender(HaveClip::Encryption enc, HaveClip::Node *node, QObject *parent) :
	QSslSocket(parent),
	m_node(node),
	encryption(enc)
{
	connect(this, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
	connect(this, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
	connect(this, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslError(QList<QSslError>)));
}

void Sender::distribute(const ClipboardContent *content, QString password)
{
	/**
	  XML protocol
	  <haveclip>
		<password>1234</password>
		<clipboard>
			<mimedata mimetype="text/plain">base64 encoded data</mimedata>
			...
		</clipboard>
	  </haveclip>
	  */

	this->content = content;
	this->password = password;

	if(encryption != HaveClip::None)
	{
		setPeerVerifyMode(QSslSocket::VerifyNone);

		connect(this, SIGNAL(encrypted()), this, SLOT(onConnect()));

		switch(encryption)
		{
		case HaveClip::Ssl:
			setProtocol(QSsl::SslV3);
			break;
		case HaveClip::Tls:
			setProtocol(QSsl::TlsV1);
			break;
		}

		connectToHostEncrypted(m_node->host, m_node->port);

	} else {
		connect(this, SIGNAL(connected()), this, SLOT(onConnect()));

		connectToHost(m_node->host, m_node->port);
	}

	/**
	  We cannot write data immediately due to a bug in Qt which will cause application to end up
	  in infinite loop when connection fails.
	  */
}

void Sender::onError(QAbstractSocket::SocketError socketError)
{
	qDebug() << "Unable to reach" << m_node->host << ":" << socketError;
	this->deleteLater();
}

void Sender::onConnect()
{
	QDomDocument doc;
	QDomElement root = doc.createElement("haveclip");
	doc.appendChild(root);

	QDomElement passEl = doc.createElement("password");
	QDomText pass = doc.createTextNode(password);

	passEl.appendChild(pass);
	root.appendChild(passEl);

	QDomElement clip = doc.createElement("clipboard");
	root.appendChild(clip);

	foreach(QString mimetype, content->mimeData->formats())
	{
		QDomElement mimedata = doc.createElement("mimedata");
		mimedata.setAttribute("mimetype", mimetype);

		QDomText text;
		QByteArray data;

		if(mimetype == "text/html")
		{
			QByteArray tmp = content->mimeData->data("text/html");

			QTextCodec *codec = QTextCodec::codecForHtml(tmp, QTextCodec::codecForName("utf-8"));
			data = codec->toUnicode(tmp).toUtf8();
		} else
			data = content->mimeData->data(mimetype);

//		qDebug() << mimetype << data;

		text = doc.createTextNode(data.toBase64());
		mimedata.appendChild(text);

		clip.appendChild(mimedata);
	}

	QByteArray ba = doc.toByteArray();

	qDebug() << "Distributing" << ba.size() << "bytes";
	write(ba);

	disconnectFromHost();
}

void Sender::onDisconnect()
{
	this->deleteLater();
}

void Sender::onSslError(const QList<QSslError> &errors)
{
	QList<QSslError::SslError> recoverable;
	recoverable << QSslError::SelfSignedCertificate
		<< QSslError::CertificateUntrusted
		<< QSslError::HostNameMismatch;

	bool exception = true;

	foreach(QSslError e, errors)
	{
		if(!recoverable.contains(e.error()))
		{
			qDebug() << "Unrecoverable SSL error" << e;
			emit sslFatalError(errors);
			return;
		}

		if(e.certificate() != m_node->certificate)
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

HaveClip::Node* Sender::node()
{
	return m_node;
}