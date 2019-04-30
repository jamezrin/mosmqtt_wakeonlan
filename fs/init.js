load('api_config.js');
load('api_events.js');
load('api_mqtt.js');
load('api_net.js');
load('api_sys.js');
load('api_timer.js');
load('api_wifi.js');

let find_device = ffi('void find_device(char*, void (*)(char*, userdata), userdata)');
let wake_device = ffi('void wake_device(char*)');

Timer.set(1000 * 1, Timer.REPEAT, function () {
  print('PING! Uptime:', Sys.uptime(), JSON.stringify({
    total_ram: Sys.total_ram(),
    free_ram: Sys.free_ram()
  }));
}, null);
