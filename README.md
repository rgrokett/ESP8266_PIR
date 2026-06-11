# ESP8266 PIR Motion Detector

This project uses an Adafruit Feather HUZZAH ESP8266 and a PIR sensor to send
motion events to an IFTTT Webhooks applet over HTTPS. Wi-Fi modem sleep is used
between events to reduce power consumption.

The original project documentation is available in `ESP8266_PIR.pdf`.

## Hardware

- Adafruit Feather HUZZAH ESP8266
- PIR motion sensor
- Suitable USB or regulated battery power supply

Default pin assignments:

| Function | ESP8266 GPIO |
| --- | --- |
| PIR output | GPIO 14 |
| Built-in LED | GPIO 0 |

The sketch assumes that the PIR output is active-low. Change
`PIR_ACTIVE_STATE` in the sketch if your sensor is active-high.

## Software

- Arduino IDE or Arduino CLI
- ESP8266 Arduino core 3.1.2 (the version used for verification)
- An IFTTT applet using the Webhooks service

The sketch uses `ESP8266HTTPClient` and BearSSL from the ESP8266 core. The old
bundled `HTTPSRedirect` implementation is no longer required.

## Configuration

1. Copy the configuration template:

   ```sh
   cp ESP8266_PIR/secrets.h.example ESP8266_PIR/secrets.h
   ```

2. Edit `ESP8266_PIR/secrets.h` and set:

   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `IFTTT_API_KEY`
   - `IFTTT_EVENT`
   - TLS validation settings

`secrets.h` is ignored by Git so Wi-Fi and IFTTT credentials are not committed.
The sketch compiles without it using nonfunctional placeholders, but it will
not connect or send events until configured.

## TLS Validation

`TLS_VALIDATE_CA` is the recommended mode. Set `IFTTT_CA_CERT` to the
PEM-encoded root CA certificate that validates `maker.ifttt.com`.

`TLS_VALIDATE_FINGERPRINT` validates the server against a SHA-1 certificate
fingerprint. It is less maintainable because it must be updated whenever the
server certificate changes. A fingerprint can be inspected with:

```sh
openssl s_client -servername maker.ifttt.com \
  -connect maker.ifttt.com:443 </dev/null 2>/dev/null |
  openssl x509 -fingerprint -sha1 -noout
```

Replace the colons in the reported fingerprint with spaces before assigning it
to `IFTTT_FINGERPRINT`.

`TLS_INSECURE` disables server authentication. Use it only for temporary
diagnostics on a trusted network. It exposes the webhook key and event request
to man-in-the-middle attacks.

## Build

With Arduino CLI and the ESP8266 core installed:

```sh
arduino-cli compile --fqbn esp8266:esp8266:huzzah ESP8266_PIR
```

Upload using the board's detected serial port:

```sh
arduino-cli upload --fqbn esp8266:esp8266:huzzah \
  --port /dev/ttyUSB0 ESP8266_PIR
```

## Runtime Behavior

- Wi-Fi connection attempts time out after 20 seconds.
- HTTPS requests time out after 10 seconds.
- HTTPS redirects are followed strictly, with a limit of three redirects.
- Only HTTP 2xx responses are treated as successful event delivery.
- The webhook key is never printed to the serial console.
- Motion events have a nonblocking 15-second cooldown.
- Every request path returns the Wi-Fi modem to sleep when power saving is
  enabled, including failures.
- Optional 5 V battery keepalive pulses only run while modem power saving is
  enabled.

Timing, pin, power-saving, and PIR polarity settings are constants near the top
of `ESP8266_PIR/ESP8266_PIR.ino`.

## Deep Sleep Version

A separate hardware-assisted deep-sleep version is available at
https://github.com/rgrokett/ESP8266_PIRv2.
