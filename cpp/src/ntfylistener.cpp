#include "ntfylistener.h"
#include "config.h"

NtfyListener::NtfyListener(const Config &cfg, QObject *parent)
    : QObject(parent), m_config(cfg)
{
    m_nam = new QNetworkAccessManager(this);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &NtfyListener::poll);
    connect(m_nam, &QNetworkAccessManager::finished, this, &NtfyListener::onReply);
}

void NtfyListener::start() {
    m_running = true;
    m_timer->start(1000); // fire immediately, then poll sets interval
    poll();
}

void NtfyListener::stop() {
    m_running = false;
    m_timer->stop();
}

void NtfyListener::poll() {
    if (!m_running) return;
    if (m_config.subscribeTopic.trimmed().isEmpty()) {
        m_timer->start(m_config.pollInterval * 1000);
        return;
    }

    QString server = m_config.ntfyServer;
    if (server.endsWith('/')) server.chop(1);
    QString url = server + "/" + m_config.subscribeTopic + "/json?poll="
                  + QString::number(m_config.pollInterval);
    if (!m_lastId.isEmpty())
        url += "&since=" + m_lastId;

    QNetworkRequest req{QUrl(url)};
    req.setRawHeader("User-Agent", "NoticeBoard/1.0");
    m_nam->get(req);
}

void NtfyListener::onReply(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit connectionChanged(false);
        QTimer::singleShot(5000, this, &NtfyListener::poll);
        return;
    }

    emit connectionChanged(true);
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        m_timer->start(m_config.pollInterval * 1000);
        return;
    }

    QJsonArray arr = doc.array();
    if (arr.isEmpty()) {
        m_timer->start(m_config.pollInterval * 1000);
        return;
    }

    QJsonObject latest = arr.last().toObject();
    QString text = latest.value("message").toString();
    if (latest.contains("id"))
        m_lastId = latest["id"].toVariant().toString();

    if (!text.trimmed().isEmpty())
        emit noticeReceived(text);

    m_timer->start(m_config.pollInterval * 1000);
}

bool NtfyListener::send(const QString &server, const QString &topic,
                        const QString &message, const QString &title) {
    if (topic.trimmed().isEmpty())
        return false;
    QString s = server;
    if (s.endsWith('/')) s.chop(1);
    QUrl url(s + "/" + topic);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    req.setRawHeader("Title", title.toUtf8());

    QNetworkAccessManager nam;
    auto *reply = nam.post(req, message.toUtf8());

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool ok = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    return ok;
}
