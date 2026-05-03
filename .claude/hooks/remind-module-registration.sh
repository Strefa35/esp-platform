#!/bin/bash
# PostToolUse: Write
# After creating a new .c file inside modules/, reminds about required registration steps.

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty')

if echo "$FILE_PATH" | grep -qE '/modules/[^/]+/[^/]+\.c$'; then
  MODULE=$(echo "$FILE_PATH" | sed -E 's|.*/modules/([^/]+)/.*|\1|')
  echo "New module source created: $FILE_PATH"
  echo ""
  echo "Registration checklist for '$MODULE':"
  echo "  1. include/mgr_reg_list.h  — add #include and mgr_reg_t entry (before mqtt_ctrl)"
  echo "  2. main/CMakeLists.txt     — add to CMAKE_BUILD_EARLY_EXPANSION list and CONFIG_ block"
  echo "  3. include/types.h         — add REG_<NAME>_CTRL to the module type enum"
fi

exit 0
