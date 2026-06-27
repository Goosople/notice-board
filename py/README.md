# Notice Board — Python (PyQt6)

Python implementation using PyQt6. Same features as the C++ version.

## Run / Install

```
# Run from source:
python3 notice_board.py

# Install to system:
./install.sh                  # ~/.local/bin
./install.sh --kiosk          # also install kiosk launcher
./install.sh /usr/local       # custom prefix
```

Requirements: `PyQt6`, `requests` (`pip install PyQt6 requests`).

## Kiosk mode

```
./run_kiosk.sh
```

Requires [cage](https://github.com/cage-kiosk/cage) (Wayland kiosk compositor).

## Autostart

```
cp notice_board.desktop ~/.config/autostart/
# or for kiosk mode:
cp notice_board_kiosk.desktop ~/.config/autostart/
```
