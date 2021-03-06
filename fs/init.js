load('api_config.js');
load('api_events.js');
load('api_mqtt.js');
load('api_net.js');
load('api_sys.js');
load('api_timer.js');
load('api_shadow.js');
load('api_wifi.js');

let state = { requests: 0, uptime: 0, free_ram: 0 };

Timer.set(1000, Timer.REPEAT, function () {
  state.uptime = Sys.uptime();
  state.free_ram = Sys.free_ram();

  Shadow.update(0, state);

  print('Current state:',
    JSON.stringify(state)
  );
}, null);

let wake_device = ffi('void wake_device_a(char*)');

MQTT.sub('WakeDevice', function (conn, topic, msg) {
  state.requests++;

  let data = JSON.parse(msg);
  print('Received request to wake:', data.mac);

  wake_device(data.mac);
  MQTT.pub('WakeDeviceRecv', JSON.stringify({
    id: data.id,
    mac: data.mac
  }), 1);
});

let find_device = ffi('void find_device_a(char*, void (*)(char*, userdata), userdata)');

MQTT.sub('FindDevice', function (conn, topic, msg) {
  state.requests++;

  let data = JSON.parse(msg);
  print('Received request to discover:', data.mac);

  MQTT.pub('FindDeviceRecv', JSON.stringify({
    id: data.id,
    mac: data.mac
  }), 1);

  find_device(data.mac, function (ipaddr, userdata) {
    print("Discovered device", userdata.mac, "with ip", ipaddr);
    MQTT.pub('FoundDevice', JSON.stringify({
      id: userdata.id,
      mac: userdata.mac,
      ip: ipaddr
    }));
  }, data);
});

let clear_list = ffi('void clear_list()');

Event.addGroupHandler(Net.EVENT_GRP, function (ev, evdata, arg) {
  let evs = '???';
  if (ev === Net.STATUS_DISCONNECTED) {
    evs = 'DISCONNECTED';
  } else if (ev === Net.STATUS_CONNECTING) {
    evs = 'CONNECTING';
  } else if (ev === Net.STATUS_CONNECTED) {
    evs = 'CONNECTED';
  } else if (ev === Net.STATUS_GOT_IP) {
    evs = 'GOT_IP';
  }

  if (ev === Net.STATUS_DISCONNECTED ||
    ev === Net.STATUS_CONNECTING) {
    clear_list();
  }

  print('Network event:', ev, evs);
}, null);