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
  char                 *target_hwaddr;
  ip_addr_t            next_ipaddr;
  device_callback      callback;
  struct ping_target   *next;
} ping_target;

static struct ping_option *options = NULL;
static struct ping_target *target_head = NULL;

// converts ascii string to eth_addr
struct eth_addr* atoeth(char *str) {
  (void) str;
  return NULL;
}

void recv_func(void *arg, void *pdata) {
  (void) arg;
  (void) pdata;

  LOG(LL_INFO, ("recv_func"));

  if (target_head != NULL) {
    // todo: require a minimum of packets received
//    if (--target_head->remaining_recv > 0) 
//      return;

    struct eth_addr *found_hwaddr = NULL;
    ip_addr_t *found_ipaddr = NULL;

    // returns -1 if not in the arp table
    s8_t result = etharp_find_addr(
      &netif_list[STATION_IF],
      &target_head->next_ipaddr,
      &found_hwaddr,
      &found_ipaddr
    );

    // todo: compare an eth_addr with the char* hwaddr
    // have to figure out converting
    if (result != -1 && false /* todo: compare if MACs are equal */) {
      target_head->callback(ipaddr_ntoa(found_ipaddr), NULL); // todo: pass mac in userdata
    
      // found the target, promote next target and start
      struct ping_target *previous = target_head;
      if (previous->next != NULL) {
        target_head = previous->next;
        options->ip = target_head->next_ipaddr.addr;
        ping_start(options);
      }

      free(previous);
    } else {
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
        // end of this target, execute callback with error
	struct ping_target *previous = target_head;
	previous->callback(NULL, NULL); // todo: pass mac in userdata
	
	//promote next target and start
        target_head = previous->next;
	if (target_head != NULL) {
          options->ip = target_head->next_ipaddr.addr;
	  ping_start(options);
        }
        
        free(previous);
      } else {
        // not what we wanted, but in range so next address
        target_head->next_ipaddr = next_addr;
	target_head->remaining_recv = options->count; // reset recv for next addr
        options->ip = next_addr.addr;
	ping_start(options);
      }
    }
  }
}

void sent_func(void *arg, void *pdata) {
  (void) arg;
  (void) pdata;
  LOG(LL_INFO, ("sent_func"));
}

void set_default_option(struct ping_option **opt_ret) {
  struct ping_option *options = NULL;
  options = (struct ping_option *) os_zalloc(sizeof(struct ping_option));
  options->sent_function = sent_func;
  options->recv_function = recv_func;
  options->count = PING_COUNT;
  *opt_ret = options;
}

enum mgos_app_init_result mgos_app_init(void) {
  set_default_option(&options);
  return MGOS_APP_INIT_SUCCESS;
}

void test_ping(char *string) {
  options->ip = ipaddr_addr(string);
  ping_start(options);
}

void find_device(char *target_hwaddr, device_callback cb, void *userdata) {
  (void) userdata;

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
  next->callback = cb;
  next->next = NULL;

  if (target_head != NULL) {
    // something in queue already, add to the last element in queue
    struct ping_target *current = target_head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = next;
  } else {
    // first element in the queue, must start manually
    target_head = next;
    options->ip = target_head->next_ipaddr.addr;
    ping_start(options);
  }
}

void wake_device(char *target_hwaddr) {
  (void) target_hwaddr;
}

