menu "ETH Controller"

    config ETH_CTRL_ENABLE
        bool "Enable Ethernet Controller"
        default "y"
        help
            Enable Ethernet Controller to use by the Manager

    choice ETH_CTRL_LOG_LEVEL
        depends on ETH_CTRL_ENABLE
        bool "Log level"
        default ETH_CTRL_LOG_DEFAULT_LEVEL_INFO
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

        config ETH_CTRL_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config ETH_CTRL_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config ETH_CTRL_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config ETH_CTRL_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config ETH_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config ETH_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    config ETH_CTRL_LOG_LEVEL
        depends on ETH_CTRL_ENABLE
        int
        default 0 if ETH_CTRL_LOG_DEFAULT_LEVEL_NONE
        default 1 if ETH_CTRL_LOG_DEFAULT_LEVEL_ERROR
        default 2 if ETH_CTRL_LOG_DEFAULT_LEVEL_WARN
        default 3 if ETH_CTRL_LOG_DEFAULT_LEVEL_INFO
        default 4 if ETH_CTRL_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if ETH_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

endmenu