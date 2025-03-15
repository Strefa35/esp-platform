menu "LCD Controller"

    config LCD_CTRL_ENABLE
        bool "Enable LCD Controller"
        default "n"
        help
            Enable LCD Controller to use by the Manager

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