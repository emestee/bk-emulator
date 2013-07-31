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
 * double.c - Double operand instrcutions.
 */


#include "defines.h"


/*
 * mov() - Move Instruction.  Move operations with registers
 * as the source and/or destination have been inlined.
 */

int
mov( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	if ( SRC_MODE ) {
		if (( result = load_src( p, &data )) != OK )
			return result;
	} else {
		data = p->regs[SRC_REG];
	}

	CHG_CC_N( data );
	CHG_CC_Z( data );
	CLR_CC_V();

	if ( DST_MODE ) {
		if (( result = store_dst( p, data )) != OK )
			return result;
	} else {
		p->regs[DST_REG] = data;
	}

	return OK;
}


/*
 * cmp() - Compare Instruction.
 */

int
cmp( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	d_word data3;
	unsigned long temp;
	int result;

	if (( result = load_src( p, &data1 )) != OK )
		return result;
	if (( result = load_dst( p, &data2 )) != OK )
		return result;

	data3 = ~data2;
	temp = ((unsigned long) data1) + ((unsigned long) ( data3 )) + 1;
	data3 = LOW16( temp );

	CHG_CC_N( data3 );
	CHG_CC_Z( data3 );
	CHG_CC_VC( data1, data2, data3 );	/* was CHG_CC_V */
	CHG_CC_IC( temp );

	return OK;
}


/*
 * add() - Add Instruction.
 */

int
add( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	d_word data3;
	unsigned long temp;
	int result;

	if (( result = load_src( p, &data1 )) != OK )
		return result;
	if (( result = load_dst( p, &data2 )) != OK )
		return result;

	temp = ((unsigned long) data1) + ((unsigned long) data2);
	data3 = LOW16( temp );

	CHG_CC_N( data3 );
	CHG_CC_Z( data3 );
	CHG_CC_V( data1, data2, data3 );
	CHG_CC_C( temp );

	if (( result = store_dst_2( p, data3 )) != OK )
		return result;

	return OK;
}


/*
 * Subtract Instruction.
 */

int
sub( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	d_word data3;
	unsigned long temp;
	int result;

	if (( result = load_src( p, &data1 )) != OK )
		return result;
	if (( result = load_dst( p, &data2 )) != OK )
		return result;

	data3 = ~data1;
	temp = ((unsigned long) data2) + ((unsigned long) data3 ) + 1;
	data3 = LOW16( temp );

	CHG_CC_N( data3 );
	CHG_CC_Z( data3 );
	CHG_CC_VS( data1, data2, data3 );	/* was CHG_CC_V */
	CHG_CC_IC( temp );

	if (( result = store_dst_2( p, data3 )) != OK )
		return result;

	return OK;
}


/*
 * bit() - Bit Test Instruction.
 */

int
bit( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	int result;

	if (( result = load_src( p, &data1 )) != OK )
		return result;
	if (( result = load_dst( p, &data2 )) != OK )
		return result;

	data2 = data1 & data2;

	CHG_CC_N( data2 );
	CHG_CC_Z( data2 );
	CLR_CC_V();

	return OK;
}


/*
 * bic() - Bit Clear Instruction.
 */

int
bic( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	int result;

	if (( result = load_src( p, &data1 )) != OK )
		return result;
	if (( result = load_dst( p, &data2 )) != OK )
		return result;

	data2 = ( ~data1 ) & data2;
	CHG_CC_N( data2 );
	CHG_CC_Z( data2 );
	CLR_CC_V();

	if (( result = store_dst_2( p, data2 )) != OK )
		return result;

	return OK;
}


/*
 * bis() - Bit Set Instruction.
 */

int
bis( p )
register pdp_regs *p;
{
	d_word data1;
	d_word data2;
	int result;

	if (( result = load_src( p, &data1 )) != OK )
		return result;
	if (( result = load_dst( p, &data2 )) != OK )
		return result;

	data2 = data1 | data2;

	CHG_CC_N( data2 );
	CHG_CC_Z( data2 );
	CLR_CC_V();

	if (( result = store_dst_2( p, data2 )) != OK )
		return result;

	return OK;
}


/*
 * movb() - Move Byte Instruction.  Move operations with registers
 * as the source and/or destination have been inlined.
 */

int
movb( p )
register pdp_regs *p;
{
	d_byte data;
	int result;

	if ( SRC_MODE ) {
		if (( result = loadb_src( p, &data )) != OK )
			return result;
	} else {
		data = LOW8( p->regs[SRC_REG] );
	}

	CHGB_CC_N( data );
	CHGB_CC_Z( data );
	CLR_CC_V();

	/* move byte to a register causes sign extension */

	if ( DST_MODE ) {
		if (( result = storeb_dst( p, data )) != OK )
			return result;
	} else {
		if ( data & SIGN_B )
			p->regs[DST_REG] = 0177400 + data;
		else
			p->regs[DST_REG] = data;
	}

	return OK;
}


/*
 * cmpb() - Compare Byte Instruction.
 */

int
cmpb( p )
register pdp_regs *p;
{
	d_byte data1;
	d_byte data2;
	d_byte data3;
	unsigned short temp;
	int result;

	if (( result = loadb_src( p, &data1 )) != OK )
		return result;
	if (( result = loadb_dst( p, &data2 )) != OK )
		return result;

	data3 = ~data2;
	temp = ((unsigned short) data1) + ((unsigned short) ( data3 )) + 1;
	data3 = LOW8( temp );

	CHGB_CC_N( data3 );
	CHGB_CC_Z( data3 );
	CHGB_CC_VC( data1, data2, data3 );
	CHGB_CC_IC( temp );

	return OK;
}


/*
 * bitb() - Bit Test Byte Instruction.
 */

int
bitb( p )
register pdp_regs *p;
{
	d_byte data1;
	d_byte data2;
	int result;

	if (( result = loadb_src( p, &data1 )) != OK )
		return result;
	if (( result = loadb_dst( p, &data2 )) != OK )
		return result;

	data2 = data1 & data2;

	CHGB_CC_N( data2 );
	CHGB_CC_Z( data2 );
	CLR_CC_V();

	return OK;
}


/*
 * bicb() - Bit Clear Byte Instruction.
 */

int
bicb( p )
register pdp_regs *p;
{
	d_byte data1;
	d_byte data2;
	int result;

	if (( result = loadb_src( p, &data1 )) != OK )
		return result;
	if (( result = loadb_dst( p, &data2 )) != OK )
		return result;

	data2 = ( ~data1 ) & data2;

	CHGB_CC_N( data2 );
	CHGB_CC_Z( data2 );
	CLR_CC_V();

	if (( result = storeb_dst_2( p, data2 )) != OK )
		return result;

	return OK;
}


/*
 * bisb() - Bit Set Byte Instruction.
 */

int
bisb( p )
register pdp_regs *p;
{
	d_byte data1;
	d_byte data2;
	int result;

	if (( result = loadb_src( p, &data1 )) != OK )
		return result;
	if (( result = loadb_dst( p, &data2 )) != OK )
		return result;

	data2 = data1 | data2;

	CHGB_CC_N( data2 );
	CHGB_CC_Z( data2 );
	CLR_CC_V();

	if (( result = storeb_dst_2( p, data2 )) != OK )
		return result;

	return OK;
}
