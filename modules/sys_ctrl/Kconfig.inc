menu "SYS Controller"

    config SYS_CTRL_ENABLE
        bool "Enable SYS Controller"
        default "n"
        help
            Enable SYS Controller to use by the Manager

    choice SYS_CTRL_LOG_LEVEL
        bool "Log level"
        default SYS_CTRL_LOG_DEFAULT_LEVEL_INFO
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

        config SYS_CTRL_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config SYS_CTRL_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config SYS_CTRL_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config SYS_CTRL_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config SYS_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config SYS_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    config SYS_CTRL_LOG_LEVEL
        int
        default 0 if SYS_CTRL_LOG_DEFAULT_LEVEL_NONE
        default 1 if SYS_CTRL_LOG_DEFAULT_LEVEL_ERROR
        default 2 if SYS_CTRL_LOG_DEFAULT_LEVEL_WARN
        default 3 if SYS_CTRL_LOG_DEFAULT_LEVEL_INFO
        default 4 if SYS_CTRL_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if SYS_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

    choice SYS_CTRL_TIMEZONE_DEFAULT
        prompt "Default Timezone"
        default SYS_CTRL_TIMEZONE_CET
        help
            Set the default timezone for the system.
            This follows POSIX TZ string format.

        config SYS_CTRL_TIMEZONE_UTC
            bool "UTC"
        config SYS_CTRL_TIMEZONE_CET
            bool "Central European Time (Poland, CET)"
        config SYS_CTRL_TIMEZONE_EST
            bool "Eastern Standard Time (US)"
        config SYS_CTRL_TIMEZONE_PST
            bool "Pacific Standard Time (US)"
        config SYS_CTRL_TIMEZONE_CST
            bool "Central Standard Time (US, Dallas)"
        config SYS_CTRL_TIMEZONE_CUSTOM
            bool "Custom"
    endchoice

    config SYS_CTRL_TIMEZONE_STRING
        string
        default "UTC0" if SYS_CTRL_TIMEZONE_UTC
        default "CET-1CEST,M3.5.0,M10.5.0/3" if SYS_CTRL_TIMEZONE_CET
        default "EST5EDT,M3.2.0,M11.1.0" if SYS_CTRL_TIMEZONE_EST
        default "PST8PDT,M3.2.0,M11.1.0" if SYS_CTRL_TIMEZONE_PST
        default "CST6CDT,M3.2.0/2,M11.1.0/2" if SYS_CTRL_TIMEZONE_CST
        default ""
        help
            Automatically derived POSIX TZ string based on selected timezone.
            For custom timezone, enter the POSIX format string below.

    config SYS_CTRL_TIMEZONE_CUSTOM_STRING
        string "Custom Timezone String (POSIX format)"
        depends on SYS_CTRL_TIMEZONE_CUSTOM
        default ""
        help
            Enter custom POSIX TZ string when "Custom" timezone is selected.
            Examples:
            - "CET-1CEST,M3.5.0,M10.5.0/3" - Central European Time
            - "EST5EDT,M3.2.0,M11.1.0" - Eastern Time
            - "PST8PDT,M3.2.0,M11.1.0" - Pacific Time
            - "CST6CDT,M3.2.0/2,M11.1.0/2" - Central Time (US, Dallas)
            - "UTC0" - UTC

    config SYS_CTRL_NTP_SERVER_DEFAULT
        string "Default NTP Server"
        default "pool.ntp.org"
        help
            Default NTP server to use for time synchronization.
            Examples:
            - "pool.ntp.org" - NTP Pool Project
            - "time.google.com" - Google Public NTP
            - "time.cloudflare.com" - Cloudflare NTP
            - "time.nist.gov" - NIST Internet Time Service

endmenu
