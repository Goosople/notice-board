#pragma once

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QString>

#include <QCryptographicHash>

#include <optional>

struct Config {
    QString ntfyServer   = "https://ntfy.sh";
    QString subscribeTopic;
    QString publishTopic;
    QString adminPasswordHash;
    QString helpMessage  = "Help requested.";
    QString helpKey      = "F1";
    QString noticeText;
    int pollInterval      = 15;
    int fontSize          = 48;
    QString fontFamily    = "Sans Serif";
    QString bgColor       = "#1a1a2e";
    QString fgColor       = "#e0e0e0";
    QString cageMode      = "extend";
    int ntfyPriority      = 3;
    QString ntfyClick;
    QString ntfyTags;

    static QString configPath() {
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
               + "/.config/notice_board/config.json";
    }

    static Config load() {
        Config c;
        QFile f(configPath());
        if (!f.open(QIODevice::ReadOnly))
            return c;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        QJsonObject o = doc.object();
        if (o.contains("ntfy_server"))        c.ntfyServer   = o["ntfy_server"].toString();
        if (o.contains("subscribe_topic"))    c.subscribeTopic = o["subscribe_topic"].toString();
        if (o.contains("publish_topic"))      c.publishTopic = o["publish_topic"].toString();
        if (o.contains("admin_password_hash")) c.adminPasswordHash = o["admin_password_hash"].toString();
        if (o.contains("help_message"))       c.helpMessage  = o["help_message"].toString();
        if (o.contains("help_key"))           c.helpKey      = o["help_key"].toString();
        if (o.contains("notice_text"))        c.noticeText   = o["notice_text"].toString();
        if (o.contains("poll_interval"))      c.pollInterval  = o["poll_interval"].toInt(15);
        if (o.contains("font_size"))          c.fontSize      = o["font_size"].toInt(48);
        if (o.contains("font_family"))        c.fontFamily   = o["font_family"].toString();
        if (o.contains("bg_color"))           c.bgColor      = o["bg_color"].toString();
        if (o.contains("fg_color"))           c.fgColor      = o["fg_color"].toString();
        if (o.contains("cage_mode"))          c.cageMode     = o["cage_mode"].toString();
        if (o.contains("ntfy_priority"))      c.ntfyPriority = o["ntfy_priority"].toInt(3);
        if (o.contains("ntfy_click"))         c.ntfyClick    = o["ntfy_click"].toString();
        if (o.contains("ntfy_tags"))          c.ntfyTags     = o["ntfy_tags"].toString();
        return c;
    }

    void save() const {
        QDir().mkpath(QFileInfo(configPath()).absolutePath());
        QFile f(configPath());
        if (!f.open(QIODevice::WriteOnly))
            return;
        QJsonObject o;
        o["ntfy_server"]         = ntfyServer;
        o["subscribe_topic"]     = subscribeTopic;
        o["publish_topic"]       = publishTopic;
        o["admin_password_hash"] = adminPasswordHash;
        o["help_message"]        = helpMessage;
        o["help_key"]            = helpKey;
        o["notice_text"]         = noticeText;
        o["poll_interval"]       = pollInterval;
        o["font_size"]           = fontSize;
        o["font_family"]         = fontFamily;
        o["bg_color"]            = bgColor;
        o["fg_color"]            = fgColor;
        o["cage_mode"]           = cageMode;
        o["ntfy_priority"]       = ntfyPriority;
        o["ntfy_click"]          = ntfyClick;
        o["ntfy_tags"]           = ntfyTags;
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    }

    static QString hash(const QString &pw) {
        return QString::fromUtf8(
            QCryptographicHash::hash(pw.toUtf8(), QCryptographicHash::Sha256).toHex()
        );
    }
};
