#ifndef USER_OPTIONS_H
#define USER_OPTIONS_H

// enables etharp_raw
// todo: move to mos.yml (?)
#define LWIP_AUTOIP 1

// count of pings to send/receive
#define PING_COUNT 3

// ping receive timeout - in milliseconds
#define PING_RCV_TIME0 200

// todo: why are there two?
#define PING_TIMEOUT_MSG 200

// ping delay - in milliseconds
#define PING_COARSE 100

#endif
