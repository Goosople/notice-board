#include "noticeboard.h"
#include "admindialog.h"

#include <QVBoxLayout>
#include <QFont>
#include <QKeyEvent>
#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>

NoticeBoard::NoticeBoard(Config &cfg, QWidget *parent)
    : QMainWindow(parent), m_config(cfg)
{
    setupUi();
    startNtfy();
    startSleepInhibit();
}

NoticeBoard::~NoticeBoard() {
    if (m_ntfy) m_ntfy->stop();
    if (m_sleepInhibit) {
        m_sleepInhibit->terminate();
        m_sleepInhibit->waitForFinished(3000);
    }
}

// -- UI ----------------------------------------------------------------------
void NoticeBoard::setupUi() {
    setWindowTitle("Notice Board");
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    auto *central = new QWidget;
    setCentralWidget(central);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(40, 40, 40, 40);

    m_noticeLabel = new QLabel;
    m_noticeLabel->setAlignment(Qt::AlignCenter);
    m_noticeLabel->setWordWrap(true);
    m_noticeLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_noticeLabel, 1);

    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedHeight(30);
    m_statusLabel->setStyleSheet("font-size: 13px; color: #888;");
    layout->addWidget(m_statusLabel);

    applyStyle();
    updateNotice(m_config.noticeText);

    // Shortcuts
    auto *adminSc = new QShortcut(QKeySequence("Ctrl+Shift+A"), this);
    connect(adminSc, &QShortcut::activated, this, &NoticeBoard::openAdmin);

    rebindHelpKey();
}

void NoticeBoard::applyStyle() {
    m_noticeLabel->setFont(QFont(m_config.fontFamily, m_config.fontSize));
    QString bg = m_config.bgColor, fg = m_config.fgColor;
    setStyleSheet(
        QString("QMainWindow { background-color: %1; } QWidget { background-color: %1; color: %2; }")
            .arg(bg, fg)
    );
}

// -- ntfy --------------------------------------------------------------------
void NoticeBoard::startNtfy() {
    m_ntfy = new NtfyListener(m_config, this);
    connect(m_ntfy, &NtfyListener::noticeReceived, this, &NoticeBoard::onNoticeReceived);
    connect(m_ntfy, &NtfyListener::connectionChanged, this, &NoticeBoard::onConnectionChanged);
    m_ntfy->start();
}

void NoticeBoard::onNoticeReceived(const QString &text) {
    m_config.noticeText = text;
    m_config.save();
    updateNotice(text);
}

void NoticeBoard::onConnectionChanged(bool ok) {
    if (ok) {
        m_wasOnline = true;
        m_statusLabel->setText("");
    } else if (m_wasOnline) {
        m_statusLabel->setText("Offline — waiting for connection…");
    }
}

// -- Sleep inhibit -----------------------------------------------------------
void NoticeBoard::startSleepInhibit() {
    m_sleepInhibit = new QProcess(this);
    m_sleepInhibit->start("systemd-inhibit", {
        "--what=idle:sleep:handle-power-key:handle-suspend-key:handle-lid-switch",
        "--who=NoticeBoard",
        "--why=Notice board is active",
        "--mode=block",
        "sleep", "infinity"
    });
}

// -- Shortcuts ---------------------------------------------------------------
void NoticeBoard::rebindHelpKey() {
    if (m_helpSc) {
        m_helpSc->setEnabled(false);
        delete m_helpSc;
        m_helpSc = nullptr;
    }
    QString hk = m_config.helpKey;
    int key = Qt::Key_F1;
    if (hk == "F1") key = Qt::Key_F1;        else if (hk == "F2") key = Qt::Key_F2;
    else if (hk == "F3") key = Qt::Key_F3;   else if (hk == "F4") key = Qt::Key_F4;
    else if (hk == "F5") key = Qt::Key_F5;   else if (hk == "F6") key = Qt::Key_F6;
    else if (hk == "F7") key = Qt::Key_F7;   else if (hk == "F8") key = Qt::Key_F8;
    else if (hk == "F9") key = Qt::Key_F9;   else if (hk == "F10") key = Qt::Key_F10;
    else if (hk == "F11") key = Qt::Key_F11; else if (hk == "F12") key = Qt::Key_F12;
    else if (hk == "Space") key = Qt::Key_Space;
    else if (hk == "Enter") key = Qt::Key_Enter;
    else if (hk == "Return") key = Qt::Key_Return;
    else if (hk == "Escape") key = Qt::Key_Escape;

    m_helpSc = new QShortcut(QKeySequence(key), this);
    connect(m_helpSc, &QShortcut::activated, this, &NoticeBoard::sendHelp);
}

// -- Slots -------------------------------------------------------------------
void NoticeBoard::openAdmin() {
    AdminDialog dlg(m_config, this);
    connect(&dlg, &AdminDialog::exitRequested, this, [this]() {
        m_exitRequested = true;
    });
    dlg.exec();
    if (m_exitRequested) {
        close();
        return;
    }
    applyStyle();
    updateNotice(m_config.noticeText);
    rebindHelpKey();
}

void NoticeBoard::sendHelp() {
    bool ok = NtfyListener::send(m_config.ntfyServer, m_config.publishTopic,
                                 m_config.helpMessage);
    m_statusLabel->setText(ok ? "Help request sent" : "Failed to send");
    QTimer::singleShot(4000, this, [this]() {
        m_statusLabel->setText("");
    });
}

void NoticeBoard::updateNotice(const QString &text) {
    m_noticeLabel->setText(text);
}

// -- Lock-down ---------------------------------------------------------------
void NoticeBoard::closeEvent(QCloseEvent *event) {
    if (m_exitRequested) {
        event->accept();
        QCoreApplication::exit(0);
    } else {
        event->ignore();
    }
}

void NoticeBoard::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape)
        return;
    QMainWindow::keyPressEvent(event);
}
