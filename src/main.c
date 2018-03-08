#include <stdio.h>
#include "mgos.h"

enum mgos_app_init_result mgos_app_init(void) {
  return MGOS_APP_INIT_SUCCESS;
}

int a2v(char c) {
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

char *unhexlify(char *input) {
  size_t len = strlen(input);
  char *output = malloc((len / 2 + 1) * sizeof(char));
  char *ptr = output;
  
  for (size_t i = 0; i < len; i += 2) {
    *ptr++ = (char) ((a2v(input[i]) << 4) + a2v(input[i + 1]));
  }
  
  *ptr = '\0';
  return output;
}