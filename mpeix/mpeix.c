
/*
 * gcc long pointer support code for HPPA.
 * Copyright 1998, DIS International, Ltd.
 * This code is free software; you may redistribute it and/or modify
 * it under the same terms as Perl itself.  (Relicensed for Perl in
 * in April 2002 by Mark Klein.)
 */
typedef struct {
  int           spaceid;
  unsigned int  offset;
  } LONGPOINTER, longpointer;

/*
 * gcc long pointer support code for HPPA.
 * Copyright 1998, DIS International, Ltd.
 * This code is free software; you may redistribute it and/or modify
 * it under the same terms as Perl itself.  (Relicensed for Perl in
 * in April 2002 by Mark Klein.)
 */

int __perl_mpe_getspaceid(void *source)
  {
  int val;
  /*
   * Given the short pointer, determine it's space ID.
   */

  /*
   * The colons separate output from input parameters. In this case,
   * the output of the instruction (output indicated by the "=" in the
   * constraint) is to a memory location (indicated by the "m"). The
   * input constraint indicates that the source to the instruction
   * is a register reference (indicated by the "r").
   * The general format is:
   *   asm("<instruction template>" : <output> : <input> : <clobbers>);
   *     where <output> and <input> are:
   *       "<constraint>" (<token>)
   *     <instruction template> is the PA-RISC instruction in template fmt.
   *     <clobbers> indicates those registers clobbered by the instruction
   *     and provides hints to the optimizer.
   *
   * Refer to the gcc documentation or http://www.dis.com/gnu/gcc_toc.html
   */
  __asm__ __volatile__ (
      "   comiclr,= 0,%1,%%r28\n"
      "\t  ldsid (%%r0,%1),%%r28\n"
      "\t stw %%r28, %0"
                        : "=m" (val)    // Output to val
                        : "r" (source)  // Source must be gen reg
                        : "%r28");      // Clobbers %r28
  return (val);
  };

LONGPOINTER __perl_mpe_longaddr(void *source)
  {
  LONGPOINTER lptr;
  /*
   * Return the long pointer for the address in sr5 space.
   */

  __asm__ __volatile__ (
      "  comiclr,= 0,%2,%%r28\n"
      "\t    ldsid (%%r0,%2),%%r28\n"
      "\t  stw %%r28, %0\n"
      "\t  stw %2, %1"
                        : "=m" (lptr.spaceid),
                          "=m" (lptr.offset)    // Store to lptr
                        : "r" (source)          // Source must be gen reg
                        : "%r28");      // Clobbers %r28
  return (lptr);
  };

LONGPOINTER __perl_mpe_addtopointer(LONGPOINTER source,    // %r26 == source offset
                                                // %r25 == source space
                        int             len)    // %r24 == length in bytes
  {
  /*
   * Increment a longpointer.
   */

  __asm__ __volatile__ (
      "  copy %0,%%r28\n"                       // copy space to r28
      "\t  add %1,%2,%%r29"                     // Increment the pointer
                        :                       // No output
                        : "r" (source.spaceid), // Source address
                          "r" (source.offset),
                          "r" (len)             // Length
                        : "%r28",               // Clobbers
                          "%r29");
  };

void __perl_mpe_longmove(int len,                  // %r26 == byte length
              LONGPOINTER source,       // %r23 == source space, %r24 == off
              LONGPOINTER target)       // sp-#56 == target space, sp-#52== off
  {
  /*
   * Move data between two buffers in long pointer space.
   */

  __asm__ __volatile__ (
      "  .import $$lr_unk_unk_long,MILLICODE\n"
      "\t  mtsp %0,%%sr1\n"                     // copy source space to sr1
      "\t  copy %1,%%r26\n"                     // load source offset to r26
      "\t  copy %4,%%r24\n"                     // load length to r24
      "\t  copy %3,%%r25\n"                     // load target offset to r25
      "\t  bl $$lr_unk_unk_long,%%r31\n"        // start branch to millicode
      "\t  mtsp %2,%%sr2"                       // copy target space to sr2
                        :                       // No output
                        : "r" (source.spaceid), // Source address
                          "r" (source.offset),
                          "r" (target.spaceid), // Target address
                          "r" (target.offset),
                          "r" (len)             // Byte length
                        : "%r1",                // Clobbers
                          "%r24",
                          "%r25",
                          "%r26",
                          "%r31");
  };

int __perl_mpe_longpeek(LONGPOINTER source)
  {
  /*
   * Fetch the int in long pointer space.
   */
  unsigned int val;

  __asm__ __volatile__ (
      "  mtsp %1, %%sr1\n"
      "\t  copy %2, %%r28\n"
      "\t  ldw 0(%%sr1, %%r28), %%r28\n"
      "\t  stw %%r28, %0"
                        : "=m" (val)            // Output val
                        : "r" (source.spaceid), // Source space ID
                          "r" (source.offset)   // Source offset
                        : "%r28");              // Clobbers %r28

  return (val);
  };

void __perl_mpe_longpoke(LONGPOINTER target,       // %r25 == spaceid, %r26 == offset
          unsigned int val)             // %r24 == value
  {
  /*
   * Store the val into long pointer space.
   */
  __asm__ __volatile__ (
      "  mtsp %0,%%sr1\n"
      "\t  copy %1, %%r28\n"
      "\t  stw %2, 0(%%sr1, %%r28)"
                        :                       // No output
                        : "r" (target.spaceid), // Target space ID
                          "r" (target.offset),  // Target offset
                          "r" (val)             // Value to store
                        : "%r28"                // Clobbers %r28
                        );                      // Copy space to %sr1
  };

void __perl_mpe_move_fast(int len,                 // %r26 == byte length
               void *source,            // %r25 == source addr
               void *target)            // %r24 == target addr
  {
  /*
   * Move using short pointers.
   */
  __asm__ __volatile__ (
      "  .import $$lr_unk_unk,MILLICODE\n"
      "\t  copy %1, %%r26\n"                    // Move source addr into pos
      "\t  copy %2, %%r25\n"                    // Move target addr into pos
      "\t  bl $$lr_unk_unk,%%r31\n"             // Start branch to millicode
      "\t  copy %0, %%r24"                      // Move length into position
                        :                       // No output
                        : "r" (len),            // Byte length
                          "r" (source),         // Source address
                          "r" (target)          // Target address
                        : "%r24",               // Clobbers
                          "%r25",
                          "%r26",
                          "%r31");
  };

/*
 * ftruncate - set file size, BSD Style
 *
 * shortens or enlarges the file as neeeded
 * uses some undocumented locking call. It is known to work on SCO unix,
 * other vendors should try.
 * The #error directive prevents unsupported OSes
 *
 * ftruncate/truncate code by Mark Bixby.
 * This code is free software; you may redistribute it and/or modify
 * it under the same terms as Perl itself.
 *
 */

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <mpe.h>

extern void FCONTROL(short, short, longpointer);
extern void PRINTFILEINFO(int);

int ftruncate(int fd, long wantsize);

int ftruncate(int fd, long wantsize) {

int ccode_return,dummy=0;

if (lseek(fd, wantsize, SEEK_SET) < 0) {
        return (-1);
}

FCONTROL(_mpe_fileno(fd),6,__perl_mpe_longaddr(&dummy)); /* Write new EOF */
if ((ccode_return=ccode()) != CCE) {
        fprintf(stderr,"MPE ftruncate failed, ccode=%d, wantsize=%ld\n",ccode_return,wantsize);
        PRINTFILEINFO(_mpe_fileno(fd));
	errno = ESYSERR;
	return (-1);
}

return (0);
}

/*
   wrapper for truncate():

   truncate() is UNIX, not POSIX.

   This function requires ftruncate().



   NAME
      truncate -

   SYNOPSIS
      #include <unistd.h>

      int truncate(const char *pathname, off_t length);

                                             Returns: 0 if OK, -1 on error

            from: Stevens' Advanced Programming in the UNIX Environment, p. 92



   ERRORS
      EACCES
      EBADF
      EDQUOT (not POSIX)    <- not implemented here
      EFAULT
      EINVAL
      EISDIR
      ELOOP (not POSIX)     <- not implemented here
      ENAMETOOLONG
      ENOTDIR
      EROFS
      ETXTBSY (not POSIX)   <- not implemented here

                                          from: HP-UX man page



   Compile directives:
      PRINT_ERROR - make this function print an error message to stderr
*/

#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE
#endif

#include <sys/types.h>	/* off_t, required by open() */
#include <sys/stat.h>	/* required by open() */
#include <fcntl.h>	/* open() */
#include <unistd.h>	/* close() */
#include <stdio.h>	/* perror(), sprintf() */



int
truncate(const char *pathname, off_t length)
{
	int fd;
#ifdef PRINT_ERROR
	char error_msg[80+1];
#endif

	if (length == 0)
	{
		if ( (fd = open(pathname, O_WRONLY | O_TRUNC)) < 0)
		{
			/* errno already set */
#ifdef PRINT_ERROR
			sprintf(error_msg,
			        "truncate(): open(%s, O_WRONLY | OTRUNC)\0",
			        pathname);
			perror(error_msg);
#endif
			return -1;
		}
	}
	else
	{
		if ( (fd = open(pathname, O_WRONLY)) < 0)
		{
			/* errno already set */
#ifdef PRINT_ERROR
			sprintf(error_msg,
			        "truncate(): open(%s, O_WRONLY)\0",
			        pathname);
			perror(error_msg);
#endif
			return -1;
		}

		if (ftruncate(fd, length) < 0)
		{
			/* errno already set */
#ifdef PRINT_ERROR
			perror("truncate(): ftruncate()");
#endif
			return -1;
		}
	}

	if (close(fd) < 0)
	{
		/* errno already set */
#ifdef PRINT_ERROR
		perror("truncate(): close()");
#endif
		return -1;
	}

	return 0;
} /* truncate() */

/* 
   wrapper for gettimeofday():
      gettimeofday() is UNIX, not POSIX.
      gettimeofday() is a BSD function.

   NAME
      gettimeofday -

   SYNOPSIS
      #include <sys/time.h>

      int gettimeofday(struct timeval *tp, struct timezone *tzp);

   DESCRIPTION
      This function returns seconds and microseconds since midnight
      January 1, 1970. The microseconds is actually only accurate to
      the millisecond.

      Note: To pick up the definitions of structs timeval and timezone
            from the <time.h> include file, the directive
            _SOCKET_SOURCE must be used.

   RETURN VALUE
      A 0 return value indicates that the call succeeded.  A -1 return
      value indicates an error occurred; errno is set to indicate the
      error.

   ERRORS
      EFAULT     not implemented

   Changes:
      2-91    DR.  Created.
*/


/* need _SOCKET_SOURCE to pick up structs timeval and timezone in time.h */
#ifndef _SOCKET_SOURCE
# define _SOCKET_SOURCE
#endif

#include <time.h>	/* structs timeval & timezone,
				difftime(), localtime(), mktime(), time() */
#include <sys/time.h>	/* gettimeofday() */

extern int TIMER();

/*
 * gettimeofday code by Mark Bixby.
 * This code is free software; you may redistribute it and/or modify
 * it under the same terms as Perl itself.
 */

#ifdef __STDC__
int gettimeofday( struct timeval *tp, struct timezone *tpz )
#else
int gettimeofday(  tp, tpz )
struct timeval  *tp;
struct timezone *tpz;
#endif
{
   static unsigned long    basetime        = 0;
   static int              dsttime         = 0;
   static int              minuteswest     = 0;
   static int              oldtime         = 0;
   register int            newtime;


   /*-------------------------------------------------------------------*/
   /* Setup a base from which all future time will be computed.         */
   /*-------------------------------------------------------------------*/
   if ( basetime == 0 )
   {
      time_t    gmt_time;
      time_t    loc_time;
      struct tm *loc_time_tm;

      gmt_time    = time( NULL );
      loc_time_tm = localtime( &gmt_time ) ;
      loc_time    = mktime( loc_time_tm );

      oldtime     = TIMER();
      basetime    = (unsigned long) ( loc_time - (oldtime/1000) );

      /*----------------------------------------------------------------*/
      /* The calling process must be restarted if timezone or dst       */
      /* changes.                                                       */
      /*----------------------------------------------------------------*/
      minuteswest = (int) (difftime( loc_time, gmt_time ) / 60);
      dsttime     = loc_time_tm->tm_isdst;
   }

   /*-------------------------------------------------------------------*/
   /* Get the new time value. The timer value rolls over every 24 days, */
   /* so if the delta is negative, the basetime value is adjusted.      */
   /*-------------------------------------------------------------------*/
   newtime = TIMER();
   if ( newtime < oldtime )  basetime += 2073600;
   oldtime = newtime;

   /*-------------------------------------------------------------------*/
   /* Return the timestamp info.                                        */
   /*-------------------------------------------------------------------*/
   tp->tv_sec          = basetime + newtime/1000;
   tp->tv_usec         = (newtime%1000) * 1000;   /* only accurate to milli */
   if (tpz)
   {
      tpz->tz_minuteswest = minuteswest;
      tpz->tz_dsttime     = dsttime;
   }

   return 0;

} /* gettimeofday() */

/*
**  MPE_FCNTL -- shadow function for fcntl()
**
**	MPE requires sfcntl() for sockets, and fcntl() for everything 
**	else.  This shadow routine determines the descriptor type and
**	makes the appropriate call.
**
**	Parameters:
**		same as fcntl().
**
**	Returns:
**		same as fcntl().
*/

#include <stdarg.h>
#include <sys/socket.h>

int
mpe_fcntl(int fildes, int cmd, ...)
{
	int len, result;
	struct sockaddr sa;
	
	void *arg;
	va_list ap;
	
	va_start(ap, cmd);
	arg = va_arg(ap, void *);
	va_end(ap);
	
	len = sizeof sa;
	if (getsockname(fildes, &sa, &len) == -1)
	{
	        if (errno == EAFNOSUPPORT)
			/* AF_UNIX socket */
	                return sfcntl(fildes, cmd, arg);

	        if (errno == ENOTSOCK) 
			/* file or pipe */
	                return fcntl(fildes, cmd, arg);

		/* unknown getsockname() failure */
	        return (-1); 
	}
	else
	{
		/* AF_INET socket */
		if ((result = sfcntl(fildes, cmd, arg)) != -1 && cmd == F_GETFL)
			result |= O_RDWR;  /* fill in some missing flags */
	        return result;
	}
}
