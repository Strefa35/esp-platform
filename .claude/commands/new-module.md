Scaffold a new ESP platform controller module named **$ARGUMENTS**.

The module name should be lowercase, e.g. `foo` → module `foo_ctrl`.

Follow the exact pattern from `modules/template_ctrl/`. Create all files below, replacing every occurrence of `template`/`TEMPLATE`/`Template` with the new module name in the appropriate case:

### Files to create

1. **`modules/<name>_ctrl/<name>_ctrl.c`** — copy structure from `modules/template_ctrl/template_ctrl.c`:
   - Static handles: `QueueHandle_t`, `TaskHandle_t`, `SemaphoreHandle_t`, `data_uid_t esp_uid`
   - Internal functions: `parseMqttData`, `<name>ctrl_ParseMsg`, `<name>ctrl_TaskFn`, `<name>ctrl_Send`, `<name>ctrl_Init`, `<name>ctrl_Done`, `<name>ctrl_Run`
   - Public functions: `<Name>Ctrl_Init`, `<Name>Ctrl_Done`, `<Name>Ctrl_Run`, `<Name>Ctrl_Send`
   - TAG: `"ESP::<NAME>"`

2. **`modules/<name>_ctrl/include/<name>_ctrl.h`** — copy from `modules/template_ctrl/include/template_ctrl.h`, update include guard and function names.

3. **`modules/<name>_ctrl/CMakeLists.txt`** — copy from `modules/template_ctrl/CMakeLists.txt`, update the status message and source file name.

4. **`modules/<name>_ctrl/Kconfig.inc`** — copy from `modules/template_ctrl/Kconfig.inc`, replace all `TEMPLATE` → `<NAME>` and update the menu label.

### Files to modify

1. **`include/msg.h`** — add a `REG_<NAME>_CTRL` bitmask `#define` (pick the next free bit in the appropriate group).

2. **`include/mgr_reg_list.h`**:
   - Add `#ifdef CONFIG_<NAME>_CTRL_ENABLE` / `#include "<name>_ctrl.h"` / `#endif` in the includes block
   - Add a `mgr_reg_t` entry in `mgr_reg_list[]` (before the `cli_ctrl` entry, and definitely before `mqtt_ctrl` which must stay last)

3. **`modules/Kconfig.inc`** — add `orsource "<name>_ctrl/Kconfig.inc"` to the list.

4. **`main/CMakeLists.txt`**:
   - Under `CMAKE_BUILD_EARLY_EXPANSION`: add `<name>_ctrl` to the list
   - Add a conditional block: `if(CONFIG_<NAME>_CTRL_ENABLE)` → append include dir and priv require

After creating all files, print a summary of the `sdkconfig.defaults` line needed to enable the module:

```kconfig
CONFIG_<NAME>_CTRL_ENABLE=y
```
