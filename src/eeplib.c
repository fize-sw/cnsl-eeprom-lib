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

#include <eeplib.h>
#include <stdint.h>

static size_t get_eeprom_size() { return (sizeof(eeprom_param_t)); }

// device - file name convert
static const char *dev2Filename(Eeprom_BrdNum dev)
{
	static const char *devFilenames[EEP_BRD_NUM_MAX] = {
	    "na",
	    "/run/eeprom_db_cache1.bin",
	    "/run/eeprom_db_cache2.bin",
	    "/run/eeprom_db_cache3.bin",
	    "/run/eeprom_db_cache4.bin",
	    "/run/eeprom_db_cache5.bin",
	    "/run/eeprom_db_cache6.bin",
	    "na",
	    "na",
	    "na",
	    "na",
	    "na",
	    "na",
	    "na",
	    "/sys/class/i2c-dev/i2c-2/device/2-0054/eeprom",
	    "/sys/class/i2c-dev/i2c-5/device/5-0050/eeprom"};

	// TODO: Add device/file name validation here
	// NAME_VALIDATION(dev, Eeprom_BrdNum);
	return devFilenames[dev];
}
// convert string mac (xx:xx:xx:xx:xx:xx) to char array mac
static int str2mac(const char *const str, char *mac)
{
	unsigned int m[MAC_SIZE];
	int rc;

	memset((void *)mac, 0, 8);
	rc = sscanf(str, "%x:%x:%x:%x:%x:%x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
	if (rc == MAC_SIZE) {
		int i = MAC_SIZE;
		while (--i >= 0) {
			mac[i] = m[i];
		}
	} else {
		syslog(LOG_EMERG, "error parsing string <%s>", str);
		rc = -1;
	}
	return rc;
}

static u_int32_t Crc32_ComputeBuf(u_int32_t crc32, const void *buf, int buflen)
{
	static const u_int32_t crcTable[256] = {
	    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535,
	    0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD,
	    0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D,
	    0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4,
	    0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
	    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC,
	    0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB,
	    0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
	    0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB,
	    0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA,
	    0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE,
	    0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A,
	    0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409,
	    0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
	    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739,
	    0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268,
	    0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
	    0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8,
	    0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF,
	    0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703,
	    0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7,
	    0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE,
	    0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
	    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6,
	    0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D,
	    0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5,
	    0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605,
	    0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D};
	unsigned char *pbuf = (unsigned char *)buf;
	int i;
	int iLookup;
	for (i = 0; i < buflen; i++) {
		iLookup = (crc32 & 0xFF) ^ (*pbuf++);
		crc32 = ((crc32 & 0xFFFFFF00) >> 8) & 0xFFFFFF; // ' nasty shr 8 with vb :/
		crc32 = crc32 ^ crcTable[iLookup];
	}
	return crc32;
}

static u_int32_t calculate_crc32(const uint8_t *const buf, const int buflen)
// u_int32_t crc32_of_buffer(const char *buf, int buflen)
{
	return Crc32_ComputeBuf(0xFFFFFFFF, buf, buflen) ^ 0xFFFFFFFF;
}

// save cache to file
static int save_to_file(eb_t *eb)
{
	FILE *fp;
	size_t written_bytes;

	if (!(fp = fopen(dev2Filename(eb->device), "wb"))) {
		syslog(LOG_EMERG, "Failed to open EEPROM file %s: %s", dev2Filename(eb->device),
		       strerror(errno));
		return -1;
	}
	written_bytes = fwrite(eb->data, sizeof(eeprom_param_t), 1, fp);
	fclose(fp);

	if (written_bytes < 1) {
		fprintf(stderr, "Writing to file failed.\n");
		syslog(LOG_EMERG, "Failed to write to EEPROM file %s: %s", dev2Filename(eb->device),
		       strerror(errno));
		return -1;
	}
	return 0;
}
#if 0
Eeprom_Rc flush_cache(Eeprom_BrdNum board)
{
	struct stat st;
	syslog(LOG_INFO, "flush_cache:%d deleted\n", (int)board);
	if (stat(dev2Filename(board), &st) == -1) {
		syslog(LOG_INFO, "cache to board:%d is not exist...\n", (int)board);
		return -1;
	} else {
		remove(dev2Filename(board));
		syslog(LOG_INFO, "cache to board:%d deleted\n", (int)board);
		return 0;
	}
	return 0;
}

#endif

Eeprom_Rc load_eeprom_to_cache(Eeprom_BrdNum board)
{
	int i;
	int fd = 0;
	int rc;
	FILE *fp;

	int size = (int)get_eeprom_size();
	struct udata data;
	char buf[MAX_EEPROM_SIZE];
	if (!(fd = open(I2C_DEV_NAME, O_RDWR))) {
		return -1;
	}

	// syslog(LOG_INFO, "bus id is: %d\n", (int)board);
	data.addr = I2C_ADDR;	/* I2C hard coded address*/
	data.bus = ((int)(board)-1); /* Bus numbers is 0-5 for daughter cards 1-6 */
	data.reg = 0;		     /* read address - eeprom offset*/

	for (i = 0; i < size; i++) {
		if (ioctl(fd, I2C_READ, &data) == -1) {
			syslog(LOG_EMERG, "I2C communication with daughter board failed");
			return EEP_ERR_FAILED;
		}
		buf[i] = data.val;
		// printf("%03d: %02hhx\n", data.reg, data.val);
		data.reg++;
	}

	if (!(fp = fopen(dev2Filename(board), "wb+"))) {
		syslog(LOG_EMERG, "Failed to open EEPROM device %s: %s", dev2Filename(board),
		       strerror(errno));
		return EEP_ERR_FAILED;
	}

	if ((rc = fwrite(buf, get_eeprom_size(), 1, fp)) > (int)get_eeprom_size()) {
		syslog(LOG_EMERG, "Failed to read EEPROM version %d %d", rc,
		       (uint32_t)get_eeprom_size());
	}

	if ((rc = fclose(fp)) < 0) {
		syslog(LOG_EMERG, "Failed to close EEPROM device %s: %s.", dev2Filename(board),
		       strerror(errno));
		return EEP_ERR_FAILED;
	}
	return EEP_OK;
}

// determine if read from MB/TOP/DB and save all eeprom content into eb struct
static int load_eeprom_bin(eb_t *eb, int *fd)
{
	int file_d = 0;

	if ((eb->device == EEP_BRD_NUM_MB) || (eb->device == EEP_BRD_NUM_TOP)) {
		int rc;
		FILE *fp;
		syslog(LOG_INFO, "Open EEPROM device %s", dev2Filename(eb->device));

		if (!(fp = fopen(dev2Filename(eb->device), "rb+"))) {
			syslog(LOG_EMERG, "Failed to open EEPROM device %s: %s",
			       dev2Filename(eb->device), strerror(errno));
			return -1;
		}

		if ((rc = fread(eb->data, get_eeprom_size(), 1, fp)) > (int)get_eeprom_size()) {
			syslog(LOG_EMERG, "Failed to read EEPROM version %d %d", rc,
			       (uint32_t)get_eeprom_size());
		}

		if ((rc = fclose(fp)) < 0) {
			syslog(LOG_EMERG, "Failed to close EEPROM device %s: %s.",
			       dev2Filename(eb->device), strerror(errno));
			return -1;
		}
	} else {
		int i;
		int size = (int)get_eeprom_size();
		struct udata data;
		file_d = open(I2C_DEV_NAME, O_RDWR);

		if (file_d == 0) {
			fd = 0;
			return -1;
		}

		// syslog(LOG_INFO, "bus id is: %d\n", (int)eb->device);
		// printf("bus id is: %d\n", eb->device);
		data.addr = I2C_ADDR; /* I2C hard coded address*/
		/* Bus numbers is 0-5 for daughter cards 1-6 */
		data.bus = ((int)(eb->device) - 1);
		data.reg = 0; /* read address - eeprom offset*/

		for (i = 0; i < size; i++) {
			if (ioctl(file_d, I2C_READ, &data) == -1) {
				syslog(LOG_EMERG, "I2C communication with daughter board failed");
				return -1;
			}
			eb->data[i] = data.val;
			// printf("%03d: %02hhx\n", data.reg, data.val);
			data.reg++;
		}
		// printf("\n");
		save_to_file(eb);
	}
	//
	eb->location.__params = (eeprom_param_t *)eb->data;
	fd = &file_d;
	return 0;
}

// check parameters and read all eeprom to eb struct
static Eeprom_Rc read_eeprom(eb_t *eb, int *fd)
{
	int int_input_param = 0;
	// syslog(LOG_INFO, "board [%d]\n", (int)(eb->device));

	if (eb->device < EEP_BRD_NUM_1 || eb->device > EEP_BRD_NUM_MAX) {
		syslog(LOG_EMERG, "Wrong board [%d] number. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else if (eb->device > EEP_BRD_NUM_6 && eb->device < EEP_BRD_NUM_MB) {
		syslog(LOG_EMERG, "Board [%d] not exist. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else {
		// Check mandatory parameters
		// syslog(LOG_INFO, "device %d\n", (int)(eb->device));
		// get required string
		if (load_eeprom_bin(eb, fd) == -1) {
			return EEP_ERR_READ;
		}
		if (pattern(eb) != EEPROM_PATTERN) {
			///*check pattern*/
			syslog(LOG_EMERG, "check pattern failed - set new pattern");
			int_input_param = htonl(EEPROM_PATTERN);
			params(eb)->pattern = int_input_param;
			write_eeprom(eb, pattern_offset, INT_SIZE);
		}
		return EEP_OK;
	}
}

// get all eeprom content from a specific board
static Eeprom_Rc get_eeprom_params(Eeprom_BrdNum board, eb_t *eb)
{
	int fd = 0;
	eb->device = board;
	return read_eeprom(eb, &fd);
}

static Eeprom_Rc get_eeprom_params_ex(Eeprom_BrdNum board, eb_t *eb, uint32_t offset, uint32_t size)
{
	int rc;
	int i;
	FILE *fp;
	eb->device = board;
	syslog(LOG_INFO, "board [%d]\n", (int)(eb->device));
	if (eb->device < EEP_BRD_NUM_1 || eb->device > EEP_BRD_NUM_MAX) {
		syslog(LOG_EMERG, "Wrong board [%d] number. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else if (eb->device > EEP_BRD_NUM_6 && eb->device < EEP_BRD_NUM_MB) {
		syslog(LOG_EMERG, "Board [%d] not exist. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else {
		if ((eb->device == EEP_BRD_NUM_MB) || (eb->device == EEP_BRD_NUM_TOP)) {
			syslog(LOG_INFO, "get_eeprom_params_ex:Open EEPROM device %s off %d",
			       dev2Filename(eb->device), offset);
			if (!(fp = fopen(dev2Filename(eb->device), "rb+"))) {
				syslog(LOG_EMERG, "Failed to open EEPROM device %s: %s",
				       dev2Filename(eb->device), strerror(errno));
				return -1;
			}
			fseek(fp, offset, SEEK_SET);
			if ((rc = fread(eb->data + offset, size, 1, fp)) > size) {
				syslog(LOG_EMERG, "Failed to read EEPROM version %d %d", rc,
				       (uint32_t)get_eeprom_size());
			}

			if ((rc = fclose(fp)) < 0) {
				syslog(LOG_EMERG, "Failed to close EEPROM device %s: %s.",
				       dev2Filename(eb->device), strerror(errno));
				return -1;
			}
		} else {
			int fd = 0;
			struct udata data;
			if (!(fd = open(I2C_DEV_NAME, O_RDWR))) {
				return -1;
			}

			// syslog(LOG_INFO, "bus id is: %d\n", (int)eb->device);
			data.addr = I2C_ADDR;
			data.bus = ((int)(eb->device) - 1);
			data.reg = offset;

			for (i = 0; i < size; i++) {
				if (ioctl(fd, I2C_READ, &data) == -1) {
					syslog(LOG_EMERG,
					       "I2C communication with daughter board failed");
					return -1;
				}
				eb->data[offset + i] = data.val;
				// printf("%03d: %02hhx %c \n", data.reg, data.val, data.val);
				data.reg++;
			}
			syslog(LOG_INFO, "Read %d chars from device.", i);
		}
		eb->location.__params = (eeprom_param_t *)eb->data;
		return EEP_OK;
	}
}

static Eeprom_Rc get_eeprom_params_cached(Eeprom_BrdNum board, eb_t *eb, uint32_t offset,
					  uint32_t size)
{
	int rc;
	int i;
	FILE *fp;
	if (eb->device < EEP_BRD_NUM_1 || eb->device > EEP_BRD_NUM_MAX) {
		syslog(LOG_EMERG, "Wrong board [%d] number. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else if (eb->device > EEP_BRD_NUM_6 && eb->device < EEP_BRD_NUM_MB) {
		syslog(LOG_EMERG, "Board [%d] not exist. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else {
		// syslog(LOG_INFO, "get_eeprom_params_cached:Open EEPROM device %s off: %d size:
		// %d", dev2Filename(eb->device), offset, size);
		if (!(fp = fopen(dev2Filename(eb->device), "rb+"))) {
			syslog(LOG_EMERG, "Failed to open EEPROM device %s: %s",
			       dev2Filename(eb->device), strerror(errno));
			if ((rc = get_eeprom_params_ex(board, eb, offset, size)) == EEP_OK) {
				syslog(LOG_INFO, "Read eeprom from i2c");
				eb->location.__params = (eeprom_param_t *)eb->data;
				return EEP_OK;
			} else {
				return -1;
			}
		}

		fseek(fp, offset, SEEK_SET);
		if ((rc = fread(eb->data + offset, size, 1, fp)) > size) {
			syslog(LOG_EMERG, "Failed to read EEPROM  rc:%d offset:%d : %d", rc, offset,
			       size);
		}

		if ((rc = fclose(fp)) < 0) {
			syslog(LOG_EMERG, "Failed to close EEPROM device %s: %s.",
			       dev2Filename(eb->device), strerror(errno));
			return -1;
		}
	}
	eb->location.__params = (eeprom_param_t *)eb->data;
#if 0
	for (i = 0; i < size; i++) {
		syslog(LOG_INFO, "%02hhx:", eb->data[offset + i]);
	}
	syslog(LOG_INFO ,"\n");
#endif
	return EEP_OK;
}

static Eeprom_Rc set_eeprom_params_cached(Eeprom_BrdNum board, eb_t *eb, uint32_t offset,
					  uint32_t size)
{
	int rc;
	int i;
	FILE *fp;
	if (eb->device < EEP_BRD_NUM_1 || eb->device > EEP_BRD_NUM_MAX) {
		syslog(LOG_EMERG, "Wrong board [%d] number. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else if (eb->device > EEP_BRD_NUM_6 && eb->device < EEP_BRD_NUM_MB) {
		syslog(LOG_EMERG, "Board [%d] not exist. Exit.", (int)(eb->device));
		return EEP_ERR_INV_PARAM;
	} else {
		// syslog(LOG_INFO, "get_eeprom_params_cached:Open EEPROM device %s off: %d size:
		// %d", dev2Filename(eb->device), offset, size);
		if (!(fp = fopen(dev2Filename(eb->device), "rb+"))) {
			syslog(LOG_EMERG, "Failed to open EEPROM device %s: %s",
			       dev2Filename(eb->device), strerror(errno));
			return -1;
		}

		fseek(fp, offset, SEEK_SET);
		if ((rc = fwrite(eb->data + offset, size, 1, fp)) > size) {
			syslog(LOG_EMERG, "Failed to write EEPROM  rc:%d offset:%d : %d", rc,
			       offset, size);
		}

		if ((rc = fclose(fp)) < 0) {
			syslog(LOG_EMERG, "Failed to close EEPROM device %s: %s.",
			       dev2Filename(eb->device), strerror(errno));
			return -1;
		}
	}
	syslog(LOG_INFO, "cache updated..\n");
	return EEP_OK;
}

// write full eeprom - all boards
static Eeprom_Rc write_eeprom(eb_t *eb, uint8_t e_off, uint8_t size)
{
	Eeprom_Rc rc = EEP_OK;
	if ((eb->device == EEP_BRD_NUM_MB) || (eb->device == EEP_BRD_NUM_TOP)) {
		rc = write_eeprom_bin(eb);
	} else {
		if ((rc = write_db_eeprom((uint8_t)(eb->device - 1), e_off, flash_start(eb) + e_off,
					  size)) == EEP_OK) {
			int32_t crc32;
			const int size =
			    sizeof(eeprom_param_t) - sizeof(uint32_t); // substruct crc field
			crc32 = calculate_crc32((uint8_t *)eb->data, size);
			params(eb)->crc = crc32;
			if (write_db_eeprom((uint8_t)(eb->device - 1), crc_32_offset,
					    flash_start(eb) + crc_32_offset, INT_SIZE) != EEP_OK) {
				syslog(LOG_EMERG, "could not write new CRC32 value to "
						  "eeprom!\n");
				return EEP_ERR_WRITE;
			} else {
				return EEP_OK;
			}
		}
	}
	return rc;
}

// write full eeprom for TOP/main board
static Eeprom_Rc write_eeprom_bin(eb_t *eb)
{
	int rc;
	int32_t crc32;
	const int size = sizeof(eeprom_param_t) - sizeof(uint32_t); // substruct crc field
	FILE *fp = fopen(dev2Filename(eb->device), "rb+");

	if (!fp) {
		syslog(LOG_EMERG, "Cannot open EEPROM device %s: %s", dev2Filename(eb->device),
		       strerror(errno));
		return EEP_ERR_WRITE;
	}
	/*This function is called after changing some field, crc32 must be
	 * applied too.*/
	crc32 = calculate_crc32((uint8_t *)eb->data, size);
	params(eb)->crc = crc32;
	rc = fwrite(eb->data, get_eeprom_size(), 1, fp);
	if (rc > (int)get_eeprom_size()) {
		syslog(LOG_EMERG, "Unable to write to EEPROM %d %d", rc,
		       (uint32_t)get_eeprom_size());
	}
	rc = fclose(fp);
	if (rc < 0) {
		syslog(LOG_EMERG, "Cannot close(?!) EEPROM device %s: %s. Weird.",
		       dev2Filename(eb->device), strerror(errno));
		return EEP_ERR_WRITE;
	}
	return EEP_OK;
}

// write full eeprom for  daughter board
static Eeprom_Rc write_db_eeprom(uint8_t bus, uint8_t e_off, uint8_t *val, uint8_t size)
{
	int i, rc, fd;
	struct udata data;

	if (!(fd = open(I2C_DEV_NAME, O_RDWR))) {
		return EEP_ERR_WRITE;
	}

	data.addr = I2C_ADDR; /* I2C hard coded address*/
	data.bus = bus;       /* select bus - daughter card eeprom */
	data.reg = e_off;     /* write address - eeprom offset*/

	for (i = 0; i < size; i++) {
		data.val = (uint8_t)*val;
		// printf("%02hhx", data.val);
		rc = ioctl(fd, I2C_WRITE, &data);
		data.reg++;
		val++;
	}

	if (rc == -1)
		return EEP_ERR_WRITE;
	else
		return EEP_OK;
}

// get all parameters from eeprom on a  specific board
Eeprom_Rc get_eeprom_all(Eeprom_BrdNum board, eeprom_param_t *params)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc;
	eb->device = board;
	if ((rc = get_eeprom_params(board, eb)) == EEP_OK) {
		memcpy(params, params(eb), MAX_EEPROM_SIZE);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}

// get version parameter from eeprom on a specific board
Eeprom_Rc get_eep_version(Eeprom_BrdNum board, uint32_t *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	eb->device = board;
	Eeprom_Rc rc;
	if ((rc = get_eeprom_params_cached(board, eb, version_offset, sizeof(uint32_t))) ==
	    EEP_OK) {
		*value = version(eb);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}

// get platform parameter from eeprom on a specific board
Eeprom_Rc get_eep_platform(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	eb->device = board;
	Eeprom_Rc rc;
	if ((rc = get_eeprom_params_cached(board, eb, platform_offset, STR_SIZE)) == EEP_OK) {
		strncpy(value, platform(eb), STR_SIZE);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}

// get catalog parameter from eeprom on a specific board
Eeprom_Rc get_eep_catalog(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	eb->device = board;
	Eeprom_Rc rc;
	if ((rc = get_eeprom_params_cached(board, eb, catalog_offset, STR_DSIZE)) == EEP_OK) {
		strncpy(value, catalog(eb), STR_DSIZE);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}

// get serial parameter from eeprom on a specific board
Eeprom_Rc get_eep_serial(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	eb->device = board;
	Eeprom_Rc rc;
	if ((rc = get_eeprom_params_cached(board, eb, serial_offset, STR_SIZE)) == EEP_OK) {
		strncpy(value, serial(eb), STR_SIZE);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}

// get assy hw version parameter from eeprom on a specific board
Eeprom_Rc get_eep_assy(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	eb->device = board;
	Eeprom_Rc rc;
	if ((rc = get_eeprom_params_cached(board, eb, assy_offset, STR_SIZE)) == EEP_OK) {
		strncpy(value, assy(eb), STR_SIZE);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}

// get mac from eeprom on a specific board
Eeprom_Rc get_eep_mac(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	eb->device = board;
	Eeprom_Rc rc;
	if ((rc = get_eeprom_params_cached(board, eb, mac_offset, STRMAC_SIZE)) == EEP_OK) {
		strncpy(value, mac(eb), STRMAC_SIZE);
		// printf("get_eep_mac %s\n", value);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}
// get number of mac's from eeprom on a specific board
Eeprom_Rc get_eep_num_macs(Eeprom_BrdNum board, uint32_t *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	eb->device = board;
	Eeprom_Rc rc;
	if ((rc = get_eeprom_params_cached(board, eb, mac_n_offset, sizeof(uint32_t))) == EEP_OK) {
		*value = mac_n(eb);
		return EEP_OK;
	} else {
		return rc;
	}
	return EEP_OK;
}

// get crc value from eeprom on a specific board
Eeprom_Rc get_eep_crc(Eeprom_BrdNum board, uint32_t *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc;

	eb->device = board;
	if ((rc = get_eeprom_params(board, eb)) == EEP_OK) {
		*value = crc_32(eb);
		return EEP_OK;
	} else
		return rc;
}

uint32_t crc_eeprom_calc(eb_t *eb)
{
	const int size = sizeof(eeprom_param_t) - sizeof(uint32_t); // substruct crc field
	return calculate_crc32((uint8_t *)eb->data, size);
}

// set new version for a specific board
Eeprom_Rc set_eep_version(Eeprom_BrdNum board, uint32_t *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc = EEP_OK;

	if ((rc = get_eeprom_params(board, eb)) != EEP_OK) {
		return rc;
	}

	if (version(eb) != *value) {
		params(eb)->eeprom_ver_num = htonl(*value);
		if (rc = set_eeprom_params_cached(board, eb, version_offset, INT_SIZE) != EEP_OK) {
			return rc;
		}
		return write_eeprom(eb, version_offset, INT_SIZE);
	}
	return rc;
}

// set new platform for a specific board
Eeprom_Rc set_eep_platform(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc = EEP_OK;

	if ((rc = get_eeprom_params(board, eb)) != EEP_OK) {
		return rc;
	}
	if (strncmp(platform(eb), value, STR_SIZE) != 0) {
		if (strlen(value) > STR_SIZE) {
			syslog(LOG_EMERG, "string has been snipped to fit "
					  "platform limits");
			//		      rc = EEP_ERR_INV_PARAM;
		} else if (value == NULL) {
			syslog(LOG_EMERG, "value provided is NULL and invalid");
			return EEP_ERR_INV_PARAM;
		}
		strncpy(platform(eb), value, STR_SIZE);
		if (rc = set_eeprom_params_cached(board, eb, platform_offset, STR_SIZE) != EEP_OK) {
			return rc;
		}
		return write_eeprom(eb, platform_offset, STR_SIZE);
	} else
		syslog(LOG_INFO, "platform current value is equal, write "
				 "was not performed");
	return rc;
}

// set new catalog for a specific board
Eeprom_Rc set_eep_catalog(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc = EEP_OK;

	if ((rc = get_eeprom_params(board, eb)) != EEP_OK) {
		return rc;
	}
	if (strncmp(catalog(eb), value, STR_DSIZE) != 0) {
		if (strlen(value) > STR_DSIZE) {
			syslog(LOG_EMERG, "string has been snipped to fit catalog limits");
			//		      rc = EEP_ERR_INV_PARAM;
		} else if (value == NULL) {
			syslog(LOG_EMERG, "value provided is NULL and invalid");
			return EEP_ERR_INV_PARAM;
		}
		strncpy(catalog(eb), value, STR_DSIZE);
		if (rc = set_eeprom_params_cached(board, eb, catalog_offset, STR_DSIZE) != EEP_OK) {
			return rc;
		}
		return write_eeprom(eb, catalog_offset, STR_DSIZE);
	} else
		syslog(LOG_INFO, "catalog current value is equal, write was not performed");
	return rc;
}

// set new serial for a specific board
Eeprom_Rc set_eep_serial(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc = EEP_OK;

	if ((rc = get_eeprom_params(board, eb)) != EEP_OK) {
		return rc;
	}
	if (strncmp(serial(eb), value, STR_SIZE) != 0) {
		if (strlen(value) > STR_SIZE) {
			syslog(LOG_EMERG, "string has been snipped to fit serial limits");
			//		      rc = EEP_ERR_INV_PARAM;
		} else if (value == NULL) {
			syslog(LOG_EMERG, "value provided is NULL and invalid");
			return EEP_ERR_INV_PARAM;
		}
		strncpy(serial(eb), value, STR_SIZE);
		if (rc = set_eeprom_params_cached(board, eb, serial_offset, STR_SIZE) != EEP_OK) {
			return rc;
		}
		return write_eeprom(eb, serial_offset, STR_SIZE);
	} else
		syslog(LOG_INFO, "serial current value is equal, write was "
				 "not performed");
	return rc;
}

// set new assy_hw_ver for a specific board
Eeprom_Rc set_eep_assy(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc = EEP_OK;

	if ((rc = get_eeprom_params(board, eb)) != EEP_OK) {
		return rc;
	}
	if (strncmp(assy(eb), value, STR_SIZE) != 0) {
		if (strlen(value) > STR_SIZE) {
			syslog(LOG_EMERG, "string has been snipped to fit assy limits");
			//		      rc = EEP_ERR_INV_PARAM;
		} else if (value == NULL) {
			syslog(LOG_EMERG, "value provided is NULL and invalid");
			return EEP_ERR_INV_PARAM;
		}
		strncpy(assy(eb), value, STR_SIZE);
		if (rc = set_eeprom_params_cached(board, eb, assy_offset, STR_SIZE) != EEP_OK) {
			return rc;
		}
		return write_eeprom(eb, assy_offset, STR_SIZE);
	} else
		syslog(LOG_INFO, "assy current value is equal, write was not performed");
	return rc;
}

// set new mac for a specific board
Eeprom_Rc set_eep_mac(Eeprom_BrdNum board, char *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc = EEP_OK;

	if ((rc = get_eeprom_params(board, eb)) != EEP_OK) {
		return rc;
	}
	if (strncmp(mac(eb), value, STR_SIZE)) {
		if (str2mac(value, mac(eb)) != -1) {
			if (rc = set_eeprom_params_cached(board, eb, mac_offset, STRMAC_SIZE) !=
				 EEP_OK) {
				return rc;
			}
			return write_eeprom(eb, mac_offset, STRMAC_SIZE);
		} else {
			return EEP_ERR_INV_PARAM;
		}
	}
	return rc;
}

// set new number of macs value for a specific board
Eeprom_Rc set_eep_num_macs(Eeprom_BrdNum board, uint32_t *value)
{
	eb_t eb_s;
	eb_t *eb = &eb_s;
	Eeprom_Rc rc = EEP_OK;

	if ((rc = get_eeprom_params(board, eb)) != EEP_OK) {
		return rc;
	}
	if (mac_n(eb) != *value) {
		params(eb)->num_macs = htonl(*value);
		if (rc = set_eeprom_params_cached(board, eb, mac_n_offset, INT_SIZE) != EEP_OK) {
			return rc;
		}
		return write_eeprom(eb, mac_n_offset, INT_SIZE);
	}
	return rc;
}
