#include "admindialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QKeySequence>

static const char *KEY_NAMES[] = {
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "Space", "Enter", "Return", "Escape"
};

AdminDialog::AdminDialog(Config &cfg, QWidget *parent)
    : QDialog(parent), m_config(cfg)
{
    setWindowTitle("Admin Panel — Notice Board");
    setMinimumWidth(540);
    buildUi();
}

void AdminDialog::buildUi() {
    auto *root = new QVBoxLayout(this);
    m_stack = new QStackedWidget;

    // ---- Auth page ----
    m_authPage = new QWidget;
    auto *al = new QVBoxLayout(m_authPage);
    al->addWidget(new QLabel("Enter admin password to unlock:"));
    auto *pwRow = new QHBoxLayout;
    m_pwInput = new QLineEdit;
    m_pwInput->setEchoMode(QLineEdit::Password);
    m_pwInput->setPlaceholderText("Password");
    pwRow->addWidget(m_pwInput);
    auto *unlockBtn = new QPushButton("Unlock");
    connect(unlockBtn, &QPushButton::clicked, this, &AdminDialog::tryUnlock);
    connect(m_pwInput, &QLineEdit::returnPressed, this, &AdminDialog::tryUnlock);
    pwRow->addWidget(unlockBtn);
    al->addLayout(pwRow);
    al->addStretch();
    m_stack->addWidget(m_authPage);

    // ---- Panel page ----
    m_panelPage = new QWidget;
    auto *pl = new QVBoxLayout(m_panelPage);

    // Notice group
    auto *ng = new QGroupBox("Notice Text");
    auto *ngl = new QVBoxLayout(ng);
    m_noticeEdit = new QTextEdit;
    m_noticeEdit->setPlainText(m_config.noticeText);
    m_noticeEdit->setMinimumHeight(120);
    ngl->addWidget(m_noticeEdit);
    auto *applyBtn = new QPushButton("Apply Notice Now");
    connect(applyBtn, &QPushButton::clicked, this, &AdminDialog::applyNotice);
    ngl->addWidget(applyBtn);
    pl->addWidget(ng);

    // Settings group
    auto *sg = new QGroupBox("Settings");
    auto *sf = new QFormLayout(sg);
    m_serverEdit = new QLineEdit(m_config.ntfyServer);
    sf->addRow("ntfy Server:", m_serverEdit);
    m_subTopicEdit = new QLineEdit(m_config.subscribeTopic);
    sf->addRow("Subscribe Topic:", m_subTopicEdit);
    m_pubTopicEdit = new QLineEdit(m_config.publishTopic);
    sf->addRow("Publish Topic:", m_pubTopicEdit);
    m_helpMsgEdit = new QLineEdit(m_config.helpMessage);
    sf->addRow("Help Message:", m_helpMsgEdit);
    m_helpKeyCombo = new QComboBox;
    for (auto k : KEY_NAMES) m_helpKeyCombo->addItem(k);
    int idx = m_helpKeyCombo->findText(m_config.helpKey);
    if (idx >= 0) m_helpKeyCombo->setCurrentIndex(idx);
    sf->addRow("Help Key:", m_helpKeyCombo);
    m_fontSizeSpin = new QSpinBox;
    m_fontSizeSpin->setRange(12, 200);
    m_fontSizeSpin->setValue(m_config.fontSize);
    sf->addRow("Font Size:", m_fontSizeSpin);
    pl->addWidget(sg);

    // Password change
    auto *pg = new QGroupBox("Change Password");
    auto *pf = new QFormLayout(pg);
    m_newPwEdit = new QLineEdit;
    m_newPwEdit->setEchoMode(QLineEdit::Password);
    pf->addRow("New Password:", m_newPwEdit);
    auto *cpwBtn = new QPushButton("Change Password");
    connect(cpwBtn, &QPushButton::clicked, this, &AdminDialog::changePassword);
    pf->addRow(cpwBtn);
    pl->addWidget(pg);

    // Bottom buttons
    auto *bb = new QHBoxLayout;
    auto *saveBtn = new QPushButton("Save All Settings");
    connect(saveBtn, &QPushButton::clicked, this, &AdminDialog::saveSettings);
    bb->addWidget(saveBtn);
    bb->addStretch();
    auto *exitBtn = new QPushButton("Exit Application");
    exitBtn->setStyleSheet("QPushButton { color: #c0392b; font-weight: bold; }");
    connect(exitBtn, &QPushButton::clicked, this, &AdminDialog::requestExit);
    bb->addWidget(exitBtn);
    auto *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bb->addWidget(closeBtn);
    pl->addLayout(bb);

    m_stack->addWidget(m_panelPage);
    m_stack->setCurrentIndex(0);
    root->addWidget(m_stack);
}

void AdminDialog::tryUnlock() {
    QString pw = m_pwInput->text();
    QString stored = m_config.adminPasswordHash;

    if (stored.isEmpty()) {
        if (pw.isEmpty()) {
            QMessageBox::warning(this, "Error", "Enter a password to set.");
            return;
        }
        m_config.adminPasswordHash = Config::hash(pw);
        m_config.save();
        QMessageBox::information(this, "Set", "Admin password has been set.");
        showPanel();
        return;
    }

    if (Config::hash(pw) == stored) {
        showPanel();
    } else {
        QMessageBox::warning(this, "Access Denied", "Incorrect password.");
        m_pwInput->clear();
    }
}

void AdminDialog::showPanel() {
    m_stack->setCurrentIndex(1);
}

void AdminDialog::applyNotice() {
    m_config.noticeText = m_noticeEdit->toPlainText();
    m_config.save();
    QMessageBox::information(this, "Applied", "Notice updated on screen.");
}

void AdminDialog::saveSettings() {
    m_config.ntfyServer    = m_serverEdit->text().trimmed();
    m_config.subscribeTopic = m_subTopicEdit->text().trimmed();
    m_config.publishTopic   = m_pubTopicEdit->text().trimmed();
    m_config.helpMessage    = m_helpMsgEdit->text().trimmed();
    m_config.helpKey        = m_helpKeyCombo->currentText();
    m_config.fontSize       = m_fontSizeSpin->value();
    m_config.save();
    QMessageBox::information(this, "Saved", "Settings saved. Some take effect on restart.");
}

void AdminDialog::changePassword() {
    QString pw = m_newPwEdit->text();
    if (pw.isEmpty()) {
        QMessageBox::warning(this, "Error", "Password cannot be empty.");
        return;
    }
    m_config.adminPasswordHash = Config::hash(pw);
    m_config.save();
    QMessageBox::information(this, "Changed", "Password updated.");
    m_newPwEdit->clear();
}

void AdminDialog::requestExit() {
    auto reply = QMessageBox::question(
        this, "Exit Application",
        "Are you sure you want to exit the notice board?\n"
        "The application will close and the user will see the desktop.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        emit exitRequested();
        accept();
    }
}
