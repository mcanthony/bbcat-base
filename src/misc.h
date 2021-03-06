#ifndef __MISC__
#define __MISC__

#include <stdint.h>
#include <stdarg.h>

#include <vector>
#include <string>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef LIMIT
#define LIMIT(a, b, c) (((a) < (b)) ? (b) : (((a) > (c)) ? (c) : (a)))
#endif
#ifndef NUMBEROF
#define NUMBEROF(x) (sizeof(x) / sizeof(x[0]))
#endif
#ifndef RANGE
#define RANGE(a, b, c) (((a) >= (b)) && ((b) <= (c)))
#endif

#define IFFID(name) (((uint32_t)name[0] << 24) | ((uint32_t)name[1] << 16) | ((uint32_t)name[2] << 8) | (uint32_t)name[3])

#define UNUSED_PARAMETER(name) ((void)(name))

#define USE_BBC_AUDIOTOOLBOX using namespace bbcat;
#define BBC_AUDIOTOOLBOX_START namespace bbcat {
#define BBC_AUDIOTOOLBOX_END }

#ifdef __BYTE_ORDER__
// endianness can be determined at compile time
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
// use #define to allow compiler to optimize
#define MACHINE_IS_BIG_ENDIAN false
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
// use #define to allow compiler to optimize
#define MACHINE_IS_BIG_ENDIAN true
#endif
#else
// endianness cannot be determined at compile time, determine at run-time
// stored in ByteSwap.cpp
extern const bool MACHINE_IS_BIG_ENDIAN;
#endif

typedef int16_t       sint16_t;
typedef int32_t       sint32_t;
typedef int64_t       sint64_t;
typedef signed   int  sint_t;
typedef unsigned int  uint_t;
typedef signed   long slong_t;
typedef unsigned long ulong_t;
typedef signed   long long sllong_t;
typedef unsigned long long ullong_t;

#define PACKEDSTRUCT   struct __attribute__ ((packed))
#define PRINTF_FORMAT  __attribute__ ((format (printf,1,2)))
#define PRINTF_FORMAT2 __attribute__ ((format (printf,2,3)))

BBC_AUDIOTOOLBOX_START

typedef void (*DEBUGHANDLER)(const char *str, void *context);

extern void debug_msg(const char *fmt, ...) PRINTF_FORMAT;
extern void debug_err(const char *fmt, ...) PRINTF_FORMAT;

extern void enable_pipe(bool enable = true);
extern bool is_pipe_enabled();
extern void pipe_msg(const char *fmt, ...) PRINTF_FORMAT;
extern bool get_pipe_msg(const char *str, ulong_t& tick, std::string& str2);

extern const char *CreateString(const char *data, uint_t len);
extern void FreeStrings();

#define ERROR debug_err
#define DEBUG debug_msg

/*--------------------------------------------------------------------------------*/
/** Debug output control
 *
 * Before including any include files from bbcat-*, #define DEBUG_LEVEL to a value
 *
 * The value dictates which debug informatin (in that file) is output:
 * DEBUG_LEVEL  Debug output
 * 0            ERROR() and DEBUG() ONLY
 * 1            As above plus DEBUG1(())
 * 2            As above plus DEBUG2(())
 * 3            As above plus DEBUG3(())
 * 4            As above plus DEBUG4(())
 * 5            As above plus DEBUG5(())
 *
 * Note: the double-parentheses are required because they allow debug lines that are
 *       NOT going to be outputted to be converted to (void)0 statements (unlike
 *       lower level controls which will evaluate the entrie string and arguments
 *       and then throw it away)
 *
 * If DEBUG_LEVEL is not defined, it is set to 0
 */
/*--------------------------------------------------------------------------------*/
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#if DEBUG_LEVEL >= 1
#define DEBUG1(x) debug_msg x
#else
#define DEBUG1(x) (void)0
#endif

#if DEBUG_LEVEL >= 2
#define DEBUG2(x) debug_msg x
#else
#define DEBUG2(x) (void)0
#endif

#if DEBUG_LEVEL >= 3
#define DEBUG3(x) debug_msg x
#else
#define DEBUG3(x) (void)0
#endif

#if DEBUG_LEVEL >= 4
#define DEBUG4(x) debug_msg x
#else
#define DEBUG4(x) (void)0
#endif

#if DEBUG_LEVEL >= 5
#define DEBUG5(x) debug_msg x
#else
#define DEBUG5(x) (void)0
#endif

#if DEBUG_LEVEL >= 6
#define DEBUG6(x) debug_msg x
#else
#define DEBUG6(x) (void)0
#endif

#if DEBUG_LEVEL >= 7
#define DEBUG7(x) debug_msg x
#else
#define DEBUG7(x) (void)0
#endif

#if DEBUG_LEVEL >= 8
#define DEBUG8(x) debug_msg x
#else
#define DEBUG8(x) (void)0
#endif

#if DEBUG_LEVEL >= 9
#define DEBUG9(x) debug_msg x
#else
#define DEBUG9(x) (void)0
#endif

// fundamental audio processing unit - don't change without careful consideration!
typedef float Sample_t;

typedef PACKEDSTRUCT
{
  uint8_t b[10];
} IEEEEXTENDED;

extern ulong_t  GetTickCount();

extern uint32_t IEEEExtendedToINT32u(const IEEEEXTENDED *num);
extern void     INT32uToIEEEExtended(uint32_t val, IEEEEXTENDED *num);

/*--------------------------------------------------------------------------------*/
/** Set debug handler (replacing printf())
 *
 * @param handler debug handler
 * @param context optional parameter to pass to handler
 *
 */
/*--------------------------------------------------------------------------------*/
extern void SetDebugHandler(DEBUGHANDLER handler, void *context = NULL);

/*--------------------------------------------------------------------------------*/
/** Set error handler (replacing printf())
 *
 * @param handler error handler
 * @param context optional parameter to pass to handler
 *
 */
/*--------------------------------------------------------------------------------*/
extern void SetErrorHandler(DEBUGHANDLER handler, void *context = NULL);

/*--------------------------------------------------------------------------------*/
/** Create indentation string
 *
 * @param indent a string representing one level of indentation (e.g. a tab or spaces)
 * @param count indentation level
 *
 * @return a string containing <count> copies of <indent>
 */
/*--------------------------------------------------------------------------------*/
extern std::string CreateIndent(const std::string& indent, uint_t count);

/*--------------------------------------------------------------------------------*/
/** printf for std::string
 *
 * @param str string to be added to
 * @param fmt printf-style format information
 *
 */
/*--------------------------------------------------------------------------------*/
extern void Printf(std::string& str, const char *fmt, ...) PRINTF_FORMAT2;

/*--------------------------------------------------------------------------------*/
/** vprintf for std::string
 *
 * @param str string to be added to
 * @param fmt printf-style format information
 * @param ap ap_list of arguments
 *
 */
/*--------------------------------------------------------------------------------*/
extern void VPrintf(std::string& str, const char *fmt, va_list ap);

/*--------------------------------------------------------------------------------*/
/** Split a string by a delimiter, allowing for quotes to prevent splitting in the wrong place
 *
 * @param str string split
 * @param list list to be populated
 * @param delim delimiter character
 * @param maxstrings if non-zero specifies the maximum number of entries in list
 *
 * @return position in string when scanning stopped
 * 
 * @note whitespace is IGNORED!
 */
/*--------------------------------------------------------------------------------*/
extern uint_t SplitString(const std::string& str, std::vector<std::string>& list, char delim = ' ', uint_t maxstrings = 0);

/*--------------------------------------------------------------------------------*/
/** Interpolate current towards target at rate coeff, protecting against denormals
 */
/*--------------------------------------------------------------------------------*/
extern void Interpolate(double& current, double target, double coeff, double limit = 1.0e-7);

/*--------------------------------------------------------------------------------*/
/** 'cout' like debug stream handling
 * 
 * Basically use local instance of StringStream instead of 'cout' and then wrap entire line in DEBUG() macro
 *
 * For example:
 * DEBUG2((StringStream() << "Test " << i << " of " << n << ":" << StringStream::eol));
 *
 */
/*--------------------------------------------------------------------------------*/
class StringStream
{
public:
  StringStream();
  ~StringStream();

  StringStream& operator << (const std::string& str);
  StringStream& operator << (const char *str);
  StringStream& operator << (sint_t n);
  StringStream& operator << (uint_t n);
  StringStream& operator << (slong_t n);
  StringStream& operator << (ulong_t n);
  StringStream& operator << (sllong_t n);
  StringStream& operator << (ullong_t n);

  const char *get() const;
  operator const char *() const {return get();}
  void clear();

  static const char *eol;

protected:
  std::string data;
};

extern void debug_msg(StringStream& str);
extern void debug_err(StringStream& str);

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs)
{
  // No predicate needed because there is operator== for pairs already.
  return ((lhs.size() == rhs.size()) &&
          std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

/*--------------------------------------------------------------------------------*/
/** Prevent value becoming denormalized (bad for performance!)
 */
/*--------------------------------------------------------------------------------*/
extern float  fix_denormal(float val);
extern double fix_denormal(double val);

typedef struct {
  const char *name;
  const char *desc;
} PARAMETERDESC;

/*--------------------------------------------------------------------------------*/
/** Add a list of parameters/controls to a list
 */
/*--------------------------------------------------------------------------------*/
extern void AddParametersToList(const PARAMETERDESC *parameters, uint_t n, std::vector<const PARAMETERDESC *>& list);

/*--------------------------------------------------------------------------------*/
/** Convert dB to gain assuming <=-120 is cut
 */
/*--------------------------------------------------------------------------------*/
extern double dBToGain(double db);

/*--------------------------------------------------------------------------------*/
/** Convert gain to dB to gain (-120 is the minimum, including cut)
 */
/*--------------------------------------------------------------------------------*/
extern double GainTodB(double gain);

/*--------------------------------------------------------------------------------*/
/** Attempt to evaluate values from strings
 */
/*--------------------------------------------------------------------------------*/
extern bool Evaluate(const std::string& str, bool& val);
extern bool Evaluate(const std::string& str, sint_t& val);
extern bool Evaluate(const std::string& str, uint_t& val);
extern bool Evaluate(const std::string& str, slong_t& val);
extern bool Evaluate(const std::string& str, ulong_t& val);
extern bool Evaluate(const std::string& str, double& val);

extern const char *DoubleFormatHuman;         // format as human-readable, scientific format with 32 decimal places
extern const char *DoubleFormatExact;         // format as hex-encoded double (exact)
extern std::string StringFrom(bool val);
extern std::string StringFrom(sint_t val, const char *fmt = "%d");
extern std::string StringFrom(uint_t val, const char *fmt = "%u");
extern std::string StringFrom(slong_t val, const char *fmt = "%ld");
extern std::string StringFrom(ulong_t val, const char *fmt = "%lu");
extern std::string StringFrom(double val, const char *fmt = DoubleFormatHuman);

/*--------------------------------------------------------------------------------*/
/** Bog-standard string search and replace that *should* be in std::string!
 */
/*--------------------------------------------------------------------------------*/
extern std::string SearchAndReplace(const std::string& str, const std::string& search, const std::string& replace);

/*--------------------------------------------------------------------------------*/
/** Very simple wildcard matching
 *
 * @param pat pattern containing characters and/or '*' / '?' 
 * @param str string to match
 *
 * @return true if string matches pattern
 *
 * @note this routine is highly recursive!
 */
/*--------------------------------------------------------------------------------*/
extern bool matchstring(const char *pat, const char *str);

BBC_AUDIOTOOLBOX_END

#endif
