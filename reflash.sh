#!/bin/bash

source .env

# --local --verbose --repo ../mongoose-os &&
mos build &&
  mos flash &&
  mos wifi ${WIFI_SSID} "${WIFI_PSK}" &&
  mos console