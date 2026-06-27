#!/bin/bash
# Launch Notice Board (Python) in kiosk mode using cage
set -e
CONFIG="$HOME/.config/notice_board/config.json"
CAGE_MODE=$(python3 -c "import json; print(json.load(open('$CONFIG')).get('cage_mode','extend'))" 2>/dev/null || echo "extend")
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP="$SCRIPT_DIR/notice_board.py"
[ -f "$APP" ] || APP="$SCRIPT_DIR/notice_board"
echo "Starting Notice Board (Python) in kiosk mode…"
echo "Exit: Ctrl+Shift+A → unlock → 'Exit Application'"
exec cage -m "$CAGE_MODE" -- python3 "$APP"
