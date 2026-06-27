#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QEventLoop>

#include "config.h"

class NtfyListener : public QObject {
    Q_OBJECT
public:
    explicit NtfyListener(const Config &cfg, QObject *parent = nullptr);

    void start();
    void stop();

    static bool send(const QString &server, const QString &topic, const QString &message,
                     const QString &title = "Notice Board", int priority = 0,
                     const QString &click = "", const QString &tags = "");

signals:
    void noticeReceived(const QString &text);
    void connectionChanged(bool connected);

private slots:
    void poll();
    void onReply(QNetworkReply *reply);

private:
    Config m_config;
    QNetworkAccessManager *m_nam;
    QTimer *m_timer;
    QString m_lastId;
    bool m_running = false;
};
