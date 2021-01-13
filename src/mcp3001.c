// SPDX-License-Identifier: OSL-3.0
/*
 * quick'n'dirty program to read, munge,
 * and display a 10-bit value from an
 * MCP3001
 *
 * Copyright (C) 2021  Trevor Woerner <twoerner@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "config.h"

static char *deviceName_pG = "/dev/spidev0.0";
static uint32_t mode_G = 0;
static uint8_t bits_G = 8;
static int verbose_G = 0;
static uint32_t speed_G = 500000;

static int parse_cmdline (int argc, char *argv[]);
static int configure_spi (int fd);
static void hex_dump (void *src_p, size_t length, size_t line_size, char *prefix_p);
static int munge (uint8_t *inBuf_p, uint16_t *rtnVal_p);


int
main (int argc, char *argv[])
{
	int ret;
	int fd;
	ssize_t retRead;
	uint8_t inBuf[16];
	uint16_t tenBitADC;

	ret = parse_cmdline(argc, argv);
	if (ret != 0)
		return 1;

	fd = open(deviceName_pG, O_RDWR);
	if (fd < 0) {
		perror("open() device");
		return 1;
	}

	ret = configure_spi(fd);
	if (ret != 0)
		goto closefd;
	if (verbose_G) {
		printf("spi mode: 0x%x\n", mode_G);
		printf("bits per word: %d\n", bits_G);
		printf("max speed: %d Hz (%d KHz)\n", speed_G, speed_G/1000);
	}

	memset(inBuf, 0, sizeof(inBuf));
	retRead = read(fd, inBuf, sizeof(inBuf));
	if (retRead == -1) {
		perror("read()");
		goto closefd;
	}
	if (verbose_G)
		hex_dump(inBuf, sizeof(inBuf), 24, "RX");

	// convert 10-bit munging
	ret = munge(inBuf, &tenBitADC);
	if (ret != 0)
		goto closefd;
	printf("%03hX (%hu)\n", tenBitADC, tenBitADC);

	ret = 0;
closefd:
	close(fd);
	return ret;
}

static int
configure_spi (int fd)
{
	int ret;

	// spi mode
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode_G);
	if (ret == -1) {
		perror("ioctl(SPI_IOC_WR_MODE32)");
		return -1;
	}
	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode_G);
	if (ret == -1) {
		perror("ioctl(SPI_IOC_RD_MODE32)");
		return -1;
	}

	// spi bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_G);
	if (ret == -1) {
		perror("ioctl(SPI_IOC_WR_BITS_PER_WORD)");
		return -1;
	}
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits_G);
	if (ret == -1) {
		perror("ioctl(SPI_IOC_RD_BITS_PER_WORD)");
		return -1;
	}

	// spi speed
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_G);
	if (ret == -1) {
		perror("ioctl(SPI_IOC_WR_MAX_SPEED_HZ)");
		return -1;
	}
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed_G);
	if (ret == -1) {
		perror("ioctl(SPI_IOC_RD_MAX_SPEED_HZ)");
		return -1;
	}

	return 0;
}

static void
hex_dump (void *src_p, size_t length, size_t line_size, char *prefix_p)
{
	size_t i = 0;
	uint8_t *address_p = src_p;
	uint8_t *line_p = address_p;
	uint8_t c;

	printf("%s | ", prefix_p);
	while (length-- > 0) {
		printf("%02X ", *address_p++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" |");
			while (line_p < address_p) {
				c = *line_p++;
				printf("%c", (c < 32 || c > 126) ? '.' : c);
			}
			printf("|\n");
			if (length > 0)
				printf("%s | ", prefix_p);
		}
	}
}

static int
munge (uint8_t *inBuf_p, uint16_t *retVal_p)
{
	uint16_t chkVal = 0;

	*retVal_p  = (inBuf_p[0] & 0x1f) << 5;
	*retVal_p |= (inBuf_p[1] & 0xf8) >> 3;

	chkVal  = (inBuf_p[1] & 0x08) >> 3;
	chkVal |= (inBuf_p[1] & 0x04) >> 1;
	chkVal |= (inBuf_p[1] & 0x02) << 1;
	chkVal |= (inBuf_p[1] & 0x01) << 3;
	chkVal |= (inBuf_p[2] & 0x80) >> 3;
	chkVal |= (inBuf_p[2] & 0x40) >> 1;
	chkVal |= (inBuf_p[2] & 0x20) << 1;
	chkVal |= (inBuf_p[2] & 0x10) << 3;
	chkVal |= (inBuf_p[2] & 0x08) << 5;
	chkVal |= (inBuf_p[2] & 0x04) << 7;

	if (*retVal_p != chkVal) {
		printf("=> verify failed: retVal(%03X) chkVal(%03X)\n", *retVal_p, chkVal);
		return -1;
	}

	return 0;
}

static void
usage (const char *cmd_p)
{
	printf("%s\n", PACKAGE_STRING);
	if (cmd_p != NULL) {
		printf("usage: %s [options]\n", cmd_p);
		printf("\n");
		printf("  where:\n");
	}
	printf("    options:\n");
	printf("      -D|--device <d> SPI device to use (default: /dev/spidev0.0)\n");
	printf("      -s|--speed  <s> max speed (Hz)\n");
	printf("      -b|--bpw <b>    bits per word\n");
	printf("      -O|--cpol       clock polarity, idle high (default: idle low)\n");
	printf("      -H|--cpha       clock phase, sample on trailing edge (default: leading)\n");
	printf("      -L|--lsb        least significant bit first\n");
	printf("      -C|--cs-high    chip select active high\n");
	printf("      -3|--3wire      SI/SO signals shared\n");
	printf("      -N|--no-cs      no chip select\n");
	printf("      -R|--ready      slave pulls low to pause\n");
	printf("      -v|--verbose    add verbosity\n");
	printf("      -V|--version    print version\n");
}

static int
parse_cmdline(int argc, char *argv[])
{
	int c;
	int ret;
	struct option opts[] = {
		{"device",  required_argument, NULL, 'D'},
		{"speed",   required_argument, NULL, 's'},
		{"bpw",     required_argument, NULL, 'b'},
		{"help",    no_argument, NULL, 'h'},
		{"cpol",    no_argument, NULL, 'O'},
		{"cpha",    no_argument, NULL, 'H'},
		{"lsb",     no_argument, NULL, 'L'},
		{"cs-high", no_argument, NULL, 'C'},
		{"3wire",   no_argument, NULL, '3'},
		{"no-cs",   no_argument, NULL, 'N'},
		{"ready",   no_argument, NULL, 'R'},
		{"verbose", no_argument, NULL, 'v'},
		{"version", no_argument, NULL, 'V'},
		{NULL, 0, NULL, 0 },
	};

	mode_G = 0;
	while (1) {
		c = getopt_long(argc, argv, "D:s:b:hHCOL3NRvV", opts, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'D':
				deviceName_pG = optarg;
				break;

			case 's':
				ret = sscanf(optarg, "%d", &speed_G);
				if (ret != 1)
					return -1;
				break;

			case 'b':
				ret = sscanf(optarg, "%hhu", &bits_G);
				if (ret != 1)
					return -1;
				break;

			case 'h':
				usage(argv[0]);
				exit(0);

			case 'O':
				mode_G |= SPI_CPOL;
				break;

			case 'H':
				mode_G |= SPI_CPHA;
				break;

			case 'L':
				mode_G |= SPI_LSB_FIRST;
				break;

			case 'C':
				mode_G |= SPI_CS_HIGH;
				break;

			case '3':
				mode_G |= SPI_3WIRE;
				break;

			case 'N':
				mode_G |= SPI_NO_CS;
				break;

			case 'R':
				mode_G |= SPI_READY;
				break;

			case 'v':
				++verbose_G;
				break;

			case 'V':
				printf("%s\n", PACKAGE_STRING);
				exit(0);

			default:
				usage(argv[0]);
				return -1;
		}
	}

	return 0;
}
