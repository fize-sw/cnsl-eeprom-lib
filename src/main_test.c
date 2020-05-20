#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

//#include <glv-defines.h>

//#include <eeprom.h>
#include "eeplib.h"

static char *progname;

/*!
 * Start program
 * @param[in] argc  Number of arguments
 * @param[in] argv  Pointer to array of arguments
 * @return 0 for succsess and other for failure
 */
int main(int argc, char **argv) {
  int board_num = 6;
  char catalog_s[STR_DSIZE];
  char *catalog = &catalog_s[0];
  char mac_s[STRMAC_SIZE];
  char *mac = &mac_s[0];
  printf("argc value:%d\n", argc);
  if (argc == 2) {
    board_num = strtol(argv[1], NULL, 10);

    if (get_eep_catalog(board_num, catalog) != -1)
      printf("Board %d:\nCatalog function: catalog Name    : %s\n", board_num,
             catalog);
    if (get_eep_mac(board_num, mac) != -1) {
      printf("mac address:    : %x:%x:%x:%x:%x:%x\n", (uint8_t)mac[0],
             (uint8_t)mac[1], (uint8_t)mac[2], (uint8_t)mac[3], (uint8_t)mac[4],
             (uint8_t)mac[5]);
    }
    printf("read done!\n");

    return 0;
  } else if (argc > 2) {
    board_num = strtol(argv[1], NULL, 10);
    printf("Board %d:\n", board_num);
    if (set_eep_catalog(board_num, argv[2]) != -1)
      printf("Catalog write successfull!\n");

    if (get_eep_catalog(board_num, catalog) != -1) {
      printf("catalog retreived value    : \"%s\"\n", catalog);
    }
    if (argc > 3) {
      if (set_eep_mac(board_num, argv[3]) != -1)
        printf("mac write successful!\n");
      if (get_eep_mac(board_num, mac) != -1) {
        printf("mac retreived:    : %x:%x:%x:%x:%x:%x\n", (uint8_t)mac[0],
               (uint8_t)mac[1], (uint8_t)mac[2], (uint8_t)mac[3],
               (uint8_t)mac[4], (uint8_t)mac[5]);
        printf("write  done!\n");
      }

      return 0;
    }
    return 1;
  }
}
