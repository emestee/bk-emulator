#include "defines.h"
#include <libintl.h>
#define _(String) gettext (String)

double ticks = 0;    /* in clock ticks, integral */

#define REGREG 12

static a_time[8]  = {0, 12, 12, 20, 12, 20, 20, 28};
static b_time[8]  = {0, 20, 20, 32, 20, 32, 32, 40};
static ab_time[8] = {0, 16, 16, 24, 16, 24, 24, 32};
static a2_time[8] = {0, 20, 20, 28, 20, 28, 28, 36};
static ds_time[8] = {0, 32, 32, 40, 32, 40, 40, 48};

#define a1_time a_time
#define dj_time a2_time

#define dst_time (SRC_MODE ? ab_time : b_time)
#define cmp_time (SRC_MODE ? a1_time : a2_time)

enum inst {
 illegal, adc, adcb, add, ash, ashc, asl, aslb, asr, asrb, bcc, bcs,
 beq, bge, bgt, bhi, bic, bicb, bis, bisb, bit, bitb, ble, blos, blt,
 bmi, bne, bpl, bpt, br, bvc, bvs, clr, clrb, ccc, cmp, cmpb, com,
 comb, dec, decb, divide, emt, halt, inc, incb, iot, jmp, jsr, mark,
 mfpd, mfpi, mfps, mov, movb, mtpd, mtpi, mtps, mul, neg, negb,
 busreset, rol, rolb, ror, rorb, rti, rts, rtt, sbc, sbcb, scc, sob,
 sub, swabi, sxt, trap, tst, tstb, waiti, xor, fis, itimtab0, itimtab1
};

static enum inst sitimtab0[64] = {
	halt, waiti, rti, bpt, iot, busreset, rtt, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal
};

static enum inst sitimtab1[64] = {
	rts, rts, rts, rts, rts, rts, rts, rts,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	ccc, ccc, ccc, ccc, ccc, ccc, ccc, ccc,
	ccc, ccc, ccc, ccc, ccc, ccc, ccc, ccc,
	scc, scc, scc, scc, scc, scc, scc, scc,
	scc, scc, scc, scc, scc, scc, scc, scc
};

static enum inst itimtab[1024] = {
	itimtab0, jmp, itimtab1, swabi, br, br, br, br,
	bne, bne, bne, bne, beq, beq, beq, beq,
	bge, bge, bge, bge, blt, blt, blt, blt,
	bgt, bgt, bgt, bgt, ble, ble, ble, ble,
	jsr, jsr, jsr, jsr, jsr, jsr, jsr, jsr,
	clr, com, inc, dec, neg, adc, sbc, tst,
	ror, rol, asr, asl, mark, illegal, illegal, sxt,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	mul, mul, mul, mul, mul, mul, mul, mul,
	divide, divide, divide, divide, divide, divide, divide, divide,
	ash, ash, ash, ash, ash, ash, ash, ash,
	ashc, ashc, ashc, ashc, ashc, ashc, ashc, ashc,
	xor, xor, xor, xor, xor, xor, xor, xor,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	sob, sob, sob, sob, sob, sob, sob, sob,
	bpl, bpl, bpl, bpl, bmi, bmi, bmi, bmi,
	bhi, bhi, bhi, bhi, blos, blos, blos, blos,
	bvc, bvc, bvc, bvc, bvs, bvs, bvs, bvs,
	bcc, bcc, bcc, bcc, bcs, bcs, bcs, bcs,
	emt, emt, emt, emt, trap, trap, trap, trap,
	clrb, comb, incb, decb, negb, adcb, sbcb, tstb,
	rorb, rolb, asrb, aslb, mtps, illegal, illegal, mfps,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis
};

timing(p)
register pdp_regs *p;
{
    int byteop = (p->ir & SIGN) && ((p->ir >> 12) != 016);
    enum {defop, binop, unop, cmpop, tstop } optype = defop;
    enum inst code;
    code = itimtab[p->ir >> 6];
    switch (code) {
    case itimtab0: code = sitimtab0[p->ir & 077]; break;
    case itimtab1: code = sitimtab1[p->ir & 077]; break;
    }
    switch ( code ) {
    case  mov: case  movb:
    case  bic: case  bicb: case  bis: case  bisb:
    case  add: case  sub:
	ticks += REGREG + a_time[SRC_MODE] + dst_time[DST_MODE];
	break;
    case  cmp: case  cmpb: case  bit: case  bitb:
	ticks += REGREG + a1_time[SRC_MODE] + cmp_time[DST_MODE];
	break;
    case  clr: case  clrb:
    case  com: case  comb:
    case  neg: case  negb:
    case  inc: case  incb:
    case  dec: case  decb:
    case  adc: case  adcb:
    case  sbc: case  sbcb:
    case  ror: case  rorb:
    case  rol: case  rolb:
    case  asl: case  aslb:
    case  asr: case  asrb:
    case  mtps: case  mfps:
    case  swabi: case  sxt:
	ticks += REGREG + ab_time[DST_MODE];
	break;
    case  tst: case  tstb:
	ticks += REGREG + a1_time[DST_MODE];
	break;
    case  bne: case  beq:
    case  bge: case  blt:
    case  bgt: case  ble:
    case  bpl: case  bmi:
    case  bhi: case  blos:
    case  bvc: case  bvs:
    case  bcc: case  bcs:
    case  br:
	ticks += 16;
	break;
    case  jmp:
	ticks += dj_time[DST_MODE];
	break;
    case  jsr:
	ticks += ds_time[DST_MODE];
	break;
    case  sob:
	ticks += 20;
	break;
    case  rts:
	ticks += 32;
	break;
    case  scc: case  ccc:
	ticks += REGREG;
	break;
    case  rti: case rtt:
	ticks += 40;
	break;
    case  trap:
    case  emt:
    case  bpt:
    case  iot:
	ticks += 68;
	break;
    case  xor:
	ticks += REGREG + a2_time[DST_MODE];
	break;
    case mark:
	ticks += 36;
	break;
    case  illegal: case  halt: case fis: case mul: case divide:
    case ash: case ashc:
	ticks += 144;
	break;
    case waiti:
    case busreset:
	ticks += 1167;	/* 350 us */
	break;
    default:
	fprintf(stderr, _("No timing for instr %06o\n"), p->ir);
    }
}
