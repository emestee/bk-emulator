#include <stdio.h>
#define SYNCH 1000
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)

#define putSI   beep(1, SYNCH), beep(0, SYNCH)
#define put0    beep(1, SYNCH), beep(0, SYNCH), putSI
#define put1    beep(1, 2 * SYNCH), beep(0, 2 * SYNCH), putSI
#define putINM  beep(1, 8 * SYNCH)
#define putOUTM beep(1, 4 * SYNCH), beep(0, 4 * SYNCH)

beep(what, len) {
	putchar((what << 7) | (len >> 8));
	putchar(len & 0xFF);
	return 0;
}

putbyte(byte) {
	int i;
	for (i = 0; i < 8; i++) {
		if (byte & 1) put1; else put0;
		byte >>= 1;
	}
}

putarray(p, l)
unsigned char * p; int l;
{
	int i;
	for(i = 0; i < l; i++)
		putbyte(p[i]);
}

puttune(l) {
	while(l--) putSI;
}

#define FAKE_ADDRESS 077777
#define FAKE_LENGTH 1
#define FAKE_CHECKSUM 0177777

void putaddr(FILE * in) {
	putbyte(in ? getc(in) : FAKE_ADDRESS & 0xff);
	putbyte(in ? getc(in) : FAKE_ADDRESS >> 8);
}

int putlen(FILE * in) {
	int len = in ? getc(in) : FAKE_LENGTH & 0xff;
	len |= (in ? getc(in) : FAKE_LENGTH >> 8) << 8;
	putbyte(len & 0xFF);
	putbyte(len >> 8);
	return len;
}

main(argc, argv) int argc; char ** argv; {

        /* Gettext staff */
        setlocale (LC_ALL, "");
	bindtextdomain ("bk", "/usr/share/locale");
        textdomain ("bk");
		
	int i, sum = 0, len, namelen;
	FILE * in;
	if (argc != 3) {
		fprintf(stderr, _("Usage: maketape BK_NAME infile > outfile\n"));
		exit(1);
	}
	in = fopen(argv[2], "r");
	/* If the file cannot be opened, make up a file with
	 * start address 0, length 0, and a bad tune-up after the header.
	 */
	putINM;
	puttune(010000);
	putOUTM;
	put1;
	puttune(010);
	putOUTM;
	put1;
	/* start address */
	putaddr(in);
	/* length */
	len = putlen(in);
	/* name */
	namelen = strlen(argv[1]);
	putarray(argv[1], namelen > 16 ? 16 : namelen);
	if (namelen < 16) putarray("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16 - namelen);
	puttune(010);
	putOUTM;
	put1;
	/* data must be here */
	while(len--) {
		i = in ? getc(in) : 0;
		sum += i;
		if (sum & 0200000) sum = (sum & 0xFFFF) + 1;
		putbyte(i);
	}

	if (!in) sum = FAKE_CHECKSUM;

	/* checksum */
	fprintf(stderr, _("checksum = %06o\n"), sum);
	putbyte(sum & 0xFF);
	putbyte(sum >> 8);
	/* exit sequence */
	putINM;
	puttune(0400);
	putOUTM;
	put1;
}
