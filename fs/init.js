load('api_config.js');
load('api_events.js');
load('api_mqtt.js');
load('api_net.js');
load('api_sys.js');
load('api_timer.js');

let unhexlify = ffi('char* unhexlify(char*)');

Timer.set(
  1000 * 10,
  Timer.REPEAT,
  function() {
    print(
      'PING! Uptime:',
      Sys.uptime(),
      JSON.stringify({
        total_ram: Sys.total_ram(),
        free_ram: Sys.free_ram()
      })
    );
  },
  null
);

MQTT.sub(
  'SendWol',
  function(conn, topic, msg) {
    let data = JSON.parse(msg);
    print('Waking up:', data.mac);

    let packet = '\xff\xff\xff\xff\xff\xff';
    let target = unhexlify(data.mac);

    for (let i = 0; i < 16; i++) {
      packet += target;
    }

    if (data.secret !== undefined) {
      packet += unhexlify(data.secret);
    }

    Net.send(
      Net.connect({
        addr: 'udp://255.255.255.255:9'
      }),
      packet
    );
  },
  null
);

Event.addGroupHandler(
  Net.EVENT_GRP,
  function(ev, evdata, arg) {
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
    print('Network event:', ev, evs);
  },
  null
);
