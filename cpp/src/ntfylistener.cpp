#include "ntfylistener.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>

namespace NtfySender {

bool send(const QString &server, const QString &topic, const QString &message,
          const QString &title, int priority, const QString &click, const QString &tags) {
    if (topic.trimmed().isEmpty())
        return false;
    QString s = server;
    if (s.endsWith('/')) s.chop(1);
    QUrl url(s + "/" + topic);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    req.setRawHeader("Title", title.toUtf8());
    if (priority > 0)
        req.setRawHeader("Priority", QByteArray::number(priority));
    if (!click.isEmpty())
        req.setRawHeader("Click", click.toUtf8());
    if (!tags.isEmpty())
        req.setRawHeader("Tags", tags.toUtf8());

    QNetworkAccessManager nam;
    auto *reply = nam.post(req, message.toUtf8());

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool ok = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    return ok;
}

}
