// fixes and config
#include "options.h"
#include "sdkfix.h"

// essentials
#include <stdio.h>
#include <mgos.h>

// memory stuff
#include <lwip/mem.h>
#include <user_interface.h>
#include <esp_missing_includes.h>

// lwip stack
#include <lwip/opt.h>
#include <lwip/raw.h>
#include <lwip/netif.h>
#include <lwip/sys.h>
#include <lwip/timers.h>
#include <lwip/dhcp.h>
#include <lwip/app/ping.h>
#include <netif/etharp.h>
#include <ipv4/lwip/ip_addr.h>
#include <ipv4/lwip/ip.h>
#include <ipv4/lwip/icmp.h>

typedef void(*device_callback)(char *ip_addr, void *userdata);

struct ping_target {
  uint32               remaining_recv;
  struct eth_addr      *target_hwaddr;
  ip_addr_t            next_ipaddr;
  
  device_callback      callback;
  void                 *userdata;

  struct ping_target   *next;
} ping_target;

static struct ping_option *options = NULL;
static struct ping_target *target_head = NULL;

void recv_func(void *arg, void *pdata) {
  LOG(LL_INFO, ("recv_func"));
  (void) arg;
  (void) pdata;

  if (target_head != NULL) {
    if (--target_head->remaining_recv > 0) 
      return;

    struct eth_addr *found_hwaddr = NULL;
    ip_addr_t *found_ipaddr = NULL;

    // returns -1 if not in the arp table
    s8_t result = etharp_find_addr(
      &netif_list[STATION_IF],
      &target_head->next_ipaddr,
      &found_hwaddr,
      &found_ipaddr
    );

    if (result != -1) {
      LOG(LL_INFO, (
        "comparing found hwaddr %02hx:%02hx:%02hx:%02hx:%02hx:%02hx"
        "(%d.%d.%d.%d) with target %02hx:%02hx:%02hx:%02hx:%02hx:%02hx",

        found_hwaddr->addr[0], found_hwaddr->addr[1],
        found_hwaddr->addr[2], found_hwaddr->addr[3],
        found_hwaddr->addr[4], found_hwaddr->addr[5],

        IP2STR(&target_head->next_ipaddr),

        target_head->target_hwaddr->addr[0], target_head->target_hwaddr->addr[1],
        target_head->target_hwaddr->addr[2], target_head->target_hwaddr->addr[3],
        target_head->target_hwaddr->addr[4], target_head->target_hwaddr->addr[5]
      ));
    } else {
      LOG(LL_INFO, (
        "could not find hwaddr of %d.%d.%d.%d", 
        IP2STR(&target_head->next_ipaddr)
      ));
    }

    if (result != -1 && eth_addr_cmp(found_hwaddr, target_head->target_hwaddr)) {
      LOG(LL_INFO, ("hwaddrs matched, executing callback"));

      // found the target, execute callback and promote next
      struct ping_target *previous = target_head;

      previous->callback(
        ipaddr_ntoa(found_ipaddr), 
        target_head->userdata
      );

      target_head = previous->next;
      if (target_head != NULL) {
        options->ip = target_head->next_ipaddr.addr;
        ping_start(options);
      }

      free(previous->target_hwaddr);
      free(previous);
    } else {
      LOG(LL_INFO, ("hwaddrs did not match"));

      // todo: get directly from netif_list[STATION_IF]
      struct ip_info info;
      wifi_get_ip_info(STATION_IF, &info);

      ip_addr_t network_addr;
      network_addr.addr = info.ip.addr & info.netmask.addr;

      ip_addr_t broadcast_addr;
      broadcast_addr.addr = network_addr.addr | ~info.netmask.addr;

      ip_addr_t next_addr;
      next_addr.addr = ntohl(htonl(target_head->next_ipaddr.addr) + 1);

      if (next_addr.addr == broadcast_addr.addr) {
        // end of this target, execute callback with null address
        LOG(LL_INFO, ("end of target, calling callback"));

        struct ping_target *previous = target_head;
        previous->callback(NULL, previous->userdata);
	
        //promote next target and start
        target_head = previous->next;
        if (target_head != NULL) {
          LOG(LL_INFO, ("starting next target"));

          options->ip = target_head->next_ipaddr.addr;
          ping_start(options);
        }

        free(previous->target_hwaddr);
        free(previous);
      } else {
        // not what we wanted, but in range so next address
        LOG(LL_INFO, ("in range, next address"));

        target_head->next_ipaddr = next_addr;
        target_head->remaining_recv = options->count; // reset recv for next addr
        options->ip = next_addr.addr;
        ping_start(options);
      }
    }
  }
}

void sent_func(void *arg, void *pdata) {
  LOG(LL_INFO, ("sent_func"));
  (void) arg;
  (void) pdata;
}

void set_default_option(struct ping_option **opt_ret) {
  struct ping_option *options = NULL;
  options = (struct ping_option *) os_zalloc(sizeof(struct ping_option));
  //options->sent_function = sent_func;
  options->recv_function = recv_func;
  options->count = PING_COUNT;
  *opt_ret = options;
}

enum mgos_app_init_result mgos_app_init(void) {
  // todo: refractor (maybe instead of using ping we can use etharp_query)
  // if successful, we can use etharp_find_addr to get and compare MACs
  // possible pros: 
  // - much faster lookup
  // - simpler code structure
  // - safer (devices must reply to ARP, unlike ICMP)
  // possible cons:
  // - maybe it does not work
  // to try after first prototype is working
  set_default_option(&options);
  return MGOS_APP_INIT_SUCCESS;
}

void find_device(struct eth_addr *target_hwaddr, device_callback cb, void *userdata) {
  // todo: get directly from netif_list[STATION_IF]
  struct ip_info info;
  wifi_get_ip_info(STATION_IF, &info);

  ip_addr_t network_addr;
  network_addr.addr = info.ip.addr & info.netmask.addr;

  ip_addr_t first_addr;
  first_addr.addr = ntohl(htonl(network_addr.addr) + 1);

  struct ping_target *next = NULL;
  next = (struct ping_target *) os_zalloc(sizeof(struct ping_target));

  next->next_ipaddr = first_addr;
  next->target_hwaddr = target_hwaddr;
  next->remaining_recv = options->count;
  next->userdata = userdata;
  next->callback = cb;
  next->next = NULL;

  if (target_head != NULL) {
    // something in queue already, add to the last element in queue
    // the next element will be started when the current one ends
    struct ping_target *current = target_head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = next;
  } else {
    // first element in the queue, we must start immediately
    target_head = next;
    options->ip = target_head->next_ipaddr.addr;
    ping_start(options);
  }

  LOG(LL_INFO, ("added new target for %02hx:%02hx:%02hx:%02hx:%02hx:%02hx", 
    target_head->target_hwaddr->addr[0], target_head->target_hwaddr->addr[1],
    target_head->target_hwaddr->addr[2], target_head->target_hwaddr->addr[3],
    target_head->target_hwaddr->addr[4], target_head->target_hwaddr->addr[5]
  ));
}

void wake_device(struct eth_addr *target_hwaddr) {
  (void) target_hwaddr;
  
}

uint32 a2v(char c) {
  if (c >= 'A' && c <= 'F') {
    c += 32;
  }

  if (c >= '0' && c <= '9') {
    return c - '0';
  }

  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }

  return 0;
}

// converts ascii string to eth_addr
// returned pointer must be freed manually
struct eth_addr *a2hwaddr(char *str) {
  struct eth_addr *addr = os_zalloc(sizeof(struct eth_addr));
  addr->addr[0] = (a2v(str[0]) << 4) + a2v(str[1]);
  addr->addr[1] = (a2v(str[2]) << 4) + a2v(str[3]);
  addr->addr[2] = (a2v(str[4]) << 4) + a2v(str[5]);
  addr->addr[3] = (a2v(str[6]) << 4) + a2v(str[7]);
  addr->addr[4] = (a2v(str[8]) << 4) + a2v(str[9]);
  addr->addr[5] = (a2v(str[10]) << 4) + a2v(str[11]);
  return addr;
}

void find_device_a(char *target_hwaddr, device_callback cb, void *userdata) {
  find_device(
    a2hwaddr(target_hwaddr), 
    cb, 
    userdata
  );
}

void wake_device_a(char *target_hwaddr) {
  wake_device(a2hwaddr(target_hwaddr));
}

