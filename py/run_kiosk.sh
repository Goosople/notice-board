#!/bin/bash
# Launch Notice Board (Python) in kiosk mode using cage
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
echo "Starting Notice Board (Python) in kiosk mode…"
echo "Exit: Ctrl+Shift+A → unlock → 'Exit Application'"
exec cage -- python3 "$SCRIPT_DIR/notice_board.py"
