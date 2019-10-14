/*	platform.h - computer platform customization for PGP
	multiprecision math package.  #Included in mpilib.h.
*/
#ifndef PLATFORM_H
#define PLATFORM_H

/* Platform customization:
 * A version which runs on almost any computer can be implemented by
 * defining PORTABLE and MPORTABLE, preferably as a command line
 * parameter.  Faster versions can be generated by specifying specific
 * parameters, such as size of unit and MULTUNIT, and by supplying some
 * of the critical in assembly.  
 *
 * This file holds customizations for different environments.
 * This is done in one of two ways:
 *	1. A symbol is defined on the command line which designates a 
 *	particular environment, such as MSDOS.  This file detects the 
 *	environment symbol and sets the appropriate low-level defines.
 *
 *	2. If no environment is named, the low-level defines are set in
 *	the same manner as for PGP 2.0, thereby providing an easy upgrade.
 *
 * Following are a description of the low-level definition symbols:
 *
 * The following preprocessor symbols should be conditionally set to 
 * optimize for a particular environment.
 *
 * Define one of the following:
 *	UNIT8, UNIT16, or UNIT32	- specifies size of operands for
 *	multiprecision add, subtract, shift, and initialization operations.
 * Define one of the following:
 *	MUNIT8, MUNIT16, MUNIT32	- specified size of operands for 
 *	multiprecision multiply and mod_mult.  This must be less than or
 *	equal to unit size.  It should be the word size for the native
 *	atomic multiply instruction.  For a 16x16 bit multiply yielding a
 *	32-bit product, MUNIT16 should be set.
 * Define one (or more) of the following:
 *	PEASANT, MERRITT, UPTON, SMITH	-algorithm used for modmult.  All defined
 *	algorithms are compiled, but the first defined name listed will be 
 *	assigned to the generic entry point symbols.  Multiple algorithms are
 *	used primarily for testing.
 * HIGHFIRST - specified if longs are stored with the most significant
 *	bit at the lowest address (Motorola), undefined otherwise.  This should
 *	be defined on the command line, normally in the makefile.
 *
 * The following symbol, if initialized, is set to specific values:
 * ALIGN - variable declaration attribute which forces optimum alignment
 *	of words, e.g. for VAX C: ALIGN=_align(quadword)
 *
 * The following symbols correspond to individual multiprecision routines
 * that may be implemented with assembly language.  If they are implemented
 * in assembly, the symbols should be defined with the name of the
 * corresponding external entry points, e.g., mp_addc=P_ADDC
 *	mp_setp        - set precision for external routines
 *	mp_addc        - add with carry
 *	mp_subb        - subtract with borrow
 *	mp_rotate_left - rotate left
 *	mp_compare     - compare
 *	mp_move        - move
 *	unitfill0      - zero fill
 *	mp_smul        - multiply vector by single word *
 *	mp_smula       - multiply vector by single word and accumulate *
 *	mp_dmul        - full multiply 
 *	mp_set_recip   - setup for mp_quo_digit
 *	mp_quo_digit   - quotient digit for modulus reduction
 *
 * Either mp_smul or mp_smula should be defined.  mp_smula provides
 * for accumulation to an existing value, while mp_smul is for use of the
 * older definition of mp_smul, used in PGP 2.0, which assumed that the high 
 * order accumulator word is zero.   Use of mp_smula causes one less word of 
 * precision to be used, thereby slightly increasing speed.
 */

/********************************************************************
 * Environment customization.  Please send any additions or corrections
 * to Philip Zimmermann.
 */
#ifndef PORTABLE

#ifdef MSDOS
#define UNIT16
#define MUNIT16
#define mp_setp		P_SETP
#define mp_addc		P_ADDC
#define mp_subb		P_SUBB
#define mp_rotate_left	P_ROTL
#define mp_smula	P_SMULA
#define mp_quo_digit	P_QUO_DIGIT
#define mp_set_recip	P_SETRECIP
#define SMITH
#define PLATFORM_SPECIFIED
#endif /* MSDOS */

#ifdef VMS
#define UNIT32		 /* use 32-bit units */
#define MUNIT32		/* not used in C code, only in assembler */
#define UPTON
#define mp_setp		p_setp
#define mp_addc		p_addc
#define mp_subb		p_subb
#define mp_rotate_left	p_rotl
#define mp_smul	p_smul
#define mp_dmul	p_dmul
#define mp_compare	p_cmp
#define ALIGN _align(quadword)

#ifdef VAXC
/*
 * A VAX is a CISC machine. Unfortunately C is at to low a level to use
 * many of the instruction set enhancements so we define some macros
 * here that implement fast moves and fast zero fills with single
 * instructions.
 */
#pragma builtins
#define mp_move( dst, src)	  _MOVC3( global_precision*4, (char *) src, (char *) dst)
#define unitfill0( r, unitcount) _MOVC5( 0, (char *) 0, 0, unitcount*4, (char *) r)
#define mp_burn(r) _MOVC5(0, (char *) 0, 0, global_precision*4, (char *) r)
#define mp_init0(r) mp_burn(r)	/* Just for documentation purposes */
#endif	/* VAXC */

#define PLATFORM_SPECIFIED
#endif /* VMS */

#ifdef mips
/*
 * Needs r3kd.s and r3000.s (or r3000.c)
 */
#define UNIT32
#define MUNIT32
#define SMITH
#define mp_dmul		p_dmul
#define mp_setp		p_setp
#define mp_addc		p_addc
#define mp_subb		p_subb
#define mp_rotate_left	p_rotl
#define mp_smula	p_smula
#define mp_quo_digit	p_quo_digit
#define mp_set_recip	p_setrecip
#define PLATFORM_SPECIFIED
#endif /* mips */

#ifdef i386
/*
 * Needs 80386.S
 */
#define UNIT32
#define MUNIT32
#define SMITH
#define mp_setp		P_SETP
#define mp_addc		P_ADDC
#define mp_subb		P_SUBB
#define mp_rotate_left	P_ROTL
#define unitfill0(r,ct) memset((void*)r, 0, (ct)*sizeof(unit))
#define mp_smula	P_SMULA
#define mp_quo_digit	p_quo_digit
#define mp_set_recip	p_setrecip
#define PLATFORM_SPECIFIED
#endif /* i386 */

#ifdef sparc
/*
 * Needs sparc.s
 */
#define UNIT32
#define MERRITT
#define mp_setp		P_SETP
#define mp_addc		P_ADDC
#define mp_subb		P_SUBB
#define mp_rotate_left	P_ROTL
#define unitfill0(r,ct) memset((void*)r, 0, (ct)*sizeof(unit))
#define PLATFORM_SPECIFIED
#endif /* sparc */

/* Add additional platforms here ... */

/**************** End of system specification ************************/

#ifndef PLATFORM_SPECIFIED
/* No platform explicitly selected.  Customization is controlled by
 * PORTABLE and MPORTABLE.
 */
#define mp_setp		P_SETP
#define mp_addc		P_ADDC
#define mp_subb		P_SUBB
#define mp_rotate_left	P_ROTL
#define UPTON
#define unitfill0(r,ct) memset((void*)r, 0, (ct)*sizeof(unit))
#ifndef MPORTABLE
#define mp_smul	P_SMUL
#endif	/* MPORTABLE */
#endif	/* PLATFORM_SPECIFIED */
#endif	/* PORTABLE */
#endif	/* PLATFORM_H */

