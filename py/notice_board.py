#!/usr/bin/env python3
"""
Notice Board — Fullscreen notice display with remote control via ntfy.
Runs on KDE Wayland; prevents sleep and locks the display.
"""

import sys
import os
import json
import hashlib
import subprocess
import signal
import time
import threading
from pathlib import Path

import requests
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QObject, QThread
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QLabel, QWidget, QVBoxLayout,
    QDialog, QLineEdit, QTextEdit, QPushButton, QFormLayout,
    QMessageBox, QHBoxLayout, QGroupBox, QSpinBox, QComboBox,
    QDialogButtonBox
)
from PyQt6.QtGui import QFont, QKeySequence, QShortcut, QColor

# ---------------------------------------------------------------------------
# Paths & defaults
# ---------------------------------------------------------------------------
APP_NAME = "NoticeBoard"
CONFIG_DIR = Path.home() / ".config" / "notice_board"
CONFIG_FILE = CONFIG_DIR / "config.json"

DEFAULT_CONFIG = {
    "ntfy_server": "https://ntfy.sh",
    "subscribe_topic": "",
    "publish_topic": "",
    "admin_password_hash": "",
    "help_message": "Help requested.",
    "help_key": "F1",
    "notice_text": "",
    "poll_interval": 15,
    "font_size": 48,
    "font_family": "Sans Serif",
    "bg_color": "#1a1a2e",
    "fg_color": "#e0e0e0"
}

KEY_NAMES = [
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "Space", "Enter", "Return", "Escape"
]

KEY_MAP = {
    "F1": Qt.Key.Key_F1, "F2": Qt.Key.Key_F2, "F3": Qt.Key.Key_F3,
    "F4": Qt.Key.Key_F4, "F5": Qt.Key.Key_F5, "F6": Qt.Key.Key_F6,
    "F7": Qt.Key.Key_F7, "F8": Qt.Key.Key_F8, "F9": Qt.Key.Key_F9,
    "F10": Qt.Key.Key_F10, "F11": Qt.Key.Key_F11, "F12": Qt.Key.Key_F12,
    "Space": Qt.Key.Key_Space, "Enter": Qt.Key.Key_Enter,
    "Return": Qt.Key.Key_Return, "Escape": Qt.Key.Key_Escape,
}

# ---------------------------------------------------------------------------
# Config helpers
# ---------------------------------------------------------------------------
def load_config():
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    if CONFIG_FILE.exists():
        try:
            with open(CONFIG_FILE) as f:
                cfg = json.load(f)
        except (json.JSONDecodeError, OSError):
            cfg = {}
        for k, v in DEFAULT_CONFIG.items():
            cfg.setdefault(k, v)
        return cfg
    return dict(DEFAULT_CONFIG)


def save_config(cfg):
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    with open(CONFIG_FILE, "w") as f:
        json.dump(cfg, f, indent=2)


def hash_password(pw):
    return hashlib.sha256(pw.encode()).hexdigest()


# ===================================================================
# Ntfy listener (runs in background thread)
# ===================================================================
class NtfyListener(QObject):
    notice_received = pyqtSignal(str)
    connection_changed = pyqtSignal(bool)

    def __init__(self, config):
        super().__init__()
        self.config = config
        self._running = False
        self._last_id = None

    def run(self):
        self._running = True
        while self._running:
            try:
                topic = self.config["subscribe_topic"].strip()
                if not topic:
                    time.sleep(self.config.get("poll_interval", 15))
                    continue

                server = self.config["ntfy_server"].rstrip("/")
                poll = self.config.get("poll_interval", 15)
                url = f"{server}/{topic}/json?poll={poll}"
                if self._last_id:
                    url += f"&since={self._last_id}"

                resp = requests.get(url, timeout=max(30, poll * 2 + 5))
                self.connection_changed.emit(True)

                if resp.status_code == 200:
                    messages = resp.json()
                    if isinstance(messages, list) and messages:
                        latest = messages[-1]
                        text = latest.get("message", "")
                        self._last_id = latest.get("id", self._last_id)
                        if text and text.strip():
                            self.notice_received.emit(text)
            except requests.RequestException:
                self.connection_changed.emit(False)
                time.sleep(5)
            except Exception:
                time.sleep(5)

    def stop(self):
        self._running = False


class NtfyManager:
    """Runs NtfyListener on a QThread."""

    def __init__(self, config):
        self.listener = NtfyListener(config)
        self._thread = QThread()
        self.listener.moveToThread(self._thread)
        self._thread.started.connect(self.listener.run)

    def start(self):
        self._thread.start()

    def stop(self):
        self.listener.stop()
        self._thread.quit()
        self._thread.wait(200)

    @staticmethod
    def send(server, topic, message, title="Notice Board"):
        if not topic or not topic.strip():
            return False
        try:
            url = f"{server.rstrip('/')}/{topic}"
            requests.post(
                url,
                data=message.encode("utf-8"),
                headers={"Title": title},
                timeout=10,
            )
            return True
        except Exception:
            return False


# ===================================================================
# Sleep inhibitor
# ===================================================================
class SleepInhibitor:
    def __init__(self):
        self._proc = None

    def start(self):
        try:
            self._proc = subprocess.Popen(
                [
                    "systemd-inhibit",
                    "--what=idle:sleep:handle-power-key:handle-suspend-key:handle-lid-switch",
                    "--who=NoticeBoard",
                    "--why=Notice board is active",
                    "--mode=block",
                    "sleep",
                    "infinity",
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
        except Exception:
            pass

    def stop(self):
        if self._proc:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self._proc.kill()
            self._proc = None


# ===================================================================
# First-run dialog
# ===================================================================
class FirstRunDialog(QDialog):
    def __init__(self, config, parent=None):
        super().__init__(parent)
        self.config = config
        self.setWindowTitle("Notice Board — First Run Setup")
        self.setMinimumWidth(420)
        layout = QVBoxLayout(self)

        info = QLabel(
            "Welcome to Notice Board!\n\n"
            "Configure the basic settings below. "
            "You can change everything later from the admin panel.\n"
            "(Press Ctrl+Shift+A to open the admin panel at any time.)"
        )
        info.setWordWrap(True)
        layout.addWidget(info)

        form = QFormLayout()

        self.pw_input = QLineEdit()
        self.pw_input.setEchoMode(QLineEdit.EchoMode.Password)
        self.pw_input.setPlaceholderText("Choose a password")
        form.addRow("Admin Password:", self.pw_input)

        self.pw_confirm = QLineEdit()
        self.pw_confirm.setEchoMode(QLineEdit.EchoMode.Password)
        self.pw_confirm.setPlaceholderText("Confirm password")
        form.addRow("Confirm:", self.pw_confirm)

        self.topic_input = QLineEdit(self.config.get("subscribe_topic", ""))
        form.addRow("ntfy Topic (receive):", self.topic_input)

        self.pub_topic_input = QLineEdit(self.config.get("publish_topic", ""))
        form.addRow("ntfy Topic (send):", self.pub_topic_input)

        self.server_input = QLineEdit(self.config.get("ntfy_server", ""))
        form.addRow("ntfy Server URL:", self.server_input)

        self.help_msg_input = QLineEdit(self.config.get("help_message", ""))
        form.addRow("Help Message:", self.help_msg_input)

        self.key_combo = QComboBox()
        self.key_combo.addItems(KEY_NAMES)
        idx = self.key_combo.findText(self.config.get("help_key", "F1"))
        if idx >= 0:
            self.key_combo.setCurrentIndex(idx)
        form.addRow("Help Key:", self.key_combo)

        self.notice_input = QLineEdit(self.config.get("notice_text", ""))
        self.notice_input.setPlaceholderText("e.g. Temporarily away. Press F1 for help.")
        form.addRow("Notice Text:", self.notice_input)

        layout.addLayout(form)

        btn_box = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        btn_box.accepted.connect(self._on_accept)
        btn_box.rejected.connect(self.reject)
        layout.addWidget(btn_box)

    def _on_accept(self):
        pw = self.pw_input.text()
        if not pw:
            QMessageBox.warning(self, "Error", "Password cannot be empty.")
            return
        if pw != self.pw_confirm.text():
            QMessageBox.warning(self, "Error", "Passwords do not match.")
            return
        self.config["admin_password_hash"] = hash_password(pw)
        self.config["subscribe_topic"] = self.topic_input.text().strip()
        self.config["publish_topic"] = self.pub_topic_input.text().strip()
        self.config["ntfy_server"] = self.server_input.text().strip()
        self.config["help_message"] = self.help_msg_input.text().strip()
        self.config["help_key"] = self.key_combo.currentText()
        self.config["notice_text"] = self.notice_input.text().strip()
        save_config(self.config)
        self.accept()


# ===================================================================
# Admin dialog
# ===================================================================
class AdminDialog(QDialog):
    def __init__(self, config, parent=None):
        super().__init__(parent)
        self.config = config
        self.wants_exit = False
        self._lock = not bool(config.get("admin_password_hash"))
        self.setWindowTitle("Admin Panel — Notice Board")
        self.setMinimumWidth(540)
        self._build()

    def _build(self):
        layout = QVBoxLayout(self)
        self._auth_widget = QWidget()
        self._panel_widget = QWidget()

        # -- Auth layer --
        auth_layout = QVBoxLayout(self._auth_widget)
        auth_layout.addWidget(QLabel("Enter admin password to unlock:"))
        pw_row = QHBoxLayout()
        self._pw_input = QLineEdit()
        self._pw_input.setEchoMode(QLineEdit.EchoMode.Password)
        self._pw_input.setPlaceholderText("Password")
        pw_row.addWidget(self._pw_input)
        unlock_btn = QPushButton("Unlock")
        unlock_btn.clicked.connect(self._try_unlock)
        pw_row.addWidget(unlock_btn)
        auth_layout.addLayout(pw_row)
        layout.addWidget(self._auth_widget)

        # -- Panel layer (hidden until unlocked) --
        panel = QVBoxLayout(self._panel_widget)

        # Notice
        ng = QGroupBox("Notice Text")
        ngl = QVBoxLayout(ng)
        self._notice_edit = QTextEdit()
        self._notice_edit.setPlainText(self.config.get("notice_text", ""))
        self._notice_edit.setMinimumHeight(120)
        ngl.addWidget(self._notice_edit)
        apply_btn = QPushButton("Apply Notice Now")
        apply_btn.clicked.connect(self._apply_notice)
        ngl.addWidget(apply_btn)
        panel.addWidget(ng)

        # Settings
        sg = QGroupBox("Settings")
        sf = QFormLayout(sg)
        self._server = QLineEdit(self.config.get("ntfy_server", ""))
        sf.addRow("ntfy Server:", self._server)
        self._sub_topic = QLineEdit(self.config.get("subscribe_topic", ""))
        sf.addRow("Subscribe Topic:", self._sub_topic)
        self._pub_topic = QLineEdit(self.config.get("publish_topic", ""))
        sf.addRow("Publish Topic:", self._pub_topic)
        self._help_msg = QLineEdit(self.config.get("help_message", ""))
        sf.addRow("Help Message:", self._help_msg)

        self._help_key = QComboBox()
        self._help_key.addItems(KEY_NAMES)
        idx = self._help_key.findText(self.config.get("help_key", "F1"))
        if idx >= 0:
            self._help_key.setCurrentIndex(idx)
        sf.addRow("Help Key:", self._help_key)

        self._font_size = QSpinBox()
        self._font_size.setRange(12, 200)
        self._font_size.setValue(self.config.get("font_size", 48))
        sf.addRow("Font Size:", self._font_size)
        panel.addWidget(sg)

        # Password change
        pg = QGroupBox("Change Password")
        pf = QFormLayout(pg)
        self._new_pw = QLineEdit()
        self._new_pw.setEchoMode(QLineEdit.EchoMode.Password)
        pf.addRow("New Password:", self._new_pw)
        cpw_btn = QPushButton("Change Password")
        cpw_btn.clicked.connect(self._change_password)
        pf.addRow(cpw_btn)
        panel.addWidget(pg)

        # Bottom buttons
        bb = QHBoxLayout()
        save_btn = QPushButton("Save All Settings")
        save_btn.clicked.connect(self._save_all)
        bb.addWidget(save_btn)
        bb.addStretch()
        exit_btn = QPushButton("Exit Application")
        exit_btn.setStyleSheet("QPushButton { color: #c0392b; font-weight: bold; }")
        exit_btn.clicked.connect(self._request_exit)
        bb.addWidget(exit_btn)
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.accept)
        bb.addWidget(close_btn)
        panel.addLayout(bb)

        layout.addWidget(self._panel_widget)
        self._panel_widget.hide()

        # Unlock immediately if no password set (first run)
        if self._lock:
            self._try_unlock_first_run()

    def _try_unlock_first_run(self):
        """If no password was ever set, use an empty check to trigger setup."""
        if not self.config.get("admin_password_hash"):
            pw = self._pw_input.text()
            if pw:
                self.config["admin_password_hash"] = hash_password(pw)
                save_config(self.config)
                QMessageBox.information(self, "Set", "Admin password has been set.")
                self._show_panel()
            # else: wait for user to type and click Unlock

    def _try_unlock(self):
        pw = self._pw_input.text()
        stored = self.config.get("admin_password_hash", "")
        if not stored:
            # First-time setup
            if not pw:
                QMessageBox.warning(self, "Error", "Enter a password to set.")
                return
            self.config["admin_password_hash"] = hash_password(pw)
            save_config(self.config)
            QMessageBox.information(self, "Password Set", "Admin password has been set.")
        elif hash_password(pw) != stored:
            QMessageBox.warning(self, "Access Denied", "Incorrect password.")
            return
        self._show_panel()

    def _show_panel(self):
        self._auth_widget.hide()
        self._panel_widget.show()

    def _apply_notice(self):
        self.config["notice_text"] = self._notice_edit.toPlainText()
        save_config(self.config)
        p = self.parent()
        if p and hasattr(p, "update_notice"):
            p.update_notice(self.config["notice_text"])
        QMessageBox.information(self, "Applied", "Notice updated on screen.")

    def _save_all(self):
        self.config["ntfy_server"] = self._server.text().strip()
        self.config["subscribe_topic"] = self._sub_topic.text().strip()
        self.config["publish_topic"] = self._pub_topic.text().strip()
        self.config["help_message"] = self._help_msg.text().strip()
        self.config["help_key"] = self._help_key.currentText()
        self.config["font_size"] = self._font_size.value()
        save_config(self.config)
        QMessageBox.information(self, "Saved", "Settings saved. Some take effect on restart.")

    def _change_password(self):
        new_pw = self._new_pw.text()
        if not new_pw:
            QMessageBox.warning(self, "Error", "Password cannot be empty.")
            return
        self.config["admin_password_hash"] = hash_password(new_pw)
        save_config(self.config)
        QMessageBox.information(self, "Changed", "Password updated.")
        self._new_pw.clear()

    def _request_exit(self):
        reply = QMessageBox.question(
            self, "Exit Application",
            "Are you sure you want to exit the notice board?\n"
            "The application will close and the user will see the desktop.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if reply == QMessageBox.StandardButton.Yes:
            self.wants_exit = True
            self.accept()


# ===================================================================
# Main notice board window
# ===================================================================
class NoticeBoard(QMainWindow):
    def __init__(self, config):
        super().__init__()
        self.config = config
        self._help_sc = None
        self._was_online = False
        self._exit_requested = False

        self._init_ui()
        self._init_ntfy()
        self._init_shortcuts()
        self._sleep = SleepInhibitor()
        self._sleep.start()

    # -- UI -----------------------------------------------------------------
    def _init_ui(self):
        self.setWindowTitle(APP_NAME)
        self.setWindowFlags(
            Qt.WindowType.FramelessWindowHint
            | Qt.WindowType.WindowStaysOnTopHint
        )

        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.setContentsMargins(40, 40, 40, 40)

        self._notice_label = QLabel()
        self._notice_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._notice_label.setWordWrap(True)
        self._notice_label.setTextFormat(Qt.TextFormat.PlainText)
        layout.addWidget(self._notice_label, stretch=1)

        self._status_label = QLabel()
        self._status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._status_label.setFixedHeight(30)
        layout.addWidget(self._status_label)

        self._apply_style()
        self.update_notice(self.config.get("notice_text", ""))

    def _apply_style(self):
        ff = self.config.get("font_family", "Sans Serif")
        fs = self.config.get("font_size", 48)
        self._notice_label.setFont(QFont(ff, fs))

        bg = self.config.get("bg_color", "#1a1a2e")
        fg = self.config.get("fg_color", "#e0e0e0")
        self.setStyleSheet(
            f"QMainWindow {{ background-color: {bg}; }}"
            f"QWidget {{ background-color: {bg}; color: {fg}; }}"
        )
        self._status_label.setStyleSheet("font-size: 13px; color: #888;")

    # -- ntfy ---------------------------------------------------------------
    def _init_ntfy(self):
        self._ntfy = NtfyManager(self.config)
        self._ntfy.listener.notice_received.connect(self._on_notice)
        self._ntfy.listener.connection_changed.connect(self._on_conn)
        self._ntfy.start()

    def _on_notice(self, text):
        self.config["notice_text"] = text
        save_config(self.config)
        self.update_notice(text)

    def _on_conn(self, ok):
        if ok:
            self._was_online = True
            self._status_label.setText("")
        elif self._was_online:
            self._status_label.setText("Offline — waiting for connection…")

    # -- Shortcuts ----------------------------------------------------------
    def _init_shortcuts(self):
        # Admin panel: Ctrl+Shift+A
        sc = QShortcut(QKeySequence("Ctrl+Shift+A"), self)
        sc.activated.connect(self._open_admin)

        # Help key
        self._rebind_help_key()

    def _rebind_help_key(self):
        if self._help_sc:
            self._help_sc.setEnabled(False)
            self._help_sc.deleteLater()
            self._help_sc = None

        hk = self.config.get("help_key", "F1")
        qt_key = KEY_MAP.get(hk, Qt.Key.Key_F1)
        self._help_sc = QShortcut(QKeySequence(qt_key), self)
        self._help_sc.activated.connect(self._send_help)

    def _open_admin(self):
        dlg = AdminDialog(self.config, self)
        dlg.exec()
        if dlg.wants_exit:
            self._exit_requested = True
            self.close()
            return
        self.config = load_config()
        self._apply_style()
        self.update_notice(self.config.get("notice_text", ""))
        self._rebind_help_key()

    def _send_help(self):
        msg = self.config.get("help_message", "Help requested.")
        ok = NtfyManager.send(
            self.config["ntfy_server"],
            self.config["publish_topic"],
            msg,
        )
        if ok:
            self._status_label.setText("Help request sent")
            QTimer.singleShot(4000, lambda: self._status_label.setText(""))
        else:
            self._status_label.setText("Failed to send help request")
            QTimer.singleShot(6000, lambda: self._status_label.setText(""))

    # -- Public API ---------------------------------------------------------
    def update_notice(self, text):
        self._notice_label.setText(text)

    # -- Lock-down overrides ------------------------------------------------
    def closeEvent(self, event):
        if self._exit_requested:
            event.accept()
            QApplication.instance().quit()
        else:
            event.ignore()

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            return
        super().keyPressEvent(event)

    def shutdown(self):
        if self._ntfy:
            self._ntfy.stop()
        self._sleep.stop()


# ===================================================================
# Entry point
# ===================================================================
def main():
    if "WAYLAND_DISPLAY" in os.environ:
        os.environ.setdefault("QT_QPA_PLATFORM", "wayland")

    app = QApplication(sys.argv)
    app.setApplicationName(APP_NAME)
    app.setQuitOnLastWindowClosed(False)

    config = load_config()

    if not config.get("admin_password_hash"):
        wizard = FirstRunDialog(config)
        if wizard.exec() != QDialog.DialogCode.Accepted:
            sys.exit(0)
        config = load_config()

    window = NoticeBoard(config)

    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda *_: (window.shutdown(), app.quit()))

    window.showFullScreen()

    try:
        ret = app.exec()
    finally:
        window.shutdown()
    sys.exit(ret)


if __name__ == "__main__":
    main()
