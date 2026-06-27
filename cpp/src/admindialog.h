#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QProcess>

#include "config.h"

class AdminDialog : public QDialog {
    Q_OBJECT
public:
    explicit AdminDialog(Config &cfg, QWidget *parent = nullptr);

signals:
    void exitRequested();

private slots:
    void tryUnlock();
    void tryFingerprint();
    void applyNotice();
    void saveSettings();
    void changePassword();
    void requestExit();

private:
    void buildUi();
    void showPanel();

    Config &m_config;
    bool m_hasFprintd = false;

    QStackedWidget *m_stack;
    QWidget *m_authPage;
    QWidget *m_panelPage;

    QLineEdit *m_pwInput;
    QPushButton *m_fpBtn = nullptr;

    QTextEdit *m_noticeEdit;
    QLineEdit *m_serverEdit;
    QLineEdit *m_pubTopicEdit;
    QLineEdit *m_helpMsgEdit;
    QComboBox *m_helpKeyCombo;
    QSpinBox *m_fontSizeSpin;
    QComboBox *m_cageModeCombo;
    QSpinBox *m_prioritySpin;
    QLineEdit *m_clickEdit;
    QLineEdit *m_tagsEdit;
    QLineEdit *m_newPwEdit;
};
