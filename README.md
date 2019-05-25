# mosmqtt_wakeonlan

An app for your esp8266 to wake your computer remotely and cheap

This app is meant to be flashed to an esp8266, it will probably work on other devices
but that hasn't been tested.

You can find an esp8266 for around 4\$ in AliExpress or a bit more in Amazon.

## Building and Flashing

To flash this app you'll need:

- The actual esp8266 chip itself (and a micro USB cable)
- A Linux (or Mac) environment so you can run the `reflash.sh` script
- [Mongoose OS tool (mos-tool)](https://mongoose-os.com/docs/mongoose-os/quickstart/setup.md) You will build this code with this tool
- [Mongoose OS Dash Account](https://mdash.net/) This is not actually needed, but it's good to have
- [Losant Account](https://losant.com) You can use other brokers but Losant works great
- Docker if you want to build locally, otherwise remove `--local` from `reflash.sh`

If you want to modify code, specially C code install VS Code and [this extension](https://marketplace.visualstudio.com/items?itemName=alxandr.mongoose-deps-gen).

You might also want to try installing the [official extension](https://marketplace.visualstudio.com/items?itemName=mongoose-os.mongoose-os-ide) but I preferred to work without it.

[Outdated Tutorial](https://jamezrin.wordpress.com/2018/03/09/wake-on-lan-remotely-and-securely-with-esp8266)
