/*
	Declaration & settings for debugging

	The following functions expand to nothing, if DEBUG is not defined,
	otherwise they expand to the given statement.

	dprintf( a )	--> print	a	[ printf() enable while debugging ]
	dbg_printmem()	--> displays the memory and how it changed
*/

#ifndef __DEBUG__H
#define __DEBUG__H

#include <stdio.h>

extern FILE *dbg_logfile;
extern FILE *dbg_logfile2;
extern int fddebug;

#ifdef DEBUG
		/* DEBUG ENABLED */

#define dprintf(p)  do { if (fddebug) dbg_print p ; } while(0)
void dbg_printmem(void);
void dbg_print(const char * const fmt, ...);
void dbg_outc(int ch);
void dbg_outs(const char * const s);
void dbg_outsn(const char * const s);

#else
		/* NO DEBUG */

#define dprintf(p)
#define dbg_printmem()

#endif	/* defined(DEBUG) */

#endif
