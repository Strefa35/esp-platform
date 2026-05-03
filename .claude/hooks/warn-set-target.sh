#!/bin/bash
# PreToolUse: Bash
# Warns before idf.py set-target, which wipes sdkconfig.

INPUT=$(cat)
COMMAND=$(echo "$INPUT" | jq -r '.tool_input.command // empty')

if echo "$COMMAND" | grep -q "set-target"; then
  echo "NOTICE: 'idf.py set-target' will delete the current sdkconfig."
  echo "To preserve your settings, run first: idf.py save-defconfig"
fi

exit 0
