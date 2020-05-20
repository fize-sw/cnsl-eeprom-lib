/************************************************************************
| Copyright (c) OSR Enterprises AG, 2018.     All rights reserved.
|
| This software is the exclusive and confidential property of
| OSR Enterprises AG and may not be disclosed, reproduced or modified
| without permission from OSR Enterprises AG.
|
| Description: read/write EEPROMs
|		library function get_db_eeprom_params()
|
|
************************************************************************/

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

#define I2C_DEV_NAME "/dev/gmbp-i2c"
#define I2C_READ _IOR('g', 1, struct udata *)
#define I2C_WRITE _IOW('g', 2, struct udata *)
#define I2C_ADDR 0x50
#define INT_SIZE 4
#define STR_SIZE 16
#define STR_DSIZE 32
#define STRMAC_SIZE 8

#define MAX_EEPROM_SIZE 0x200
#define EEPROM_PATTERN 0x5a5aa5a5
#define MAC_SIZE 6

#define _PACKED __attribute__((packed))

#define flash_start(eb) (uint8_t *)eb->location.__params
#define params(eb) ((eeprom_param_t *)eb->location.__params)
#define pattern(eb) htonl(params(eb)->pattern)
#define version(eb) htonl(params(eb)->eeprom_ver_num)
#define platform(eb) params(eb)->platform
#define catalog(eb) params(eb)->catalog
#define serial(eb) params(eb)->serial
#define assy(eb) params(eb)->assy_hw_ver
#define mac(eb) params(eb)->mac
#define mac_n(eb) htonl(params(eb)->num_macs)
#define crc_32(eb) htonl(params(eb)->crc)

#define pattern_offset 0
#define version_offset pattern_offset + sizeof(pattern(eb))
#define platform_offset version_offset + sizeof(version(eb))
#define catalog_offset platform_offset + sizeof(platform(eb))
#define serial_offset catalog_offset + sizeof(catalog(eb))
#define assy_offset serial_offset + sizeof(serial(eb))
#define mac_offset assy_offset + sizeof(assy(eb))
#define mac_n_offset mac_offset + sizeof(mac(eb))
#define crc_32_offset mac_n_offset + sizeof(mac_n(eb))

//#ifndef EEPROM_H_
//
///* return codes */
typedef enum {
  EEP_OK = 0,
  EEP_ERR_FAILED,
  EEP_ERR_INV_PARAM, /* invalid param value */
  EEP_ERR_READ,      /* read error */
  EEP_ERR_WRITE,     /* write error */

  EEP_ERR_MAX
} Eeprom_Rc;

typedef enum {
  EEP_BRD_NUM_UNKNOWN = 0,
  EEP_BRD_NUM_1 = 1,
  EEP_BRD_NUM_2 = 2,
  EEP_BRD_NUM_3 = 3,
  EEP_BRD_NUM_4 = 4,
  EEP_BRD_NUM_5 = 5,
  EEP_BRD_NUM_6 = 6,
  EEP_BRD_NUM_MB = 14,
  EEP_BRD_NUM_TOP = 15,

  /* Always last*/
  EEP_BRD_NUM_MAX
} Eeprom_BrdNum;

struct udata {
  uint8_t bus, reg, val, addr;
};
#pragma pack(1)
typedef struct mac { uint8_t mac[MAC_SIZE]; } mac_t;
#pragma pack(0)

typedef uint16_t e_version_t;
typedef int32_t e_crc_t;

typedef struct {
  uint32_t pattern;
  uint32_t eeprom_ver_num;
  char platform[STR_SIZE];
  char catalog[STR_DSIZE];
  char serial[STR_SIZE];
  char assy_hw_ver[STR_SIZE];
  char mac[STRMAC_SIZE];
  uint32_t num_macs;
  uint32_t crc;
} _PACKED eeprom_param_t;

typedef struct eeprom_buffer_s {
  Eeprom_BrdNum device;
  char data[MAX_EEPROM_SIZE];
  struct ebp_s {
    eeprom_param_t *__params;
  } location;
} _PACKED eb_t;

static size_t get_eeprom_size();
static const char *dev2Filename(Eeprom_BrdNum dev);
static int str2mac(const char *const str, char *mac);
static u_int32_t calculate_crc32(const uint8_t *const data, const int size);
// static int get_struct_crc32(eb_t *eb);
static int save_to_file();
static int load_eeprom_bin();
static Eeprom_Rc get_eeprom_params(Eeprom_BrdNum board, eb_t *eb);
static Eeprom_Rc read_eeprom(eb_t *eb, int *fd);
static Eeprom_Rc write_eeprom(eb_t *eb, uint8_t e_off, uint8_t size);
static Eeprom_Rc write_eeprom_bin(eb_t *eb);
static Eeprom_Rc write_db_eeprom(uint8_t bus, uint8_t e_off, uint8_t *val,
                                 uint8_t size);

Eeprom_Rc load_eeprom_to_cache(Eeprom_BrdNum board);
Eeprom_Rc flush_cache(Eeprom_BrdNum board);
Eeprom_Rc get_eeprom_all(Eeprom_BrdNum board, eeprom_param_t *params);
Eeprom_Rc get_eep_version(Eeprom_BrdNum board, uint32_t *value);
Eeprom_Rc get_eep_platform(Eeprom_BrdNum board, char *value);
Eeprom_Rc get_eep_catalog(Eeprom_BrdNum board, char *value);
Eeprom_Rc get_eep_serial(Eeprom_BrdNum board, char *value);
Eeprom_Rc get_eep_assy(Eeprom_BrdNum board, char *value);
Eeprom_Rc get_eep_mac(Eeprom_BrdNum board, char *value);
Eeprom_Rc get_eep_num_macs(Eeprom_BrdNum board, uint32_t *value);
uint32_t crc_eeprom_calc(eb_t *eb);

Eeprom_Rc set_eep_version(Eeprom_BrdNum board, uint32_t *value);
Eeprom_Rc set_eep_platform(Eeprom_BrdNum board, char *value);
Eeprom_Rc set_eep_catalog(Eeprom_BrdNum board, char *value);
Eeprom_Rc set_eep_serial(Eeprom_BrdNum board, char *value);
Eeprom_Rc set_eep_assy(Eeprom_BrdNum board, char *value);
Eeprom_Rc set_eep_mac(Eeprom_BrdNum board, char *value);
Eeprom_Rc set_eep_num_macs(Eeprom_BrdNum board, uint32_t *value);
