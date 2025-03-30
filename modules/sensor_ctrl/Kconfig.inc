menu "Sensor Controller"

    config SENSOR_CTRL_ENABLE
        bool "Enable Sensor Controller"
        default "n"
        help
            Enable Sensor Controller to use by the Manager

        choice SENSOR_CTRL_LOG_LEVEL
            bool "Log level"
            default SENSOR_CTRL_LOG_DEFAULT_LEVEL_INFO
            help
                Specify how much output to see in logs by default.
                You can set lower verbosity level at runtime using
                esp_log_level_set() function if LOG_DYNAMIC_LEVEL_CONTROL
                is enabled.

            config SENSOR_CTRL_LOG_DEFAULT_LEVEL_NONE
                bool "No output"
            config SENSOR_CTRL_LOG_DEFAULT_LEVEL_ERROR
                bool "Error"
            config SENSOR_CTRL_LOG_DEFAULT_LEVEL_WARN
                bool "Warning"
            config SENSOR_CTRL_LOG_DEFAULT_LEVEL_INFO
                bool "Info"
            config SENSOR_CTRL_LOG_DEFAULT_LEVEL_DEBUG
                bool "Debug"
            config SENSOR_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
                bool "Verbose"
        endchoice

        config SENSOR_CTRL_LOG_LEVEL
            int
            default 0 if SENSOR_CTRL_LOG_DEFAULT_LEVEL_NONE
            default 1 if SENSOR_CTRL_LOG_DEFAULT_LEVEL_ERROR
            default 2 if SENSOR_CTRL_LOG_DEFAULT_LEVEL_WARN
            default 3 if SENSOR_CTRL_LOG_DEFAULT_LEVEL_INFO
            default 4 if SENSOR_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            default 5 if SENSOR_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

    config SENSOR_TSL2561_ENABLE
        bool "Enable Light Sensor TSL2561"
        default "n"
        help
            Enable Light Sensor TSL2561 to use in Sensors Controller

        choice SENSOR_TSL2561_LOG_LEVEL
            bool "Log level"
            default SENSOR_TSL2561_LOG_DEFAULT_LEVEL_INFO
            help
                Specify how much output to see in logs by default.
                You can set lower verbosity level at runtime using
                esp_log_level_set() function if LOG_DYNAMIC_LEVEL_CONTROL
                is enabled.

            config SENSOR_TSL2561_LOG_DEFAULT_LEVEL_NONE
                bool "No output"
            config SENSOR_TSL2561_LOG_DEFAULT_LEVEL_ERROR
                bool "Error"
            config SENSOR_TSL2561_LOG_DEFAULT_LEVEL_WARN
                bool "Warning"
            config SENSOR_TSL2561_LOG_DEFAULT_LEVEL_INFO
                bool "Info"
            config SENSOR_TSL2561_LOG_DEFAULT_LEVEL_DEBUG
                bool "Debug"
            config SENSOR_TSL2561_LOG_DEFAULT_LEVEL_VERBOSE
                bool "Verbose"
        endchoice

        config SENSOR_TSL2561_LOG_LEVEL
            int
            default 0 if SENSOR_TSL2561_LOG_DEFAULT_LEVEL_NONE
            default 1 if SENSOR_TSL2561_LOG_DEFAULT_LEVEL_ERROR
            default 2 if SENSOR_TSL2561_LOG_DEFAULT_LEVEL_WARN
            default 3 if SENSOR_TSL2561_LOG_DEFAULT_LEVEL_INFO
            default 4 if SENSOR_TSL2561_LOG_DEFAULT_LEVEL_DEBUG
            default 5 if SENSOR_TSL2561_LOG_DEFAULT_LEVEL_VERBOSE

endmenu
