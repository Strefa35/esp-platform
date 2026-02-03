# MQTT Protocol Documentation

The MQTT protocol is used for bidirectional communication between the Controller and the outside world. This allows you to send information from the controller through the **MQTT Broker** to any other module that has also connected to the broker, as well as send queries from an external module to the controller. Data is sent in **JSON format**.

The **MQTT** protocol consists of two parts:
- **topic** - used to inform which resource is the sender or receiver of the information
- **payload** - used to pass the information in **JSON format**

The most important field in the JSON payload is **operation** which specifies the type of operation:
- `event` - information about new data
- `set` - set request
- `get` - get request
- `response` - set/get response

More information: [mqtt.org](https://mqtt.org/)

## List of topics

- [REGISTER/ESP](#registeresp)
- [MQTT Module](#mqtt-module)
- [RELAY Module](#relay-module)
- [SENSOR Module](#sensor-module)
- [SYSTEM Module](#system-module)

---

## REGISTER/ESP

Information about connected devices. After starting, the controller sends information about its configuration, then subscribes to the topic list for each resource.

### Request
- **Topic:** `REGISTER/ESP`
- **Operation:** `get`
- **Get current controller configuration:**
  ```json
  {
    "operation": "get"
  }
  ```

### Response
- **Topic:** `REGISTER/ESP`
- **Operation:** `response`
- **Response after get:**
  ```json
  {
    "operation": "response",
    "uid": "ESP/12AB34",
    "mac": "12:34:56:78:90:AB",
    "ip": "10.0.0.20",
    "list": ["eth", "mqtt"]
  }
  ```

### Event
- **Topic:** `REGISTER/ESP`
- **Operation:** `event`
- **Notification about controller configuration and list of available modules:**
  ```json
  {
    "operation": "event",
    "uid": "ESP/12AB34",
    "mac": "12:34:56:78:90:AB",
    "ip": "10.0.0.20",
    "list": ["eth", "mqtt"]
  }
  ```

---

## MQTT Module

Configuration of MQTT broker connection.

### Request
- **Topic:** `ESP/12AB34/req/mqtt`
- **Operation:** `set/get`

**Set broker configuration (address, port, credentials):**
```json
{
  "operation": "set",
  "broker": { 
    "address": {
      "uri": "mqtt://broker.example.com",
      "port": 1883
    },
    "username": "user",
    "password": "pass"
  }
}
```

**Get current state for mqtt:**
```json
{
  "operation": "get"
}
```

### Response
- **Topic:** `ESP/12AB34/res/mqtt`
- **Operation:** `response`
- **Response after get:**
  ```json
  {
    "operation": "response",
    "broker": { 
      "address": {
        "uri": "mqtt://broker.example.com",
        "port": 1883
      }
    }
  }
  ```

---

## RELAY Module

Control of relay switches.

### Request
- **Topic:** `ESP/12AB34/req/relay`
- **Operation:** `set/get`

**Set state for specific relay number:**
```json
{
  "operation": "set",
  "relays": [
    { 
      "number": 0,
      "state": "on"
    },
    { 
      "number": 1,
      "state": "off"
    }
  ]
}
```

**Get current state for specific relay number:**
```json
{
  "operation": "get"
}
```

### Response
- **Topic:** `ESP/12AB34/res/relay`
- **Operation:** `response`
- **Response after get:**
  ```json
  {
    "operation": "response",
    "relays": [
      { 
        "number": 0,
        "state": "on"
      },
      { 
        "number": 1,
        "state": "off"
      }
    ]
  }
  ```

### Event
- **Topic:** `ESP/12AB34/event/relay`
- **Operation:** `event`
- **Event for specific relays:**
  ```json
  {
    "operation": "event",
    "relays": [
      { 
        "number": 0,
        "state": "on"
      },
      { 
        "number": 1,
        "state": "off"
      }
    ]
  }
  ```

---

## SENSOR Module

Configuration and monitoring of sensors.

### Request
- **Topic:** `ESP/12AB34/req/sensor`
- **Operation:** `set/get`

**Set sensor configuration:**
```json
{
  "operation": "set",
  "sensor": "name-of-sensor",
  "data": [
    {
      "type": "threshold",
      "threshold": 100
    },
    {
      "type": "lux",
      "lux": 5000
    }
  ]
}
```

**Get list of parameters for specific sensor:**
```json
{
  "operation": "get",
  "sensor": "name-of-sensor",
  "data": ["info", "threshold", "lux"]
}
```

### Response
- **Topic:** `ESP/12AB34/res/sensor`
- **Operation:** `response`
- **Response after get:**
  ```json
  {
    "operation": "response",
    "sensor": "name-of-sensor",
    "data": [
      {
        "type": "threshold",
        "threshold": 100
      },
      {
        "type": "lux",
        "lux": 5000
      },
      {
        "type": "info",
        "info": {}
      }
    ]
  }
  ```

### Event
- **Topic:** `ESP/12AB34/event/sensor`
- **Operation:** `event`
- **Event for specific sensor:**
  ```json
  {
    "operation": "event",
    "sensor": "name-of-sensor",
    "data": [
      {
        "type": "threshold",
        "threshold": 100
      },
      {
        "type": "lux",
        "lux": 5000
      }
    ]
  }
  ```

---

## SYSTEM Module

Time synchronization (NTP) and timezone configuration.

### Request
- **Topic:** `ESP/12AB34/req/sys`
- **Operation:** `set/get`

**Set timezone, NTP server(s), or time (all fields are optional):**
```json
{
  "operation": "set",
  "timezone": "CST6CDT,M3.2.0/2,M11.1.0/2",
  "time": {
    "unix": 1738512000,
    "local": "2026-02-02 14:30:00",
    "utc": "2026-02-02 20:30:00"
  },
  "ntp": {
    "servers": [
      "pool.ntp.org",
      "time.google.com",
      "time.cloudflare.com"
    ]
  }
}
```

**Timezone format** follows POSIX TZ string format. Examples:
- `"CET-1CEST,M3.5.0,M10.5.0/3"` - Central European Time
- `"EST5EDT,M3.2.0,M11.1.0"` - Eastern Time (US)
- `"PST8PDT,M3.2.0,M11.1.0"` - Pacific Time (US)
- `"CST6CDT,M3.2.0/2,M11.1.0/2"` - Central Time (US, Dallas)
- `"UTC0"` - UTC

**Get current time and timezone information** (specify which fields to retrieve):
```json
{
  "operation": "get",
  "fields": ["timezone", "time", "ntp"]
}
```

**Available fields:**
- `"timezone"` - current timezone
- `"time"` - current time information
- `"ntp"` - NTP server configuration and sync status

### Response
- **Topic:** `ESP/12AB34/res/sys`
- **Operation:** `response`
- **Response after get:**
  ```json
  {
    "operation": "response",
    "timezone": "CST6CDT,M3.2.0/2,M11.1.0/2",
    "time": {
      "unix": 1738512000,
      "local": "2026-02-02 14:30:00",
      "utc": "2026-02-02 20:30:00"
    },
    "ntp": {
      "servers": [
        "pool.ntp.org",
        "time.google.com",
        "time.cloudflare.com"
      ],
      "synced": true
    }
  }
  ```

### Event
- **Topic:** `ESP/12AB34/event/sys`
- **Operation:** `event`
- **Event when time is synchronized or timezone changes:**
  ```json
  {
    "operation": "event",
    "timezone": "CST6CDT,M3.2.0/2,M11.1.0/2",
    "time": {
      "unix": 1738512000,
      "local": "2026-02-02 14:30:00",
      "utc": "2026-02-02 20:30:00"
    },
    "ntp": {
      "servers": [
        "pool.ntp.org",
        "time.google.com",
        "time.cloudflare.com"
      ],
      "synced": true
    }
  }
  ```
