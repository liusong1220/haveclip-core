#include <QTextDocument>
#include <QUrl>
#include <QStringList>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>
#include <QImage>
#include <QPixmap>
#include <QDebug>

#include "ClipboardContent.h"

ClipboardContent::Preview::~Preview()
{
	QFile::remove(path);
}

ClipboardContent::ClipboardContent(QMimeData *data) :
	preview(0),
	mimeData(data)
{
	foreach(QString f, mimeData->formats())
	{
		if(f.indexOf('/') != -1)
			formats << f;
	}
}

void ClipboardContent::init()
{
	QString tmp;

	if(mimeData->hasText()) {
		tmp = mimeData->text();

#ifdef Q_OS_LINUX
		excerpt = Qt::escape(tmp.left(200));

		if(tmp.count() > 200)
			excerpt += "<br>...";
#endif

		setTitle(tmp);

	} else if(mimeData->hasHtml()) {
		tmp = mimeData->html();

#ifdef Q_OS_LINUX
		excerpt = Qt::escape(tmp.left(200));

		if(tmp.count() > 200)
			excerpt += "<br>...";
#endif

		setTitle(tmp);

	} else if(mimeData->hasUrls()) {
		QList<QUrl> urls = mimeData->urls();

#ifdef Q_OS_LINUX
		foreach(QUrl u, urls.mid(0, 5))
			excerpt += u.toString() + "<br>";

		if(urls.size() > 5)
			excerpt += "<br>...";
#endif

		if(urls.size() > 0)
			tmp = urls.first().toString();
		else tmp = QObject::tr("URLs");

		setTitle(tmp);

	} else if(mimeData->hasImage()) {
		// For some reason, QMimeData::imageData().value<QImage>() does not work on copied QMimeData
		// Loaded image is null
		QImage img;
		img.loadFromData(mimeData->data("application/x-qt-image"));

		excerpt = QObject::tr("Image");
		setTitle(excerpt);
		icon = QIcon( QPixmap::fromImage(img) );

#ifdef Q_OS_LINUX
		preview = createItemPreview(img);

		if(preview)
		{
			QString prop;

			if(preview->width > 400)
				prop = QString("width=\"%1\"").arg(400);
			else if(preview->height > 400)
				prop = QString("height=\"%1\"").arg(400);

			excerpt += QString("<br><img src=\"%1\" %2>").arg(preview->path).arg(prop);
		}
#endif
	} else {
		tmp = QObject::tr("Unknown content");
		setTitle(tmp);
	}
}

ClipboardContent::~ClipboardContent()
{
	if(preview)
		delete preview;

	delete mimeData;
}

QString ClipboardContent::toPlainText()
{
	QString ret;

	if(mimeData->hasText())
		ret = mimeData->text();

	else if(mimeData->hasHtml())
		ret = mimeData->html();

	else if(mimeData->hasUrls()) {
		foreach(QUrl u, mimeData->urls())
			ret += u.toString() + "\n";
	}

	return ret;
}

bool ClipboardContent::operator==(const ClipboardContent &other) const
{
	if(formats != other.formats)
		return false;

	foreach(QString f, formats)
	{
		if(mimeData->data(f) != other.mimeData->data(f))
			return false;
	}

	return true;
}

ClipboardContent::Preview* ClipboardContent::createItemPreview(QImage &img)
{
	Preview *preview = 0;
	QTemporaryFile tmp(QDir::tempPath() + "/haveclip-preview-XXXXXX");
	tmp.setAutoRemove(false);

	if(tmp.open())
	{
		if( img.save(&tmp, "PNG") )
		{
			preview = new Preview;
			preview->path = tmp.fileName();
			preview->width = img.width();
			preview->height = img.height();
		}

		tmp.close();
	}

	return preview;
}

void ClipboardContent::setTitle(QString &str)
{
	QString s = str.trimmed(), tmp;
	int start = 0, end;

	while((end = s.indexOf('\n', start)) != -1)
	{
		tmp = s.mid(start, end).trimmed().left(30);

		if(!tmp.isEmpty())
		{
			title = tmp;
			break;
		}

		start = end + 1;
	}

	if(title.isEmpty())
		title = s.left(30);

	title.replace("&", "&&");

	if(str.count() > 30)
		title += "...";
}