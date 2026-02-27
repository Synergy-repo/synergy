#ifndef COMPILER_H
#define COMPILER_H

#define CACHE_LINE_SIZE 64

#ifndef likely
#define likely( x ) __builtin_expect ( !!( x ), 1 )
#define unlikely( x ) __builtin_expect ( !!( x ), 0 )
#endif

#ifndef __noreturn
#define __noreturn __attribute__ ( ( noreturn ) )
#endif

#ifndef __packed
#define __packed __attribute__ ( ( packed ) )
#endif

#ifndef __notused
#define __notused __attribute__ ( ( unused ) )
#endif

#ifndef __aligned
#define __aligned( x ) __attribute__ ( ( aligned ( x ) ) )
#endif

#ifndef __unreachable
#define __unreachable() __builtin_unreachable ()
#endif

#ifndef __noretun
#define __noretun __attribute__ ( ( noreturn ) )
#endif

#if __GNUC__ >= 11
#define __nofp __attribute__ ( ( target ( "general-regs-only" ) ) )
#else
#define __nofp
#endif

#endif
