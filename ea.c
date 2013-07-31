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
 * ea.c - Calculate, load, and store using the
 * proper effective address as specified by the
 * current instruction.  Also push and pop stack
 * operations.
 */


#include "defines.h"


/*
 * load_ea()
 */

load_ea( p, addr )
register pdp_regs *p;
d_word *addr;
{
	d_word indirect;
	int result;

	switch (DST_MODE) {
	case 0:	
		return CPU_ILLEGAL;
		/*NOTREACHED*/
		break;
	case 1:
		*addr = p->regs[DST_REG];
		return OK;
		/*NOTREACHED*/
		break;
	case 2:
		*addr = p->regs[DST_REG];	/* this is wrong for 11/34 */
		p->regs[DST_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 3:
		indirect = p->regs[DST_REG];	/* this is wrong for 11/34 */
		p->regs[DST_REG] += 2;
		return ll_word( p, indirect, addr );
		/*NOTREACHED*/
		break;
	case 4:
		p->regs[DST_REG] -= 2;
		*addr = p->regs[DST_REG];
		return OK;
		/*NOTREACHED*/
		break;
	case 5:
		p->regs[DST_REG] -= 2;
		indirect = p->regs[DST_REG];
		return ll_word( p, indirect, addr );
		/*NOTREACHED*/
		break;
	case 6:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		*addr = p->regs[DST_REG] + indirect;
		return OK;
		/*NOTREACHED*/
		break;
	case 7:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		indirect = p->regs[DST_REG] + indirect;
		return ll_word( p, indirect, addr );
		/*NOTREACHED*/
		break;
	}
	return CPU_ILLEGAL;
}


/*
 * pop()
 */

pop( p, data )
register pdp_regs *p;
d_word *data;
{
	int result;

	if (( result = ll_word( p, p->regs[SP], data )) != OK )
		return result;
	p->regs[SP] += 2;
	return OK;
}


/*
 * push()
 */

push( p, data )
register pdp_regs *p;
d_word data;
{
	p->regs[SP] -= 2;
	return sl_word( p, p->regs[SP], data );
}


/*
 * loadb_dst()
 */

loadb_dst( p, data )
register pdp_regs *p;
d_byte *data;
{
	d_word addr, indirect;
	int result;

	switch (DST_MODE) {
	case 0:	
		*data = p->regs[DST_REG] & 0377;
		return OK;
		/*NOTREACHED*/
		break;
	case 1:
		addr = p->regs[DST_REG];
		p->ea_addr = addr;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 2:
		addr = p->regs[DST_REG];
		p->ea_addr = addr;
		if (( result = ll_byte( p, addr, data )) != OK )
			return result;
		if ( DST_REG >= 6 )
			p->regs[DST_REG] += 2;
		else
			p->regs[DST_REG] += 1;
		return OK;
		/*NOTREACHED*/
		break;
	case 3:
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->ea_addr = addr;
		if (( result = ll_byte( p, addr, data )) != OK )
			return result;
		p->regs[DST_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 4:
		if ( DST_REG >= 6 )
			p->regs[DST_REG] -= 2;
		else
			p->regs[DST_REG] -= 1;
		addr = p->regs[DST_REG];
		p->ea_addr = addr;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 5:
		p->regs[DST_REG] -= 2;
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->ea_addr = addr;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 6:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		addr = p->regs[DST_REG] + indirect;
		p->ea_addr = addr;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 7:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		indirect = p->regs[DST_REG] + indirect;
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->ea_addr = addr;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	}
	return CPU_ILLEGAL;
}


/*
 * loadb_src()
 */

loadb_src( p, data )
register pdp_regs *p;
d_byte *data;
{
	d_word addr, indirect;
	int result;

	switch (SRC_MODE) {
	case 0:	
		*data = p->regs[SRC_REG] & 0377;
		return OK;
		/*NOTREACHED*/
		break;
	case 1:
		addr = p->regs[SRC_REG];
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 2:
		addr = p->regs[SRC_REG];
		if (( result = ll_byte( p, addr, data )) != OK )
			return result;
		if ( SRC_REG >= 6 )
			p->regs[SRC_REG] += 2;
		else
			p->regs[SRC_REG] += 1;
		return OK;
		/*NOTREACHED*/
		break;
	case 3:
		indirect = p->regs[SRC_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		if (( result = ll_byte( p, addr, data )) != OK )
			return result;
		p->regs[SRC_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 4:
		if ( SRC_REG >= 6 )
			p->regs[SRC_REG] -= 2;
		else
			p->regs[SRC_REG] -= 1;
		addr = p->regs[SRC_REG];
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 5:
		p->regs[SRC_REG] -= 2;
		indirect = p->regs[SRC_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 6:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		addr = p->regs[SRC_REG] + indirect;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 7:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		indirect = p->regs[SRC_REG] + indirect;
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return ll_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	}
	return CPU_ILLEGAL;
}


/*
 * storeb_dst() -
 */

storeb_dst( p, data )
register pdp_regs *p;
d_byte data;
{
	d_word addr, indirect;
	int result;

	switch (DST_MODE) {
	case 0:	
		p->regs[DST_REG] &= 0177400;
		p->regs[DST_REG] += data;
		return OK;
		/*NOTREACHED*/
		break;
	case 1:
		addr = p->regs[DST_REG];
		return sl_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 2:
		addr = p->regs[DST_REG];
		if (( result = sl_byte( p, addr, data )) != OK )
			return result;
		if ( DST_REG >= 6 )
			p->regs[DST_REG] += 2;
		else
			p->regs[DST_REG] += 1;
		return OK;
		/*NOTREACHED*/
		break;
	case 3:
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		if (( result = sl_byte( p, addr, data )) != OK )
			return result;
		p->regs[DST_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 4:
		if ( DST_REG >= 6 )
			p->regs[DST_REG] -= 2;
		else
			p->regs[DST_REG] -= 1;
		addr = p->regs[DST_REG];
		return sl_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 5:
		p->regs[DST_REG] -= 2;
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return sl_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 6:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		addr = p->regs[DST_REG] + indirect;
		return sl_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	case 7:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		indirect = p->regs[DST_REG] + indirect;
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return sl_byte( p, addr, data );
		/*NOTREACHED*/
		break;
	}
	return CPU_ILLEGAL;
}


/*
 * storeb_dst_2() -
 */

storeb_dst_2( p, data )
register pdp_regs *p;
d_byte data;
{
	if (DST_MODE == 0) {
		p->regs[DST_REG] &= 0177400;
		p->regs[DST_REG] += data;
		return OK;
	}
	return sl_byte( p, p->ea_addr, data );
}


/*
 * load_src()
 */

load_src( p, data )
register pdp_regs *p;
d_word *data;
{
	d_word addr, indirect;
	int result;

	switch (SRC_MODE) {
	case 0:
		*data = p->regs[SRC_REG];
		return OK;
		/*NOTREACHED*/
		break;
	case 1:
		addr = p->regs[SRC_REG];
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 2:
		addr = p->regs[SRC_REG];
		if (( result = ll_word( p, addr, data )) != OK )
			return result;
		p->regs[SRC_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 3:
		indirect = p->regs[SRC_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->regs[SRC_REG] += 2;		/* is this right ? */
		if (( result = ll_word( p, addr, data )) != OK )
			return result;
		return OK;
		/*NOTREACHED*/
		break;
	case 4:
		p->regs[SRC_REG] -= 2;
		addr = p->regs[SRC_REG];
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 5:
		p->regs[SRC_REG] -= 2;
		indirect = p->regs[SRC_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 6:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		addr = p->regs[SRC_REG] + indirect;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 7:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		indirect = p->regs[SRC_REG] + indirect;
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	}
	return CPU_ILLEGAL;
}


/*
 * store_dst() -
 */

store_dst( p, data )
register pdp_regs *p;
d_word data;
{
	d_word addr, indirect;
	int result;

	switch (DST_MODE) {
	case 0:	
		p->regs[DST_REG] = data;
		return OK;
		/*NOTREACHED*/
		break;
	case 1:
		addr = p->regs[DST_REG];
		return sl_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 2:
		addr = p->regs[DST_REG];
		if (( result = sl_word( p, addr, data )) != OK )
			return result;
		p->regs[DST_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 3:
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->regs[DST_REG] += 2;		/* is this right ? */
		if (( result = sl_word( p, addr, data )) != OK )
			return result;
		return OK;
		/*NOTREACHED*/
		break;
	case 4:
		p->regs[DST_REG] -= 2;
		addr = p->regs[DST_REG];
		return sl_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 5:
		p->regs[DST_REG] -= 2;
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return sl_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 6:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		addr = p->regs[DST_REG] + indirect;
		return sl_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 7:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		indirect = p->regs[DST_REG] + indirect;
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		return sl_word( p, addr, data );
		/*NOTREACHED*/
		break;
	}
	return CPU_ILLEGAL;
}


/*
 * load_dst()
 */

load_dst( p, data )
register pdp_regs *p;
d_word *data;
{
	d_word addr, indirect;
	int result;

	switch (DST_MODE) {
	case 0:	
		*data = p->regs[DST_REG];
		return OK;
		/*NOTREACHED*/
		break;
	case 1:
		addr = p->regs[DST_REG];
		p->ea_addr = addr;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 2:
		addr = p->regs[DST_REG];
		p->ea_addr = addr;
		if (( result = ll_word( p, addr, data )) != OK )
			return result;
		p->regs[DST_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 3:
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->ea_addr = addr;
		if (( result = ll_word( p, addr, data )) != OK )
			return result;
		p->regs[DST_REG] += 2;
		return OK;
		/*NOTREACHED*/
		break;
	case 4:
		p->regs[DST_REG] -= 2;
		addr = p->regs[DST_REG];
		p->ea_addr = addr;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 5:
		p->regs[DST_REG] -= 2;
		indirect = p->regs[DST_REG];
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->ea_addr = addr;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 6:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		addr = p->regs[DST_REG] + indirect;
		p->ea_addr = addr;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	case 7:
		if (( result = ll_word( p, p->regs[PC], &indirect )) != OK )
			return result;
		p->regs[PC] += 2;
		indirect = p->regs[DST_REG] + indirect;
		if (( result = ll_word( p, indirect, &addr )) != OK )
			return result;
		p->ea_addr = addr;
		return ll_word( p, addr, data );
		/*NOTREACHED*/
		break;
	}
	return CPU_ILLEGAL;
}


/*
 * store_dst_2() -
 */

store_dst_2( p, data )
register pdp_regs *p;
d_word data;
{
	if (DST_MODE == 0) {
		p->regs[DST_REG] = data;
		return OK;
	}
	return sl_word( p, p->ea_addr, data );
}
