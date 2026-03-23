menu "LCD Controller"

    config LCD_CTRL_ENABLE
        bool "Enable LCD Controller"
        depends on IDF_TARGET_ESP32
        default "n"
        help
            Enable LCD Controller to use by the Manager.
            This module is supported only for ESP32 target.

    choice LCD_CTRL_LOG_LEVEL
        bool "LCD Controller Log level"
        default LCD_CTRL_LOG_DEFAULT_LEVEL_INFO
        help
            Specify how much output to see in logs by default.

        config LCD_CTRL_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config LCD_CTRL_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config LCD_CTRL_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config LCD_CTRL_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config LCD_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config LCD_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    choice LCD_HELPER_LOG_LEVEL
        bool "LCD Helper Log level"
        default LCD_HELPER_LOG_DEFAULT_LEVEL_INFO
        help
            Specify how much output to see in logs by default.

        config LCD_HELPER_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config LCD_HELPER_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config LCD_HELPER_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config LCD_HELPER_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config LCD_HELPER_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config LCD_HELPER_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    choice LCD_HW_LOG_LEVEL
        bool "LCD HW Log level"
        default LCD_HW_LOG_DEFAULT_LEVEL_INFO
        help
            Specify how much output to see in logs by default.

        config LCD_HW_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config LCD_HW_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config LCD_HW_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config LCD_HW_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config LCD_HW_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config LCD_HW_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    choice LCD_ILI9341V_LOG_LEVEL
        bool "LCD ILI9341V Log level"
        default LCD_ILI9341V_LOG_DEFAULT_LEVEL_INFO
        help
            Specify how much output to see in logs by default.

        config LCD_ILI9341V_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config LCD_ILI9341V_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config LCD_ILI9341V_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config LCD_ILI9341V_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config LCD_ILI9341V_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config LCD_ILI9341V_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    choice LCD_NS2009_LOG_LEVEL
        bool "LCD NS2009 Log level"
        default LCD_NS2009_LOG_DEFAULT_LEVEL_INFO
        help
            Specify how much output to see in logs by default.

        config LCD_NS2009_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config LCD_NS2009_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config LCD_NS2009_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config LCD_NS2009_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config LCD_NS2009_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config LCD_NS2009_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    menu "NS2009 Touch Calibration"

        config LCD_NS2009_I2C_PORT
            int "I2C port"
            range 0 1
            default 0
            help
                ESP-IDF I2C master port used by NS2009.

        config LCD_NS2009_SDA_GPIO
            int "SDA GPIO"
            range -1 48
            default 13
            help
                GPIO used as I2C SDA for NS2009 touch controller.

        config LCD_NS2009_SCL_GPIO
            int "SCL GPIO"
            range -1 48
            default 16
            help
                GPIO used as I2C SCL for NS2009 touch controller.

        config LCD_NS2009_I2C_FREQ_HZ
            int "I2C frequency [Hz]"
            range 10000 400000
            default 100000
            help
                I2C clock speed for NS2009 communication.

        config LCD_NS2009_TOUCH_THRESHOLD
            int "Touch pressure threshold (Z1)"
            range 1 4095
            default 20
            help
                Minimum Z1 value required to report touch as pressed.

        config LCD_NS2009_REQUIRE_PRESSURE
            bool "Require pressure threshold for touch detect"
            default n
            help
                If enabled, touch is reported only when Z1 is above threshold.
                If disabled, X/Y raw range is used as the main touch presence check.

        config LCD_NS2009_RAW_MIN_X
            int "Raw X minimum"
            range 0 4095
            default 180
            help
                Minimum raw X value for calibration mapping.

        config LCD_NS2009_RAW_MAX_X
            int "Raw X maximum"
            range 0 4095
            default 3900
            help
                Maximum raw X value for calibration mapping.

        config LCD_NS2009_RAW_MIN_Y
            int "Raw Y minimum"
            range 0 4095
            default 220
            help
                Minimum raw Y value for calibration mapping.

        config LCD_NS2009_RAW_MAX_Y
            int "Raw Y maximum"
            range 0 4095
            default 3900
            help
                Maximum raw Y value for calibration mapping.

        config LCD_NS2009_SWAP_XY
            bool "Swap X and Y axes"
            default n
            help
                Swap axes if touch orientation is rotated by 90 degrees.

        config LCD_NS2009_INVERT_X
            bool "Invert X axis"
            default n
            help
                Invert calibrated X axis direction.

        config LCD_NS2009_INVERT_Y
            bool "Invert Y axis"
            default n
            help
                Invert calibrated Y axis direction.

    endmenu

    choice LCD_UI_ROTATION
        bool "LCD UI rotation"
        default LCD_UI_ROTATION_0
        help
            Rotate LVGL UI to match physical panel mounting orientation.

        config LCD_UI_ROTATION_0
            bool "0 degrees"

        config LCD_UI_ROTATION_90
            bool "90 degrees"

        config LCD_UI_ROTATION_180
            bool "180 degrees"

        config LCD_UI_ROTATION_270
            bool "270 degrees"
    endchoice

    config LCD_CTRL_LOG_LEVEL
        int
        default 0 if LCD_CTRL_LOG_DEFAULT_LEVEL_NONE
        default 1 if LCD_CTRL_LOG_DEFAULT_LEVEL_ERROR
        default 2 if LCD_CTRL_LOG_DEFAULT_LEVEL_WARN
        default 3 if LCD_CTRL_LOG_DEFAULT_LEVEL_INFO
        default 4 if LCD_CTRL_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if LCD_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

    config LCD_HELPER_LOG_LEVEL
        int
        default 0 if LCD_HELPER_LOG_DEFAULT_LEVEL_NONE
        default 1 if LCD_HELPER_LOG_DEFAULT_LEVEL_ERROR
        default 2 if LCD_HELPER_LOG_DEFAULT_LEVEL_WARN
        default 3 if LCD_HELPER_LOG_DEFAULT_LEVEL_INFO
        default 4 if LCD_HELPER_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if LCD_HELPER_LOG_DEFAULT_LEVEL_VERBOSE

    config LCD_HW_LOG_LEVEL
        int
        default 0 if LCD_HW_LOG_DEFAULT_LEVEL_NONE
        default 1 if LCD_HW_LOG_DEFAULT_LEVEL_ERROR
        default 2 if LCD_HW_LOG_DEFAULT_LEVEL_WARN
        default 3 if LCD_HW_LOG_DEFAULT_LEVEL_INFO
        default 4 if LCD_HW_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if LCD_HW_LOG_DEFAULT_LEVEL_VERBOSE

    config LCD_ILI9341V_LOG_LEVEL
        int
        default 0 if LCD_ILI9341V_LOG_DEFAULT_LEVEL_NONE
        default 1 if LCD_ILI9341V_LOG_DEFAULT_LEVEL_ERROR
        default 2 if LCD_ILI9341V_LOG_DEFAULT_LEVEL_WARN
        default 3 if LCD_ILI9341V_LOG_DEFAULT_LEVEL_INFO
        default 4 if LCD_ILI9341V_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if LCD_ILI9341V_LOG_DEFAULT_LEVEL_VERBOSE

    config LCD_NS2009_LOG_LEVEL
        int
        default 0 if LCD_NS2009_LOG_DEFAULT_LEVEL_NONE
        default 1 if LCD_NS2009_LOG_DEFAULT_LEVEL_ERROR
        default 2 if LCD_NS2009_LOG_DEFAULT_LEVEL_WARN
        default 3 if LCD_NS2009_LOG_DEFAULT_LEVEL_INFO
        default 4 if LCD_NS2009_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if LCD_NS2009_LOG_DEFAULT_LEVEL_VERBOSE


endmenu
