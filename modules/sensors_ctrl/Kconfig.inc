menu "Sensors Controller"

    config SENSORS_CTRL_ENABLE
        bool "Enable Sensors Controller"
        default "n"
        help
            Enable Sensors Controller to use by the Manager

        choice SENSORS_CTRL_LOG_LEVEL
            depends on SENSORS_CTRL_ENABLE
            bool "Log level"
            default SENSORS_CTRL_LOG_DEFAULT_LEVEL_INFO
            help
                Specify how much output to see in logs by default.
                You can set lower verbosity level at runtime using
                esp_log_level_set() function if LOG_DYNAMIC_LEVEL_CONTROL
                is enabled.

            config SENSORS_CTRL_LOG_DEFAULT_LEVEL_NONE
                bool "No output"
            config SENSORS_CTRL_LOG_DEFAULT_LEVEL_ERROR
                bool "Error"
            config SENSORS_CTRL_LOG_DEFAULT_LEVEL_WARN
                bool "Warning"
            config SENSORS_CTRL_LOG_DEFAULT_LEVEL_INFO
                bool "Info"
            config SENSORS_CTRL_LOG_DEFAULT_LEVEL_DEBUG
                bool "Debug"
            config SENSORS_CTRL_LOG_DEFAULT_LEVEL_VERBOSE
                bool "Verbose"
        endchoice

        config SENSORS_CTRL_LOG_LEVEL
            int
            default 0 if SENSORS_CTRL_LOG_DEFAULT_LEVEL_NONE
            default 1 if SENSORS_CTRL_LOG_DEFAULT_LEVEL_ERROR
            default 2 if SENSORS_CTRL_LOG_DEFAULT_LEVEL_WARN
            default 3 if SENSORS_CTRL_LOG_DEFAULT_LEVEL_INFO
            default 4 if SENSORS_CTRL_LOG_DEFAULT_LEVEL_DEBUG
            default 5 if SENSORS_CTRL_LOG_DEFAULT_LEVEL_VERBOSE

    config SENSOR_TSL2561_ENABLE
        depends on SENSORS_CTRL_ENABLE
        bool "Enable Light Sensor TSL2561"
        default "n"
        help
            Enable Light Sensor TSL2561 to use in Sensors Controller

        choice SENSOR_TSL2561_LOG_LEVEL
            depends on SENSOR_TSL2561_ENABLE
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
