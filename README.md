# mosmqtt_wakeonlan

This app for esp8266 sends wake on lan packets remotely, through MQTT.
The esp8266 is an IoT device that's both reliable and inexpensive.
It might work on other devices that share the SDK, but that has
not been yet tested.

You can find an esp8266 for around 4\$ in AliExpress or a bit more in Amazon.

## Requirements

To flash this app you'll need:

- The actual esp8266 chip itself (and a micro USB cable)
- A Linux (or Mac) environment so you can run the `reflash.sh` script
- [Mongoose OS tool (mos-tool)](https://mongoose-os.com/docs/mongoose-os/quickstart/setup.md) You will build this code with this tool
- [Mongoose OS Dash Account](https://mdash.net/) This is not actually needed, but it's good to have
- [Losant Account](https://losant.com) You can use other brokers but Losant works great
- Docker if you want to build locally, otherwise remove `--local` from `reflash.sh`

## Building and Flashing

1. Clone this repo

   ```bash
   git clone --recursive https://github.com/jamezrin/mosmqtt_wakeonlan
   ```

2. Copy `.env.sample` to `.env` and edit it accordingly
    - The `WIFI_SSID` and `WIFI_PSK` variables are your WiFi SSID and password respectively
    - The `MQTT_CLIENT_ID`, `MQTT_USER` and `MQTT_PASS` variables can be obtained from https://losant.com/
    - The `DASH_TOKEN` variable can be obtained from https://mdash.net/

3. Run the flash script. If everything is fine, your device should be detected automatically and you should end up with a flashed device.

   ```bash
   ./reflash.sh
   ```

## Development

If you plan to make changes, you will probably want IntelliSense, for that install the [official extension](https://marketplace.visualstudio.com/items?itemName=mongoose-os.mongoose-os-ide) from Cesanta.

It is also recommended running at least one time the flash script, or at least, `mos build --local`.

Make sure your C/C++ properties look similar to this

```json
{
  "configurations": [
    {
      "name": "Linux",
      "includePath": ["${workspaceFolder}/**"],
      "defines": ["MG_ENABLE_CALLBACK_USERDATA"],
      "compilerPath": "/usr/bin/gcc",
      "cStandard": "c11",
      "cppStandard": "c++17",
      "intelliSenseMode": "clang-x64"
    }
  ],
  "version": 4
}
```

[Outdated Tutorial](https://jamezrin.wordpress.com/2018/03/09/wake-on-lan-remotely-and-securely-with-esp8266)
