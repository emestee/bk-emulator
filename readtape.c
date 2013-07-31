#include <stdio.h>
#include <malloc.h>
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)

#define SENSE 150
#define OK 0
#define FAIL 1
#define max(a, b) ((a) > (b) ? (a) : (b))

get_strobe(len) int *len;
{
    static int old_strobe = 0;
    int c;
again:
    if (feof(stdin)) {
	fprintf(stderr, _("Tape ended\n"));
        exit(1);
    }
    c = getchar() & 0xFF;
    c = (c << 8) | (getchar() & 0xFF);
    if ((c & 0x7fff) > 1) {
	int toret = old_strobe;
	old_strobe = c;
	*len = toret & 0x7fff;
	return toret >> 15;
    } else {
	old_strobe++;
	c = getchar() & 0xFF;
	c = (c << 8) | (getchar() & 0xFF);
	old_strobe += c & 0x7fff;
	goto again;
    }
}

static polarity = 1;
static threshold;

main() {
	/* Gettext staff */
        setlocale (LC_ALL, "");
	bindtextdomain ("bk", "/usr/share/locale");
	textdomain ("bk");

	for (;;) do { do {
	    find_header();
	} while (OK != read_header());
	} while (OK != read_data());
}

find_header() {
	int strobe_cnt;
	int strobe_len;
	int len, val;
	int num;
restart:
	strobe_cnt = 0;
	strobe_len = 0;
	do {
	    while (0 == get_strobe(&len));
	    if (strobe_len - len > SENSE) {
		strobe_cnt = 0;
		strobe_len = 0;
	    } else {
		strobe_len = len;
		strobe_cnt++;
	    }
	} while (strobe_cnt <= 04000);
	fprintf(stderr, _("Found start tune-up sequence\n"));
	find_threshold();
	num = 0;
	do {
	    val = get_strobe(&len);
	    num++;
	} while (len < threshold);
	if (len > 2 * threshold || num < 100) {
	    fprintf(stderr, "False file marker\n");
	    goto restart;
	}
	fprintf(stderr, _("Skipped %d strobes to find marker\n"), num);
	polarity = val;
	fprintf(stderr, _("Polarity set to %d\n"), polarity);
	read_bit(); /* skip one bit */
}

find_threshold() {
    int sum = 0;
    int i;
    for (i = 0; i < 0200; i++)
	sum += read_bit();
    sum >>= 7;
    threshold = sum + (sum >> 1);
    fprintf(stderr, _("Threshold set to %d\n"), threshold);
}

read_bit() {
    int len1, len2, len3, len4;
    while(polarity != get_strobe(&len1));
    get_strobe(&len2);
    get_strobe(&len3);
    get_strobe(&len4);
    return max(len1, len3) + max(len2, len4);
}

static file_length, start_addr;
unsigned char header[025];

read_header()
{
    header[024] = '\0';
    if (FAIL == read_array(header, 024)) return FAIL;
    fprintf(stderr, _("Start address %06o, length %06o\n"),
	start_addr = header[1] << 8 | header[0],
	file_length = header[3] << 8 | header[2]
    );
    fprintf(stderr, _("File name: <% .16s>\n"), header + 4);
    return OK;
}

read_data()
{
	int checksum = 0;
	int i, sum = 0;
	unsigned char * array = calloc(file_length, 1);
	if (FAIL == read_array(array, file_length)) {
		fprintf(stderr, _("Failed\n"));
		return FAIL;
	}
	for(i = 0; i < 16; i++) {
	    if (read_bit() > threshold) checksum |= 1 << i;
	}

	for (i = 0; i < file_length; i++) {
		sum += array[i];
		if (sum & 0200000) sum = (sum & 0xFFFF) + 1;
	}
	if (sum != checksum) {
		fprintf(stderr, _("Checksum error: read %06o, computed %06o\n"),
			checksum, sum);
		return FAIL;
	} else {
		fprintf(stderr, _("Checksum = %06o\n"), checksum);
	}
	/* Cut extra spaces from the end of file name */
	for (i = 19; i > 4; i--)
        	if (header[i] == ' ') header[i] = '\0';
		else break;
	FILE * out = fopen(header+4, "w");
	if (!out) {
        	perror(header+4);
        	return FAIL;
	}
	fputc(start_addr & 0xFF, out);
	fputc(start_addr >> 8, out);
	fputc(file_length & 0xFF, out);
	fputc(file_length >> 8, out);
	fwrite(array, file_length, 1, out);
	fclose(out);
}

read_array(addr, length)
unsigned char * addr;
{
    int len;
    unsigned char * p = addr;
    int i;
    do {
	while(get_strobe(&len) != polarity) fputc('.', stderr);
    } while (len < threshold);
    if (len > threshold * 2) {
	fprintf(stderr, _("False header marker\n"));
	return FAIL;
    }
    read_bit();
    for (; length--;) {
	unsigned char c = 0;
	for(i = 0; i < 8; i++) {
	    int l = read_bit();
	    if (l < threshold / 2) {
#if 0
		fprintf(stderr, _("Too short (%d) assuming 0\n"), l);
		fprintf(stderr, _("Tape position %0o\n"), ftell(stdin));
#endif
	    } else if (l > threshold * 2) {
#if 0
		fprintf(stderr, _("Too long (%d), assuming 1\n"), l);
		fprintf(stderr, _("Tape position %0o\n"), ftell(stdin));
#endif
	    }
	    if (l > threshold + 1) c |= 1 << i;
	}
	*p++ = c;
    }
    return OK;
}
