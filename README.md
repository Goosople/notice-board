# Notice Board

A simple, fullscreen notice board for displaying announcements. Designed for **KDE Wayland** with optional kiosk lock via [cage](https://github.com/cage-kiosk/cage).

Two implementations are provided — pick whichever you prefer:

| | Language | Dependencies |
|---|---|---|
| `cpp/` | C++17 + Qt6 | cmake, g++, Qt6 Widgets/Network |
| `py/` | Python 3 | PyQt6, requests |

## Features

- **Fullscreen notice display** — shows a single notice, always on top
- **Remote control via ntfy** — change the notice from anywhere by publishing to an ntfy topic
- **One-touch help call** — user presses a key (default F1) to send a predefined message to the admin
- **Admin panel** (`Ctrl+Shift+A`) — password-protected, edit notice, configure settings, exit app
- **Sleep inhibition** — prevents the screen from sleeping or locking
- **Screen lock** — blocks Escape and close events; combine with [cage](https://github.com/cage-kiosk/cage) for true kiosk mode (no Alt+Tab, no compositor shortcuts)

## Quick start

### C++

```
cd cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/notice_board
```

### Python

```
cd py
python3 notice_board.py
```

### Kiosk mode (cage)

```
./cpp/run_kiosk.sh     # C++ version
./py/run_kiosk.sh      # Python version
```

## First run

The first-run wizard asks you to set an **admin password** and configure ntfy topics and the notice text. After that the board launches fullscreen.

## ntfy integration

The board polls an ntfy **subscribe topic** for notice updates. When the user presses the help key, a message is published to the ntfy **publish topic**.

Example — push a notice remotely:

```
curl -d "Meeting in room 3 at 2pm" https://ntfy.sh/mynoticeboard
```

## Install

### C++

```
cd cpp
./install.sh                  # installs to ~/.local/bin
./install.sh --kiosk          # also installs cage kiosk launcher
./install.sh /usr/local       # custom prefix
./install.sh --no-desktop     # skip .desktop file
```

### Python

```
cd py
./install.sh                  # same options as above
```

## Config

Stored at `~/.config/notice_board/config.json`. Delete this file to reset the admin password.
