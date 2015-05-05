#ifndef CPLOG_H
#define CPLOG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
@{
*/
/* the #ifndef protects these values from being clobbered by values
of the same name in Unix syslog.h */
#ifndef LOG_EMERG
/** system is unusable */
#define LOG_EMERG       0
/** action must be taken immediately */
#define LOG_ALERT       1
/** critical conditions */
#define LOG_CRIT        2
/** error conditions */
#define LOG_ERR         3
/** warning conditions */
#define LOG_WARNING     4
/** normal but significant condition */
#define LOG_NOTICE      5
/** informational */
#define LOG_INFO        6
/** debug-level messages */
#define LOG_DEBUG       7
/** stack debug-level messages */
#endif

#define LOG_DEBUG_STACK 8
/** stack operator debug-level messages */
#define LOG_DEBUG_OPER  9
/** heartbeat debug-level messages */
#define LOG_DEBUG_HB   10
/** an alias for the last cpLog priority level, for use in bounds-checking
code */
#define LAST_PRIORITY 10
/**
@}
*/
#if defined(_DEBUG)
extern void cpLog (int pri, const char *fmt, ...);
#else
#define  cpLog
#endif

/**
Set the priority level at which messages should be printed (for the current
thread).  Messages of a priority level conceptually greater than or equal
to this number (numerically less than or equal) are printed.  Messages with
conceptually lower (numerically higher) priorities than this level are
ignored.  Don't blame us for the backwards semantics; syslog started them!

@param pri the new priority level
*/
// extern void cpLogSetPriority (int pri);
#define cpLogSetPriority(n)

/**
Get the current priority level.
@return the current priority level
@see cpLogSetPriority
*/
// extern int cpLogGetPriority ();
#define cpLogGetPriority()		0

/* CPLOG_H */

#ifdef __cplusplus
}
#endif

#endif
