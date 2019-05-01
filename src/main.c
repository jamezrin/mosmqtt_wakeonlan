// fixes and config
#include "options.h"
#include "sdkfix.h"

// essentials
#include <stdio.h>
#include <mgos.h>
#include <mgos_mongoose.h>

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

// todo: include some error parameter to know the result
typedef void(*device_callback)(char *ip_addr, void *userdata);

struct ping_target {
  u32_t                remaining_recv;
  struct eth_addr      target_hwaddr;
  ip_addr_t            current_ipaddr;

  device_callback      callback;
  void                 *userdata;

  struct ping_target   *next;
} ping_target;

static struct ping_option *options = NULL;
static struct ping_target *target_head = NULL;

void recv_func(void *arg, void *pdata) {
  LOG(LL_DEBUG, ("recv_func"));
  (void) arg; (void) pdata;

  if (target_head != NULL) {
    if (--target_head->remaining_recv > 0) 
      return;

    struct eth_addr *found_hwaddr = NULL;
    ip_addr_t *found_ipaddr = NULL;

    struct netif sta_iface = netif_list[STATION_IF];

    // returns -1 if not in the arp table
    s8_t result = etharp_find_addr(
      &sta_iface,
      &target_head->current_ipaddr,
      &found_hwaddr,
      &found_ipaddr
    );

    if (result != -1) {
      LOG(LL_INFO, (
        "comparing found hwaddr " MACSTR 
        " (" IPSTR ") with target " MACSTR,

        MAC2STR(found_hwaddr->addr),
        IP2STR(&target_head->current_ipaddr),
        MAC2STR(target_head->target_hwaddr.addr)
      ));
    } else {
      LOG(LL_INFO, (
        "could not find hwaddr of " IPSTR, 
        IP2STR(&target_head->current_ipaddr)
      ));
    }

    if (result != -1 && eth_addr_cmp(found_hwaddr, &target_head->target_hwaddr)) {
      LOG(LL_INFO, ("hwaddrs matched, executing callback"));

      // found the target, execute callback and promote next
      struct ping_target *previous = target_head;

      previous->callback(
        ipaddr_ntoa(found_ipaddr), 
        target_head->userdata
      );

      target_head = previous->next;
      if (target_head != NULL) {
        options->ip = target_head->current_ipaddr.addr;
        ping_start(options);
      }

      free(previous);
    } else {
      LOG(LL_INFO, ("hwaddrs did not match"));

      ip_addr_t network_ipaddr;
      network_ipaddr.addr = sta_iface.ip_addr.addr & sta_iface.netmask.addr;

      ip_addr_t broadcast_ipaddr;
      broadcast_ipaddr.addr = network_ipaddr.addr | ~sta_iface.netmask.addr;

      ip_addr_t next_ipaddr;
      next_ipaddr.addr = ntohl(htonl(target_head->current_ipaddr.addr) + 1);

      if (next_ipaddr.addr == broadcast_ipaddr.addr) {
        // end of this target, execute callback with null address
        LOG(LL_INFO, ("end of target, executing callback"));

        struct ping_target *previous = target_head;
        previous->callback(NULL, previous->userdata);
	
        //promote next target and start
        target_head = previous->next;
        if (target_head != NULL) {
          LOG(LL_INFO, ("starting next target"));

          options->ip = target_head->current_ipaddr.addr;
          ping_start(options);
        }

        free(previous);
      } else {
        // not what we wanted, but in range so next address
        LOG(LL_INFO, ("in range, next address (%u remaining)", 
          htonl(broadcast_ipaddr.addr) - htonl(next_ipaddr.addr)
        ));

        target_head->current_ipaddr = next_ipaddr;
        target_head->remaining_recv = options->count; // reset recv for next addr
        options->ip = next_ipaddr.addr;
        ping_start(options);
      }
    }
  }
}

void sent_func(void *arg, void *pdata) {
  LOG(LL_DEBUG, ("sent_func"));
  (void) arg; (void) pdata;
}

void set_default_option(struct ping_option **opt_ret) {
  struct ping_option *options = NULL;
  options = (struct ping_option *) os_zalloc(sizeof(struct ping_option));
  #if EMPTY_SENT_FUNC
  options->sent_function = sent_func; 
  #endif
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

  // todo: event handler if network changes, reset queue
  set_default_option(&options);
  return MGOS_APP_INIT_SUCCESS;
}

void clear_list(void) {
  struct ping_target *current = target_head;

  // promote next and free previous
  while (current != NULL) {
    struct ping_target *previous = current;
    current = previous->next;
    
    free(previous);
  }

  target_head = NULL;
}

void find_device(struct eth_addr target_hwaddr, device_callback cb, void *userdata) {
  struct netif *sta_if = &netif_list[STATION_IF];

  ip_addr_t network_ipaddr;
  network_ipaddr.addr = sta_if->ip_addr.addr & sta_if->netmask.addr;

  ip_addr_t first_addr;
  first_addr.addr = ntohl(htonl(network_ipaddr.addr) + 1);

  struct ping_target *next = NULL;
  next = (struct ping_target *) os_zalloc(sizeof(struct ping_target));

  next->current_ipaddr = first_addr;
  next->target_hwaddr = target_hwaddr;
  next->remaining_recv = options->count;
  next->userdata = userdata;
  next->callback = cb;
  next->next = NULL;

  u32_t position = 0;
  if (target_head != NULL) {
    // something in queue already, add to the last element in queue
    // the next element will be started when the current one ends
    struct ping_target *current = target_head;
    while (current->next != NULL) {
      position++;
      current = current->next;
    }
    position++;
    current->next = next;
  } else {
    // first element in the queue, we must start immediately
    target_head = next;
    options->ip = target_head->current_ipaddr.addr;
    ping_start(options);
  }

  LOG(LL_INFO, ("added new target for " MACSTR " at position %u",
    MAC2STR(target_head->target_hwaddr.addr), position));
}

// todo: ...
// use mg_connect and mg_send
void wake_device(struct eth_addr target_hwaddr) {
  //mg_connect()
  //mg_send()
  (void) target_hwaddr;
}

u32_t a2v(char c) {
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
struct eth_addr a2hwaddr(char *str) {
  struct eth_addr hwaddr = {{
    (a2v(str[0]) << 4) + a2v(str[1]),
    (a2v(str[2]) << 4) + a2v(str[3]),
    (a2v(str[4]) << 4) + a2v(str[5]),
    (a2v(str[6]) << 4) + a2v(str[7]),
    (a2v(str[8]) << 4) + a2v(str[9]),
    (a2v(str[10]) << 4) + a2v(str[11])
  }};
  return hwaddr;
}

// todo: ensure valid string
// takes an ascii with chars representing the hex values
void find_device_a(char *target_hwaddr, device_callback cb, void *userdata) {
  find_device(
    a2hwaddr(target_hwaddr), 
    cb,
    userdata
  );
}

// todo: ensure valid string
// takes an ascii with chars representing the hex values
void wake_device_a(char *target_hwaddr) {
  wake_device(a2hwaddr(target_hwaddr));
}
