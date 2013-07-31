/*
 * This file is part of 'pdp', a PDP-11 simulator.
 *
 * For information contact:
 *
 *   Computer Science House
 *   Attn: Eric Edwards
 *   Box 861
 *   25 Andrews Memorial Drive
 *   Rochester, NY 14623
 *
 * Email:  mag@potter.csh.rit.edu
 * FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
 * 
 * Copyright 1994, Eric A. Edwards
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Eric A. Edwards makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * weird.c - Weird instructions.
 */


#include "defines.h"


int busreset( p )
register pdp_regs *p;
{
	fputc('*', stderr);
	fflush(stderr);
	q_reset();
	p->psw = 0200; 
	return OK;
}
	
int waiti( p )
register pdp_regs *p;
{
	extern unsigned long pending_interrupts;
	if (pending_interrupts)
		return OK;
	p->regs[PC] -= 2;	/* repeat instruction */
	return CPU_WAIT;
}

int halt(pdp_regs *p) {
	io_stop_happened = 4;
	return CPU_HALT;
}
int iot() { return CPU_IOT; }
int emt() { return CPU_EMT; }
int trap() { return CPU_TRAP; }
int bpt() { return CPU_BPT; }
int fis() { return CPU_ILLEGAL; }		/* fis() would be fun! */
int illegal() { return CPU_ILLEGAL; }
/*
 * mark() - Restore stack and jump.
 */
int mark( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	last_branch = p->regs[PC];
	p->regs[SP] = p->regs[PC] + (p->ir & 077) * 2;
	p->regs[PC] = p->regs[R5];
       if (( result = pop( p, &data )) != OK )
                return result;

        p->regs[R5] = data;

        return OK;

	
}
int bne( p ) register pdp_regs *p; { return brx( p, CC_Z, 0 ); }
int beq( p ) register pdp_regs *p; { return brx( p, 0, CC_Z ); }
int bpl( p ) register pdp_regs *p; { return brx( p, CC_N, 0 ); }
int bmi( p ) register pdp_regs *p; { return brx( p, 0, CC_N ); }
int bhi( p ) register pdp_regs *p; { return brx( p, CC_C|CC_Z, 0 ); }
int bvc( p ) register pdp_regs *p; { return brx( p, CC_V, 0 ); }
int bvs( p ) register pdp_regs *p; { return brx( p, 0, CC_V ); }
int bcc( p ) register pdp_regs *p; { return brx( p, CC_C, 0 ); }
int bcs( p ) register pdp_regs *p; { return brx( p, 0, CC_C ); }
int scc( p ) register pdp_regs *p; { p->psw |= (p->ir & 017 ); return OK; }
int ccc( p ) register pdp_regs *p; { p->psw &= ~(p->ir & 017 ); return OK; }


/*
 * sob() - Subtract One and Branch Instruction.
 */

int sob( p )
register pdp_regs *p;
{
	last_branch = p->regs[PC];
	p->regs[SRC_REG] -= 1;
	if (p->regs[SRC_REG]) {
		p->regs[PC] -= ( p->ir & 077 ) * 2;
	}
	return OK;
}


/*
 * mfps() - Move from Processor Status Instruction.
 */

int mfps( p )
register pdp_regs *p;
{
	d_byte data;
	d_word word;
	int result;

	data = LOW8( p->psw );

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CLR_CC_V();

	if ( DST_MODE ) {
		if (( result = storeb_dst( p, data )) != OK )
			return result;
	} else {
		if ( data & SIGN_B ) {
			word = 0177400;
		} else {
			word = 0;
		}
		word += data;
		if (( result = store_dst( p, word )) != OK )
			return result;
	}

	return OK;
}


/*
 * mtps() - Move to Processor Status Instruction.
 */

int mtps( p )
register pdp_regs *p;
{
	d_byte data;
	int result;
	static d_byte last_spl;

	if (( result = loadb_dst( p, &data )) != OK )
		return result;

	p->psw &= bkmodel ? ~0357 : ~0217;
	p->psw += (data & (bkmodel ? 0357 : 0217));
	
	if (last_spl != (p->psw >> 5)) {
		// fprintf(stderr, "SPL %o\n", p->psw >> 5);
	}
	last_spl = (p->psw >> 5);
	return OK;
}


/*
 * ash() - Arithmetic Shift Instruction.
 */

int ash( p )
register pdp_regs *p;
{
	d_word temp;
	d_word old;
	d_word sign;
	d_word shift;
	unsigned count;
	int result;

	temp = p->regs[SRC_REG];

	if (( result = load_dst( p, &shift )) != OK )
		return result;

	old = temp;

	if (( shift & 077 ) == 0 ) {	/* no shift */
		CHG_CC_N( temp );
		CHG_CC_Z( temp );
		CLR_CC_V();
		return OK;
	}

	if ( shift & 040 ) {		/* right shift */
		count = 0100 - ( shift & 077 );
		sign = temp & SIGN;
		while ( count-- ) {
			if ( temp & LSBIT ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp >>= 1;
			temp += sign;
		}
	} else {			/* left shift */
		count = shift & 037;
		while ( count-- ) {
			if ( temp & SIGN ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp <<= 1;
		}
	}
		
	CHG_CC_N( temp );
	CHG_CC_Z( temp );

	if (( old & SIGN ) == ( temp & SIGN )) {
		CLR_CC_V();
	} else {
		SET_CC_V();
	}

	p->regs[SRC_REG] = temp;

	return OK;
}


/*
 * mul() and divide() - Multiply and Divide Instrcutions.  These
 * work on signed values, and we'll do the same.  This may not
 * be portable.
 */


union s_u_word {
	d_word u_word;
	short s_word;
};

union s_u_long {
	unsigned long u_long;
	long s_long;
};


int mul( p )
register pdp_regs *p;
{
	union s_u_word data1;
	union s_u_word data2;
	union s_u_long tmp;
	int result;

	data1.u_word = p->regs[SRC_REG];
	if (( result = load_dst( p, &data2.u_word )) != OK )
		return result;

	tmp.s_long = ((long) data1.s_word) * ((long) data2.s_word );

	p->regs[SRC_REG] = (tmp.u_long >> 16);
	p->regs[(SRC_REG) | 1] = ( tmp.u_long & 0177777 );

	CLR_CC_ALL();

	if ( tmp.u_long == 0 )
		SET_CC_Z();
	else
		CLR_CC_Z();

	if ( tmp.u_long & 0x80000000 )
		SET_CC_N();
	else
		CLR_CC_N();

	return OK;
}

int divide( p )
register pdp_regs *p;
{
	union s_u_long tmp;
	union s_u_long eql;
	union s_u_word data2;
	int result;
	
	tmp.u_long = p->regs[SRC_REG];
	tmp.u_long = tmp.u_long << 16;
	tmp.u_long  += p->regs[(SRC_REG) | 1 ];

	if (( result = load_dst( p, &data2.u_word )) != OK )
		return result;

	if ( data2.u_word == 0 ) {
		SET_CC_C();
		SET_CC_V();
		return OK;
	} else {
		CLR_CC_C();
	}

	eql.s_long = tmp.s_long / data2.s_word;
	p->regs[SRC_REG] = eql.u_long & 0177777;

	if ( eql.u_long == 0 )
		SET_CC_Z();
	else
		CLR_CC_Z();

	if (( eql.s_long > 077777) || ( eql.s_long < -0100000))
		SET_CC_V();
	else
		CLR_CC_V();

	if ( eql.s_long < 0 )
		SET_CC_N();
	else
		CLR_CC_N();

	eql.s_long = tmp.s_long % data2.s_word;
	p->regs[(SRC_REG) | 1] = eql.u_long & 0177777;


	return OK;
}


/*
 * ashc() - Arithmetic Shift Combined Instruction.
 */

int ashc( p )
register pdp_regs *p;
{
	unsigned long temp;
	unsigned long old;
	unsigned long sign;
	unsigned count;
	int result;
	d_word shift;

	temp = p->regs[SRC_REG];
	temp <<= 16;
	temp += p->regs[(SRC_REG) | 1 ];

	old = temp;

	if (( result = load_dst( p, &shift )) != OK )
		return result;

	if (( shift & 077 ) == 0 ) {	/* no shift */

		CLR_CC_V();

		if ( temp & 0x80000000 ) {
			SET_CC_N();
		} else {
			CLR_CC_N();
		}
	
		if ( temp ) {
			CLR_CC_Z();
		} else {
			SET_CC_Z();
		}

		return OK;
	}
	if ( shift & 040 ) {		/* right shift */
		count = 0100 - ( shift & 077 );
		sign = temp & 0x80000000;
		while ( count-- ) {
			if ( temp & LSBIT ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp >>= 1;
			temp += sign;
		}
	} else {			/* left shift */
		count = shift & 037;
		while ( count-- ) {
			if ( temp & 0x80000000 ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp <<= 1;
		}
	}
		
	if ( temp & 0x80000000 )
		SET_CC_N();
	else
		CLR_CC_N();

	if ( temp )
		CLR_CC_Z();
	else
		SET_CC_Z();

	if (( old & 0x80000000 ) == ( temp & 0x80000000 )) {
		CLR_CC_V();
	} else {
		SET_CC_V();
	}

	p->regs[SRC_REG] = (temp >> 16);
	p->regs[(SRC_REG) | 1] = LOW16( temp );

	return OK;
}


/*
 * xor() - Exclusive Or Instruction
 */

int xor( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	int result;

	data2 = p->regs[SRC_REG];

	if ((result = load_dst( p, &data1 )) != OK )
		return result;

	data2 = data2 ^ data1;

	CHG_CC_N( data2 );
	CHG_CC_Z( data2 );
	CLR_CC_V();

	return store_dst_2( p, data2 );
}
