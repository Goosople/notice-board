#include <QApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QLabel>
#include <QVBoxLayout>

#include <csignal>
#include <unistd.h>

#include "config.h"
#include "noticeboard.h"

static void handleSignal(int) {
    ::_exit(0);
}

static bool firstRunWizard(Config &cfg) {
    QDialog dlg;
    dlg.setWindowTitle("Notice Board — First Run Setup");
    dlg.setMinimumWidth(420);
    auto *layout = new QVBoxLayout(&dlg);

    auto *info = new QLabel(
        "Welcome to Notice Board!\n\n"
        "Configure the basic settings below. "
        "You can change everything later from the admin panel.\n"
        "(Press Ctrl+Shift+A to open the admin panel at any time.)"
    );
    info->setWordWrap(true);
    layout->addWidget(info);

    auto *form = new QFormLayout;

    auto *pw = new QLineEdit;
    pw->setEchoMode(QLineEdit::Password);
    pw->setPlaceholderText("Choose a password");
    form->addRow("Admin Password:", pw);

    auto *pwConfirm = new QLineEdit;
    pwConfirm->setEchoMode(QLineEdit::Password);
    pwConfirm->setPlaceholderText("Confirm password");
    form->addRow("Confirm:", pwConfirm);

    auto *topic = new QLineEdit(cfg.subscribeTopic);
    form->addRow("ntfy Topic (receive):", topic);

    auto *pubTopic = new QLineEdit(cfg.publishTopic);
    form->addRow("ntfy Topic (send):", pubTopic);

    auto *server = new QLineEdit(cfg.ntfyServer);
    form->addRow("ntfy Server URL:", server);

    auto *helpMsg = new QLineEdit(cfg.helpMessage);
    form->addRow("Help Message:", helpMsg);

    auto *helpKey = new QComboBox;
    const char *keys[] = {"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
                          "Space","Enter","Return","Escape"};
    for (auto k : keys) helpKey->addItem(k);
    helpKey->setCurrentText(cfg.helpKey);
    form->addRow("Help Key:", helpKey);

    auto *cageMode = new QComboBox;
    cageMode->addItems({"extend", "last"});
    cageMode->setEditable(true);
    cageMode->setCurrentText(cfg.cageMode);
    form->addRow("Cage Display Mode:", cageMode);

    auto *priority = new QSpinBox;
    priority->setRange(1, 5);
    priority->setValue(cfg.ntfyPriority);
    form->addRow("ntfy Priority (1-5):", priority);

    auto *clickUrl = new QLineEdit(cfg.ntfyClick);
    clickUrl->setPlaceholderText("URL opened when help notification is clicked");
    form->addRow("ntfy Click URL:", clickUrl);

    auto *tags = new QLineEdit(cfg.ntfyTags);
    tags->setPlaceholderText("e.g. +1,loudspeaker");
    form->addRow("ntfy Tags:", tags);

    auto *notice = new QLineEdit(cfg.noticeText);
    notice->setPlaceholderText("e.g. Temporarily away. Press F1 for help.");
    form->addRow("Notice Text:", notice);

    layout->addLayout(form);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, [&]() {
        if (pw->text().isEmpty()) {
            QMessageBox::warning(&dlg, "Error", "Password cannot be empty.");
            return;
        }
        if (pw->text() != pwConfirm->text()) {
            QMessageBox::warning(&dlg, "Error", "Passwords do not match.");
            return;
        }
        cfg.adminPasswordHash = Config::hash(pw->text());
        cfg.subscribeTopic = topic->text().trimmed();
        cfg.publishTopic   = pubTopic->text().trimmed();
        cfg.ntfyServer     = server->text().trimmed();
        cfg.helpMessage    = helpMsg->text().trimmed();
        cfg.helpKey        = helpKey->currentText();
        cfg.cageMode       = cageMode->currentText().trimmed();
        cfg.ntfyPriority   = priority->value();
        cfg.ntfyClick      = clickUrl->text().trimmed();
        cfg.ntfyTags       = tags->text().trimmed();
        cfg.noticeText     = notice->text().trimmed();
        cfg.save();
        dlg.accept();
    });
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(btns);

    return dlg.exec() == QDialog::Accepted;
}

int main(int argc, char *argv[]) {
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
        qputenv("QT_QPA_PLATFORM", "wayland");

    QApplication app(argc, argv);
    app.setApplicationName("NoticeBoard");
    app.setQuitOnLastWindowClosed(false);

    signal(SIGINT,  handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGHUP,  handleSignal);

    Config cfg = Config::load();

    if (cfg.adminPasswordHash.isEmpty()) {
        if (!firstRunWizard(cfg))
            return 0;
        cfg = Config::load();
    }

    NoticeBoard window(cfg);
    window.showFullScreen();

    return app.exec();
}
