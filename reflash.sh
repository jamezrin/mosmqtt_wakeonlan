#!/bin/bash

set -e
source .env

# builds the firmware
mos build

# flashes the firmware
mos flash

# sets all our configuration options
mos config-set \
  dash.enable=true \
  dash.token=${DASH_TOKEN} \
  mqtt.client_id=${MQTT_CLIENT_ID} \
  mqtt.user=${MQTT_USER} \
  mqtt.pass=${MQTT_PASS}

# configures the wifi
mos wifi ${WIFI_SSID} "${WIFI_PSK}"

# starts up the console
mos console

exit 0
