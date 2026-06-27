#!/bin/bash
# Install Notice Board (C++) to a prefix directory.
#
# Usage:
#   ./install.sh                  # install to ~/.local (default)
#   ./install.sh /usr/local       # install to custom prefix
#   ./install.sh --kiosk          # install kiosk desktop entry
#   ./install.sh --no-desktop     # skip desktop file

set -e

PREFIX="${HOME}/.local"
INSTALL_KIOSK=0
INSTALL_DESKTOP=1

for arg in "$@"; do
    case "$arg" in
        --kiosk)       INSTALL_KIOSK=1 ;;
        --no-desktop)  INSTALL_DESKTOP=0 ;;
        /*|~*)         PREFIX="$arg" ;;
        *)             echo "Unknown option: $arg"; exit 1 ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Build
echo "==> Building..."
cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX"
cmake --build "$SCRIPT_DIR/build" -j"$(nproc)"

# Install binary
echo "==> Installing to $PREFIX/bin ..."
cmake --install "$SCRIPT_DIR/build"

# Desktop file
if [ "$INSTALL_DESKTOP" = 1 ]; then
    APPS_DIR="${HOME}/.local/share/applications"
    mkdir -p "$APPS_DIR"

    cat > "$APPS_DIR/notice-board.desktop" << EOF
[Desktop Entry]
Type=Application
Name=Notice Board
Comment=Fullscreen notice board for displaying announcements
Exec=${PREFIX}/bin/notice_board
Icon=emblem-information
Categories=Utility;
StartupNotify=false
NoDisplay=false
EOF
    echo "==> Wrote $APPS_DIR/notice-board.desktop"

    if [ "$INSTALL_KIOSK" = 1 ]; then
        cp "$SCRIPT_DIR/run_kiosk.sh" "$PREFIX/bin/notice_board_kiosk"
        chmod +x "$PREFIX/bin/notice_board_kiosk"
        cat > "$APPS_DIR/notice-board-kiosk.desktop" << EOF
[Desktop Entry]
Type=Application
Name=Notice Board (Kiosk)
Comment=Notice board locked in kiosk mode with cage
Exec=${PREFIX}/bin/notice_board_kiosk
Icon=emblem-information
Categories=Utility;
StartupNotify=false
NoDisplay=false
EOF
        echo "==> Wrote $APPS_DIR/notice-board-kiosk.desktop"
    fi
fi

echo "==> Done."
