#!/bin/bash
# Launch Notice Board (C++) in kiosk mode using cage
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
echo "Starting Notice Board (C++) in kiosk mode…"
echo "Exit: Ctrl+Shift+A → unlock → 'Exit Application'"
exec cage -- "$SCRIPT_DIR/build/notice_board"
