menu "Template Controller"

    config TEMPLATE_CTRL_ENABLE
        bool "Enable Template Controller"
        default "n"
        help
            Enable Template Controller to use by the Manager

    choice TEMPLATE_CTRL_LOG_LEVEL
        depends on TEMPLATE_CTRL_ENABLE
        bool "Log level"
        default TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_INFO
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

        config TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    config TEMPLATE_CTRL_LOG_LEVEL
        depends on TEMPLATE_CTRL_ENABLE
        int
        default 0 if TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_NONE
        default 1 if TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_ERROR
        default 2 if TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_WARN
        default 3 if TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_INFO
        default 4 if TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if TEMPLATE_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

endmenu
