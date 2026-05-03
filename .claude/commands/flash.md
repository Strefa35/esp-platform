Flash and monitor the ESP platform firmware.

Arguments (optional): `$ARGUMENTS`
- If a port is given (e.g. `/dev/ttyUSB0`), use it directly.
- If no port is given, detect automatically:
  - Check `/dev/ttyUSB*` first (USB-A adapter)
  - Fall back to `/dev/ttyACM*` (USB-C / native USB)

Run:
```
idf.py -p <port> flash monitor
```

If no serial device is found, list what is in `/dev/tty*` and ask the user to specify the port.
