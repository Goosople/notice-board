# Notice Board — C++ (Qt6)

Native desktop implementation using C++17 and Qt6. Runs on KDE Wayland.

## Run / Install

```
# Build & run from source:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/notice_board

# Install to system:
./install.sh                  # ~/.local/bin
./install.sh --kiosk          # also install kiosk launcher
./install.sh /usr/local       # custom prefix
```

## Source layout

```
src/
├── main.cpp           Entry point, signal handling, first-run wizard
├── config.h           Config struct, JSON load/save, password hashing
├── noticeboard.h/cpp  Main fullscreen window, closeEvent lock
├── admindialog.h/cpp  Password-protected admin panel
└── ntfylistener.h/cpp Background ntfy poller (QNetworkAccessManager)
```
