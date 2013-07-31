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
 * branch.c - Branch Instructions
 */


#include "defines.h"


/*
 * brx() - Simple Branch Instructions.
 */

int
brx( p, clear, set )
register pdp_regs *p;
unsigned clear;
unsigned set;
{
	d_word offset;

	last_branch = p->regs[PC];
	if ((( p->psw & set ) == set ) && (( p->psw & clear ) == 0 )) {
		offset = LOW8( p->ir );
		if ( offset & SIGN_B )
			offset += 0177400 ;
		p->regs[PC] += ( offset * 2 );
	}
	
	return OK;
}


/*
 * br() - Branch Always.
 */

int
br( p )
register pdp_regs *p;
{
	d_word offset;

	last_branch = p->regs[PC];
	offset = LOW8( p->ir );
	if ( offset & SIGN_B )
		offset += 0177400 ;
	p->regs[PC] += ( offset * 2 );
	return OK;
}


/*
 * blos() - Branch Lower or Same Instruction.
 */

int
blos( p )
register pdp_regs *p;
{
	d_word offset;

	last_branch = p->regs[PC];
	if (( p->psw & CC_C ) || ( p->psw & CC_Z )) {
		offset = LOW8( p->ir );
		if ( offset & SIGN_B )
			offset += 0177400 ;
		p->regs[PC] += ( offset * 2 );
	}

	return OK;
}


/*
 * bge() - Branch Greater Than or Equal Instruction.
 */

int
bge( p )
register pdp_regs *p;
{
	d_word offset;
	unsigned nbit;
	unsigned vbit;

	last_branch = p->regs[PC];
	nbit = ( p->psw & CC_N ) ? 1 : 0;
	vbit = ( p->psw & CC_V ) ? 1 : 0;

	if (( nbit ^ vbit ) == 0 ) {
		offset = LOW8( p->ir );
		if ( offset & SIGN_B )
			offset += 0177400 ;
		p->regs[PC] += ( offset * 2 );
	}

	return OK;
}


/*
 * blt() - Branch Less Than Instruction.
 */

int
blt( p )
register pdp_regs *p;
{
	d_word offset;
	unsigned nbit;
	unsigned vbit;

	last_branch = p->regs[PC];
	nbit = ( p->psw & CC_N ) ? 1 : 0;
	vbit = ( p->psw & CC_V ) ? 1 : 0;

	if (( nbit ^ vbit ) == 1 ) {
		offset = LOW8( p->ir );
		if ( offset & SIGN_B )
			offset += 0177400 ;
		p->regs[PC] += ( offset * 2 );
	}

	return OK;
}


/*
 * ble() - Branch Less Than Or Equal Instruction.
 */

int
ble( p )
register pdp_regs *p;
{
	d_word offset;
	unsigned nbit;
	unsigned vbit;

	last_branch = p->regs[PC];
	nbit = ( p->psw & CC_N ) ? 1 : 0;
	vbit = ( p->psw & CC_V ) ? 1 : 0;

	if ((( nbit ^ vbit ) == 1) || (p->psw & CC_Z)) {
		offset = LOW8( p->ir );
		if ( offset & SIGN_B )
			offset += 0177400 ;
		p->regs[PC] += ( offset * 2 );
	}

	return OK;
}


/*
 * bgt() - Branch Greater Than Instruction.
 */

int
bgt( p )
register pdp_regs *p;
{
	d_word offset;
	unsigned nbit;
	unsigned vbit;

	last_branch = p->regs[PC];
	nbit = ( p->psw & CC_N ) ? 1 : 0;
	vbit = ( p->psw & CC_V ) ? 1 : 0;

	if ((( nbit ^ vbit ) == 0) && (( p->psw & CC_Z ) == 0 )) {
		offset = LOW8( p->ir );
		if ( offset & SIGN_B )
			offset += 0177400 ;
		p->regs[PC] += ( offset * 2 );
	}

	return OK;
}


/*
 * jmp() - Jump Instruction.
 */

int
jmp( p )
register pdp_regs *p;
{
	int result;

	last_branch = p->regs[PC];
	if (( result = load_ea( p, &( p->regs[PC] ))) != OK )
		return result;

	return OK;
}



/*
 * jsr() - Jump To Subroutine Instruction.
 */

int
jsr( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	last_branch = p->regs[PC];
	if (( result = load_ea( p, &data )) != OK )
		return result;

	if (( result = push( p, p->regs[SRC_REG] )) != OK )
		return result;

	p->regs[SRC_REG] = p->regs[PC];
	p->regs[PC] = data;

	return OK;
}


/*
 * rts() - Return From Subroutine Instruction.
 */

int
rts( p )
register pdp_regs *p;
{
	d_word data;
	int result;

	last_branch = p->regs[PC];
	p->regs[PC] = p->regs[DST_REG];

	if (( result = pop( p, &data )) != OK )
		return result;

	p->regs[DST_REG] = data;

	return OK;
}

