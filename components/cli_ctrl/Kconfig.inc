menu "CLI Controller"

    config CLI_CTRL_ENABLE
        bool "Enable CLI Controller"
        default "n"
        help
            Enable CLI Controller to use by the Manager

    choice CLI_CTRL_LOG_LEVEL
        depends on CLI_CTRL_ENABLE
        bool "Log level"
        default CLI_CTRL_LOG_DEFAULT_LEVEL_INFO
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

        config CLI_CTRL_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config CLI_CTRL_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config CLI_CTRL_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config CLI_CTRL_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config CLI_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config CLI_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    config CLI_CTRL_LOG_LEVEL
        depends on CLI_CTRL_ENABLE
        int
        default 0 if CLI_CTRL_LOG_DEFAULT_LEVEL_NONE
        default 1 if CLI_CTRL_LOG_DEFAULT_LEVEL_ERROR
        default 2 if CLI_CTRL_LOG_DEFAULT_LEVEL_WARN
        default 3 if CLI_CTRL_LOG_DEFAULT_LEVEL_INFO
        default 4 if CLI_CTRL_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if CLI_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

endmenu