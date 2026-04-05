menu "WiFi Controller"

    config WIFI_CTRL_ENABLE
        bool "Enable WiFi Controller (STA)"
        default "n"
        help
            Station-mode Wi-Fi: scan, connect, IP and link events forwarded to the manager.

    choice WIFI_CTRL_LOG_LEVEL
        bool "Log level"
        default WIFI_CTRL_LOG_DEFAULT_LEVEL_INFO
        help
            Default compile-time log verbosity for the WiFi controller tag.

        config WIFI_CTRL_LOG_DEFAULT_LEVEL_NONE
            bool "No output"
        config WIFI_CTRL_LOG_DEFAULT_LEVEL_ERROR
            bool "Error"
        config WIFI_CTRL_LOG_DEFAULT_LEVEL_WARN
            bool "Warning"
        config WIFI_CTRL_LOG_DEFAULT_LEVEL_INFO
            bool "Info"
        config WIFI_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            bool "Debug"
        config WIFI_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
            bool "Verbose"
    endchoice

    config WIFI_CTRL_LOG_LEVEL
        int
        default 0 if WIFI_CTRL_LOG_DEFAULT_LEVEL_NONE
        default 1 if WIFI_CTRL_LOG_DEFAULT_LEVEL_ERROR
        default 2 if WIFI_CTRL_LOG_DEFAULT_LEVEL_WARN
        default 3 if WIFI_CTRL_LOG_DEFAULT_LEVEL_INFO
        default 4 if WIFI_CTRL_LOG_DEFAULT_LEVEL_DEBUG
        default 5 if WIFI_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

    config WIFI_CTRL_SCAN_MAX_AP
        int "Maximum AP records kept after a scan"
        range 1 40
        default 20
        help
            Number of wifi_ap_record_t entries stored after esp_wifi_scan_get_ap_records().

endmenu
