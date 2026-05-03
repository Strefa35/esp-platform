#!/bin/bash
# PreToolUse: Edit | Write
# Blocks direct editing of sdkconfig — use idf.py menuconfig instead.

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty')
BASENAME=$(basename "$FILE_PATH")

if [ "$BASENAME" = "sdkconfig" ]; then
  echo "Direct edits to 'sdkconfig' are not allowed." >&2
  echo "Use 'idf.py menuconfig' to change configuration." >&2
  echo "Direct edits will be overwritten on the next build." >&2
  exit 2
fi

exit 0
