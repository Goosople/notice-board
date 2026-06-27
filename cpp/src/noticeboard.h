#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QProcess>
#include <QShortcut>
#include <QTimer>

#include "config.h"
#include "ntfylistener.h"

class NoticeBoard : public QMainWindow {
    Q_OBJECT
public:
    explicit NoticeBoard(Config &cfg, QWidget *parent = nullptr);
    ~NoticeBoard();

    void updateNotice(const QString &text);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNoticeReceived(const QString &text);
    void onConnectionChanged(bool connected);
    void openAdmin();
    void sendHelp();

private:
    void setupUi();
    void applyStyle();
    void startNtfy();
    void startSleepInhibit();
    void rebindHelpKey();

    Config &m_config;
    QLabel *m_noticeLabel;
    QLabel *m_statusLabel;
    NtfyListener *m_ntfy;
    QShortcut *m_helpSc = nullptr;
    QProcess *m_sleepInhibit = nullptr;
    bool m_wasOnline = false;
    bool m_exitRequested = false;
};
