/*
 * Copyright 2003, Leonid Broukhis
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Leonid Broukhis makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * disas.c - Instruction disassembly.
 */

#include "defines.h"
#include <ctype.h>
#include <libintl.h>
#define _(String) gettext (String)

/* Instructions can be prefixed by an uppercase letter:
 * B - branch (signed jump offset)
 * F - floating point (one register)
 * G - two general operands
 * N - print 1-word opcode
 * O - one general operand
 * R - one register operand, one general operand
 * S - sob-style (negative) jump offset
 * T - trap (8-bit immediate operand),
 * U - unsigned 6-bit immediate operand (for mark),
 * otherwise it has no operands.
 * Digits mean "look deeper".
 */
char* base[16] = {
"0", "Gmov", "Gcmp", "Gbit", "Gbic", "Gbis", "Gadd", "7",
"8", "Gmovb", "Gcmpb", "Gbitb", "Gbicb", "Gbisb", "Gsub", "N"
};

char* decode0[64] = {
/*000*/ "0", "Ojmp", "2", "Oswab", "Bbr", "Bbr", "Bbr", "Bbr",
/*001*/ "Bbne", "Bbne", "Bbne", "Bbne", "Bbeq", "Bbeq", "Bbeq", "Bbeq",
/*002*/ "Bbge", "Bbge", "Bbge", "Bbge", "Bblt", "Bblt", "Bblt", "Bblt",
/*003*/ "Bbgt", "Bbgt", "Bbgt", "Bbgt", "Bble", "Bble", "Bble", "Bble",
/*004*/ "Rjsr", "Rjsr", "Rjsr", "Rjsr", "Rjsr", "Rjsr", "Rjsr", "Rjsr",
/*005*/ "Oclr", "Ocom", "Oinc", "Odec", "Oneg", "Oadc", "Osbc", "Otst",
/*006*/ "Oror", "Orol", "Oasr", "Oasl", "Umark", "Omfpi", "Omtpi", "Osxt",
/*007*/ "N", "N", "N", "N", "N", "N", "N", "N"
};

char* decode000[64] = {
	"halt", "wait", "rti", "bpt", "iot", "reset", "rtt", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N"
};

/* Here the instruction is completely decoded, we can print it as is */
char * decode002[64] = {
	"rts r0", "rts r1", "rts r2", "rts r3", "rts r4", "rts r5", "rts sp", "rts pc",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"N", "N", "N", "N", "N", "N", "N", "N",
	"nop", "clc", "clv", "clvc", "clz", "clzc", "clzv", "clzvc",
	"cln", "clnc", "clnv", "clnvc", "clnz", "clnzc", "clnzv", "ccc",
	"nops", "sec", "sev", "sevc", "sez", "sezc", "sezv", "sezvc",
	"sen", "senc", "senv", "senvc", "senz", "senzc", "senzv", "scc"
};

char * decode7[64] = {
/*070*/ "Rmul",
/*071*/ "Rdiv",
/*072*/ "Rash",
/*073*/ "Rashc",
/*074*/ "Rxor",
/*075*/ "5", 
/*076*/ "N", 
/*077*/ "Ssob",
};

char * decode750[4] = {
/*07500*/ "Ffadd",
/*07501*/ "Ffsub",
/*07502*/ "Ffmul",
/*07503*/ "Ffdiv",
};

char * regnam[8] = {"r0", "r1", "r2", "r3", "r4", "r5", "sp", "pc" };

char * decode8[64] = {
/*100*/ "Bbpl", "Bbpl", "Bbpl", "Bbpl", "Bbmi", "Bbmi", "Bbmi", "Bbmi",
/*101*/ "Bbhi", "Bbhi", "Bbhi", "Bbhi", "Bblos", "Bblos", "Bblos", "Bblos",
/*102*/ "Bbvc", "Bbvc", "Bbvc", "Bbvc", "Bbvs", "Bbvs", "Bbvs", "Bbvs",
/*103*/ "Bbcc", "Bbcc", "Bbcc", "Bbcc", "Bbcs", "Bbcs", "Bbcs", "Bbcs",
/*104*/ "Temt", "Temt", "Temt", "Temt", "Ttrap", "Ttrap", "Ttrap", "Ttrap",
/*105*/ "Oclrb", "Ocomb", "Oincb", "Odecb", "Onegb", "Oadcb", "Osbcb", "Otstb",
/*106*/ "Ororb", "Orolb", "Oasrb", "Oaslb", "Omtps", "Omfpd", "Omtpd", "Omfps",
/*107*/ "N", "N", "N", "N", "N", "N", "N", "N",

};

/* Will update pc as necessary.
 * Writes to the pointed string and corrects
 * it to point past end.
 */
void printop(unsigned rm, c_addr * a, char **destp) {
	d_word word;
	unsigned r = rm & 7;
	unsigned mode = rm >> 3;
	switch (mode) {
	case 0:
		sprintf(*destp, regnam[r]);
		break;
	case 1:
		sprintf(*destp, "(%s)", regnam[r]);
		break;
	case 3:
		*(*destp)++ = '@';
		/* fall thru */
	case 2:
		if (r == 7) {
			lc_word(*a, &word);
			*a += 2;
			*a &= 0177777;
			sprintf(*destp, "#%o", word);
		} else {
			sprintf(*destp, "(%s)+", regnam[r]);
		}
		break;
	case 5:
		*(*destp)++ = '@';
		/* fall thru */
	case 4:
		sprintf(*destp, "-(%s)", regnam[r]);
		break;
	case 7:
		*(*destp)++ = '@';
		/* fall thru */
	case 6:
		lc_word(*a, &word);
		*a += 2;
		*a &= 0177777;
		if (r == 7) {
			sprintf(*destp, "%o", (*a + word) & 0177777);
		} else {
			sprintf(*destp, "%o(%s)", word, regnam[r]);
		}
	}
	*destp += strlen(*destp);
}

/* Returns the PC of the next instruction */
c_addr disas (c_addr a, char * dest) {
	d_word inst = 0;
	lc_word(a, &inst);
	a += 2;
	a &= 0177777;
	char * code = base[inst >> 12];
	char soffset;
	unsigned char uoffset;
	switch (*code) {
		case '0': code = decode0[(inst >> 6) & 077]; break;
		case '7': code = decode7[(inst >> 9) & 7]; break;
		case '8': code = decode8[(inst >> 6) & 077]; break;
	}
	switch (*code) {
		case '0': code = decode000[inst & 077]; break;
		case '2': code = decode002[inst & 077]; break;
		case '5':
			if (inst & 0000740)
				code = "N";
			else
				code = decode750[(inst >> 3) & 3];
			break; 
	}
	sprintf(dest, "%06o: %s ", a - 2, code + !!isupper(*code));

	if (isupper(*code)) {
		dest += strlen(dest);
		switch (*code) {
		case 'B':
			soffset = inst;
			sprintf(dest, "%06o", a + 2 * soffset);
			break;
		case 'F':
			sprintf(dest, "(%s)", regnam[inst & 7]);
			break;
		case 'S':
			sprintf(dest, "%s, ", regnam[(inst >> 6) & 7]);
			uoffset = inst & 077;
			dest += strlen(dest);
			sprintf(dest, "%06o", a - 2 * uoffset);
			break;
		case 'G':
			printop((inst >> 6) & 077, &a, &dest);
			sprintf(dest, ", ");
			dest += 2;
			printop(inst & 077, &a, &dest);
			break;
		case 'R':
			sprintf(dest, "%s, ", regnam[(inst >> 6) & 7]);
			dest += strlen(dest);
			/* fall thru */
		case 'O':
			printop(inst & 077, &a, &dest);
			break;
		case 'T':
			sprintf(dest, "%o", inst & 0377);
			break;
		case 'U':
			sprintf(dest, "%o", inst & 077);
			break;
		case 'N':
			sprintf(dest, "%06o", inst);
			break;
		default: sprintf(dest, _("ERROR code %c"), *code);
	}}
	return a;
}

char * state(pdp_regs * p) {
	static char buf[80];
	sprintf(buf, "%04x %04x %04x %04x %04x %04x %04x %04x",
		p->regs[0], p->regs[1], p->regs[2], p->regs[3],
		p->regs[4], p->regs[5], p->regs[6], p->psw);
	return buf;
}
