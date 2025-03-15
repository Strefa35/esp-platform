menu "GPIO Controller"

    config GPIO_CTRL_ENABLE
        bool "Enable GPIO Controller"
        default "n"
        help
            Enable GPIO Controller to use by the Manager

    choice GPIO_CTRL_LOG_LEVEL
        bool "Log level"
        default GPIO_CTRL_LOG_DEFAULT_LEVEL_INFO
        help
            Specify how much output to see in logs by default.
            You can set lower verbosity level at runtime using
            esp_log_level_set() function if LOG_DYNAMIC_LEVEL_CONTROL
            is enabled.

            By default, this setting limits which log statements
            are compiled into the program. For example, selecting
            "Warning" would mean that changing log level to "Debug"
            at runtime will not be possible. To allow increasing log
            level above the default at runtime, see the next option.

        config GPIO_CTRL_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config GPIO_CTRL_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config GPIO_CTRL_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config GPIO_CTRL_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config GPIO_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config GPIO_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    config GPIO_CTRL_LOG_LEVEL
        int
        default 0 if GPIO_CTRL_LOG_DEFAULT_LEVEL_NONE
        default 1 if GPIO_CTRL_LOG_DEFAULT_LEVEL_ERROR
        default 2 if GPIO_CTRL_LOG_DEFAULT_LEVEL_WARN
        default 3 if GPIO_CTRL_LOG_DEFAULT_LEVEL_INFO
        default 4 if GPIO_CTRL_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if GPIO_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

endmenu