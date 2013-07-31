#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <libintl.h>
#define _(String) gettext (String)

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#ifdef linux
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#define DEVTAP "/dev/net/tun"
#endif /* linux */

static int fd = -1;
static unsigned char plip_buf[1500];
static unsigned char plip_buf_tx[1500];
static unsigned lasttime;

/* PLIP for BK-0010 uses the ability of the BK parallel port
 * to receive 16 bit; that is, from PC to BK a whole byte can
 * be sent at once.
 */
bkplip_init() {
#ifdef linux
  if (fd != -1) return OK;

  fd = open(DEVTAP, O_RDWR);
  if(fd == -1) {
    perror("tundev: tundev_init: open");
    exit(1);
  }

  {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN|IFF_NO_PI;
    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
      perror("tundev: tundev_init: ioctl");
      exit(1);
    }
  }

  lasttime = 0;
#endif
  return OK;
}

static int len_left = 0;
static int curbyte = 0;
static flag_t nibble = 0;
static int txlen = 0, txbyte = 0;
/*
 * When no data is present, returns 0.
 * If a packet is present, returns its length (a word) with bit 15 set,
 * then its contents (N bytes). Each read returns a new byte, no strobing yet.
 */
bkplip_read(addr, word)
c_addr addr;
d_word *word;
{
#ifdef linux
  fd_set fdset;
  struct timeval tv, now;
  int ret;

  if (len_left) {
	*word = plip_buf[curbyte];
	curbyte++;
	len_left--;
	return OK;
  }

  tv.tv_sec = 0;
  tv.tv_usec = 500000;

  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  ret = select(fd + 1, &fdset, NULL, NULL, &tv);
  if(ret == 0) {
    *word = 0;
    lasttime = 0;    
    return OK;
  } 
  ret = read(fd, plip_buf, 1500);  
  if(ret == -1) {
    perror("tun_dev: tundev_read: read");
  }
  curbyte = 0;
  len_left = ret;
  *word = 1<<15 | ret;

  printf("Got packet of length %d\n", ret);
#endif
  return OK;
}

/*
 * Expects a packet length (a word) with bit 15 set,
 * then N bytes. Each write transmits a byte, no strobing yet.
 */
bkplip_write(addr, word)
c_addr addr;
d_word word;
{
#ifdef linux
	if (word & (1<<15)) {
		if (txlen) {
			printf("Sending new packet when %d bytes left from the old one\n",
			txlen);
			return BUS_ERROR;
		}
		txlen = word & 0x7fff;
		txbyte = 0;
		if (txlen > 1500) {
			printf("Transmit length %d???\n", txlen);
			return BUS_ERROR;
		}
		return OK;
	}
	if (!txlen) return OK;
	plip_buf_tx[txbyte] = word & 0xff;
	txlen--;
	txbyte++;
	if (!txlen) {
		printf("Sending packet of length %d\n", txbyte);
		write(fd, plip_buf_tx, txbyte);
	}
#endif
	return OK;
}

bkplip_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr & 1;
	d_word word;
	bkplip_read(addr & ~1, &word);
	if (offset == 0) {
		word = (word & 0177400) | byte;
	} else {
		word = (byte << 8) | (word & 0377);
	}
	return bkplip_write(addr & ~1, word);
}

