load('api_config.js');
load('api_events.js');
load('api_mqtt.js');
load('api_net.js');
load('api_sys.js');
load('api_timer.js');
load('api_wifi.js');

let find_device = ffi('void find_device_a(char*, void (*)(char*, userdata), userdata)');
let wake_device = ffi('void wake_device_a(char*)');

Timer.set(1000 * 10, Timer.REPEAT, function () {
  /*
  print('PING! Uptime:', Sys.uptime(), JSON.stringify({
    total_ram: Sys.total_ram(),
    free_ram: Sys.free_ram()
  }));
  */

  for (let i = 0; i < 15; i++) {
    find_device("408d5cb21c65", function (ipaddr, userdata) {
      //find_device("aabbccddeeff", function (ipaddr, userdata) {
      print("Found address: ", ipaddr, userdata);
    }, "some user data");

    wake_device("408d5cb21c65");
  }

}, null);
