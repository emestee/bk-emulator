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
 * single.c - Single operand instructions.
 */


#include "defines.h"


/*
 * adc() - Add Carry Instruction.
 */

adc( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	if ( p->psw & CC_C ) {		/* do if carry is set */
		if ( data == MPI )
			SET_CC_V();
		else
			CLR_CC_V();
		if ( data == NEG_1 )
			SET_CC_C();
		else
			CLR_CC_C();
		data++;			/* add the carry */
	} else {
		CLR_CC_V();
		CLR_CC_C();
	}

	CHG_CC_N( data );
	CHG_CC_Z( data );

	return store_dst_2( p, data );
}


/*
 * asl() - Arithmetic Shift Left Instruction.
 */

asl( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data & SIGN )
		SET_CC_C();
	else
		CLR_CC_C();

	data <<= 1;

	CHG_CC_N( data );
	CHG_CC_Z( data );
	CHG_CC_V_XOR_C_N();

	return store_dst_2( p, data );
}


/*
 * asr() - Arithmetic Shift Right Instruction.
 */

asr( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data & LSBIT )
		SET_CC_C();
	else
		CLR_CC_C();

	data = (data >> 1) + (data & SIGN );	/* shift and replicate */

	CHG_CC_N( data );
	CHG_CC_Z( data );

	CHG_CC_V_XOR_C_N();

	return store_dst_2( p, data );
}


/*
 * clr() - Clear Instruction.
 */

clr( p )
register pdp_regs *p;
{

	CLR_CC_ALL();
	SET_CC_Z();

	return store_dst( p, (d_word) 0 );
}


/*
 * com() - Complement Instrcution.
 */

com( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	data = ~data;

	CHG_CC_N( data );
	CHG_CC_Z( data );
	CLR_CC_V();
	SET_CC_C();

	return store_dst_2( p, data );
}


/*
 * dec() - Decrement Instrcution.
 */

dec( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data == MNI )
		SET_CC_V();
	else
		CLR_CC_V();

	--data;

	CHG_CC_N( data );
	CHG_CC_Z( data );

	return store_dst_2( p, data );
}


/*
 * inc() - Increment Instruction.
 */

inc( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data == MPI )
		SET_CC_V();
	else
		CLR_CC_V();

	++data;

	CHG_CC_N( data );
	CHG_CC_Z( data );

	return store_dst_2( p, data );
}


/*
 * neg() - Negate Instruction.
 */

neg( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	data = ( NEG_1 - data ) + 1;

	CHG_CC_N( data );
	CHG_CC_Z( data );

	if ( data == MNI )
		SET_CC_V();
	else
		CLR_CC_V();

	if ( data == 0 )
		CLR_CC_C();
	else
		SET_CC_C();

	return store_dst_2( p, data );
}


/*
 * rol() - Rotate Left Instruction.
 */

rol( p )
register pdp_regs *p;
{
	d_word data;
	d_word temp;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	temp = data & SIGN;		/* get sign bit */
	data <<= 1;			/* shift */

	if ( p->psw & CC_C )		/* roll in carry */
		data += LSBIT;

	if ( temp )			/* roll out to carry */
		SET_CC_C();
	else
		CLR_CC_C();

	CHG_CC_N( data );
	CHG_CC_Z( data );
	CHG_CC_V_XOR_C_N();

	return store_dst_2( p, data );
}


/*
 * ror() - Rotate Right Instruction.
 */

ror( p )
register pdp_regs *p;
{
	d_word data;
	d_word temp;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	temp = data & LSBIT;		/* get low bit */
	data >>= 1;			/* shift */

	if ( p->psw & CC_C )		/* roll in carry */
		data += SIGN;

	if ( temp )			/* roll out to carry */
		SET_CC_C();
	else
		CLR_CC_C();

	CHG_CC_N( data );
	CHG_CC_Z( data );
	CHG_CC_V_XOR_C_N();

	return store_dst_2( p, data );
}


/*
 * sbc() - Subtract Carry Instruction.
 */

sbc( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data == MNI )
		SET_CC_V();
	else
		CLR_CC_V();

	if ( p->psw & CC_C ) {		/* do if carry is set */
		if ( data )
			CLR_CC_C();
		else
			SET_CC_C();
		--data;			/* subtract carry */
	} else {
		CLR_CC_C();
	}

	CHG_CC_N( data );
	CHG_CC_Z( data );

	return store_dst_2( p, data );
}


/*
 * swabi() - Swap Bytes Instruction.
 */

swabi( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	d_word data3;
	int result;

	if (( result = load_dst( p, &data1 )) != OK ) {
		return result;
	}

	data2 = (data1 << 8 ) & 0xff00;
	data3 = (data1 >> 8 ) & 0x00ff;
	data1 = data2 + data3;

	CLR_CC_ALL();
	CHGB_CC_N( data3 );	/* cool, negative and zero */
	CHGB_CC_Z( data3 );	/* checks done on low byte only */

	return store_dst_2( p, data1 );
}


/*
 * sxt() - Sign Extend Instruction.
 */

sxt( p )
register pdp_regs *p;
{
	d_word data;

	if ( p->psw & CC_N ) {
		data = NEG_1;
		CLR_CC_Z();
	} else {
		data = 0;
		SET_CC_Z();
	}
	CLR_CC_V();

	return store_dst( p, data );
}


/*
 * tst() - Test Instruction.
 */

tst( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if (( result = load_dst( p, &data )) != OK ) {
		return result;
	}

	CLR_CC_ALL();
	CHG_CC_N( data );
	CHG_CC_Z( data );

	return OK;
}


/*
 * tstb() - Test Byte Instruction.
 */

tstb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CLR_CC_V();
	CLR_CC_C();

	return OK;
}

/*
 * aslb() - Arithmetic Shift Left Byte Instruction.
 */

aslb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data & SIGN_B )
		SET_CC_C();
	else
		CLR_CC_C();

	data <<= 1;

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CHG_CC_V_XOR_C_N();

	return storeb_dst_2( p, data );
}


/*
 * asrb() - Arithmetic Shift Right Byte Instruction.
 */

asrb( p )
pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data & LSBIT )
		SET_CC_C();
	else
		CLR_CC_C();

	data = (data >> 1) + (data & SIGN_B);	/* shift and replicate */

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CHG_CC_V_XOR_C_N();

	return storeb_dst_2( p, data );
}


/*
 * clrb() - Clear Byte Instruction.
 */

clrb( p )
register pdp_regs *p;
{
	CLR_CC_ALL();
	SET_CC_Z();

	return storeb_dst( p, (d_byte) 0 );
}


/*
 * comb() - Complement Byte Instrcution.
 */

comb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	data = ~data;

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CLR_CC_V();
	SET_CC_C();

	return storeb_dst_2( p, data );
}


/*
 * decb() - Decrement Byte Instrcution.
 */

decb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data == MNI_B )
		SET_CC_V();
	else
		CLR_CC_V();

	--data;

	CHGB_CC_N( data );
	CHGB_CC_Z( data );

	return storeb_dst_2( p, data );
}


/*
 * incb() - Increment Byte Instruction.
 */

incb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	if ( data == MPI_B )
		SET_CC_V();
	else
		CLR_CC_V();

	++data;

	CHGB_CC_N( data );
	CHGB_CC_Z( data );

	return storeb_dst_2( p, data );
}


/*
 * negb() - Negate Byte Instruction.
 */

negb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	data = (NEG_1_B - data) + 1;	/* hope this is right */

	CHGB_CC_N( data );
	CHGB_CC_Z( data );

	if ( data == MNI_B )
		SET_CC_V();
	else
		CLR_CC_V();

	if ( data )
		SET_CC_C();
	else
		CLR_CC_C();

	return storeb_dst_2( p, data );
}


/*
 * rolb() - Rotate Left Byte Instruction.
 */

rolb( p )
register pdp_regs *p;
{
	d_byte data;
	d_byte temp;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	temp = data & SIGN_B;		/* get top bit */
	data <<= 1;			/* shift */

	if ( p->psw & CC_C )		/* roll in carry */
		data = data + LSBIT;

	if ( temp )			/* roll out to carry */
		SET_CC_C();
	else
		CLR_CC_C();

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CHG_CC_V_XOR_C_N();

	return storeb_dst_2( p, data );
}


/*
 * rorb() - Rotate Right Byte Instruction.
 */

rorb( p )
register pdp_regs *p;
{
	d_byte data;
	d_byte temp;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	temp = data & LSBIT;		/* get low bit */
	data >>= 1;			/* shift */

	if ( p->psw & CC_C )		/* roll in carry */
		data += SIGN_B;

	if ( temp )			/* roll out to carry */
		SET_CC_C();
	else
		CLR_CC_C();

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CHG_CC_V_XOR_C_N();

	return storeb_dst_2( p, data );
}


/*
 * adcb() - Add Carry Byte Instruction.
 */

adcb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	if ( p->psw & CC_C ) {		/* do if carry is set */
		if ( data == MPI_B )
			SET_CC_V();
		else
			CLR_CC_V();
		if ( data == NEG_1_B )
			SET_CC_C();
		else
			CLR_CC_C();
		++data;			/* add the carry */
	} else {
		CLR_CC_V();
		CLR_CC_C();
	}

	CHGB_CC_N( data );
	CHGB_CC_Z( data );

	return storeb_dst_2( p, data );
}


/*
 * sbcb() - Subtract Carry Byte Instruction.
 */

sbcb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if (( result = loadb_dst( p, &data )) != OK ) {
		return result;
	}

	if ( p->psw & CC_C ) {		/* do if carry is set */
		if ( data )
			CLR_CC_C();
		else
			SET_CC_C();

		--data;			/* subtract carry */
	} else {
		CLR_CC_C();
	}

	if ( data == MNI_B )
		SET_CC_V();
	else
		CLR_CC_V();

	CHGB_CC_N( data );
	CHGB_CC_Z( data );

	return storeb_dst_2( p, data );
}
