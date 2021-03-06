
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#ifdef __MACH__
#include <mach/mach_time.h>
#endif

#include <vector>

#define DEBUG_LEVEL 1

#include "misc.h"
// explicit use of current directory's ByteSwap.h
#include "./ByteSwap.h"
#include "ThreadLock.h"
#include "EnhancedFile.h"

BBC_AUDIOTOOLBOX_START

static std::vector<char *> allocatedStrings;

static ThreadLockObject debuglock;

static DEBUGHANDLER debughandler = NULL;
static void         *debughandler_context = NULL;

static DEBUGHANDLER errorhandler = NULL;
static void         *errorhandler_context = NULL;

const char *DoubleFormatHuman = "%0.32le";
const char *DoubleFormatExact = "#%016lx";

/*--------------------------------------------------------------------------------*/
/** Set debug handler (replacing printf())
 *
 * @param handler debug handler
 * @param context optional parameter to pass to handler
 *
 */
/*--------------------------------------------------------------------------------*/
void SetDebugHandler(DEBUGHANDLER handler, void *context)
{
  ThreadLock lock(debuglock);

  debughandler = handler;
  debughandler_context = context;
}

/*--------------------------------------------------------------------------------*/
/** Set error handler (replacing printf())
 *
 * @param handler error handler
 * @param context optional parameter to pass to handler
 *
 */
/*--------------------------------------------------------------------------------*/
void SetErrorHandler(DEBUGHANDLER handler, void *context)
{
  ThreadLock lock(debuglock);

  errorhandler = handler;
  errorhandler_context = context;
}

void debug_msg(const char *fmt, ...)
{
  va_list     ap;
  std::string str;

  va_start(ap, fmt);
  VPrintf(str, fmt, ap);
  va_end(ap);

  {
    ThreadLock lock(debuglock);
    if (debughandler)
    {
      (*debughandler)(str.c_str(), debughandler_context);
    }
    else
    {
      printf("%s\n", str.c_str());
      fflush(stdout);
    }

    FreeStrings();
  }
}

void debug_err(const char *fmt, ...)
{
  FILE *errstr = stdout;
  va_list     ap;
  std::string str;

  va_start(ap, fmt);
  VPrintf(str, fmt, ap);
  va_end(ap);

  {
    ThreadLock lock(debuglock);
    static bool _within = false;        // protect against recursive calls
    bool        within  = _within;

    _within = true;

    if (!within)
    {
      static EnhancedFile file(EnhancedFile::catpath(getenv("HOME"), "bbcat-errors.txt").c_str(), "w");

      file.fprintf("%s\n", str.c_str());
      file.fflush();
    }
    
    if (errorhandler)
    {
      (*errorhandler)(str.c_str(), errorhandler_context);
    }
    else
    {
      fprintf(errstr, "%s\n", str.c_str());
      fflush(errstr);
    }

    FreeStrings();

    _within = false;
  }
}

static bool pipe_enabled = false;
void enable_pipe(bool enable)
{
  pipe_enabled = enable;
}

bool is_pipe_enabled()
{
  return pipe_enabled;
}

void pipe_msg(const char *fmt, ...)
{
  if (pipe_enabled) {
    va_list     ap;
    std::string str;

    va_start(ap, fmt);
    VPrintf(str, fmt, ap);
    va_end(ap);

    ThreadLock lock(debuglock);
    static ulong_t tick0 = GetTickCount();
    ulong_t tick = GetTickCount() - tick0;

    printf("INFO[%010lu]: %s\n", tick, str.c_str());
    fflush(stdout);

    FreeStrings();
  }
}

bool get_pipe_msg(const char *str, ulong_t& tick, std::string& str2)
{
  bool ispipemsg = false;

  if (sscanf(str, "INFO[%lu]: ", &tick) > 0)
  {
    str2.assign(strstr(str, ": ") + 2);
    ispipemsg = true;
  }

  return ispipemsg;
}

const char *CreateString(const char *data, uint_t len)
{
  char *str;

  if ((str = new char[len + 1]) != NULL)
  {
    memcpy(str, data, len);
    str[len] = 0;

    allocatedStrings.push_back(str);
  }

  return str;
}

void FreeStrings()
{
  uint_t i;

  for (i = 0; i < allocatedStrings.size(); i++)
  {
    delete[] allocatedStrings[i];
  }

  allocatedStrings.clear();
}

ulong_t GetTickCount()
{
#ifdef __MACH__
  static mach_timebase_info_data_t timebase;
  static bool inited = false;

  if (!inited)
  {
    mach_timebase_info(&timebase);
    inited = true;
  }

  uint64_t tick = mach_absolute_time();
  tick = (tick * timebase.numer) / timebase.denom;

  return (ulong_t)(tick / 1000000);
#else
  struct timespec timespec;

#ifdef ANDROID
  clock_gettime(CLOCK_MONOTONIC_HR, &timespec);
#elif defined(__CYGWIN__)
  clock_gettime(CLOCK_MONOTONIC, &timespec);
#else
  clock_gettime(CLOCK_MONOTONIC_RAW, &timespec);
#endif

  return timespec.tv_sec * 1000UL + (timespec.tv_nsec / 1000000UL);
#endif
}

uint32_t IEEEExtendedToINT32u(const IEEEEXTENDED *num)
{
  /* Format of 80-bit IEEE floating point number is:
   *
   * sign(1).exponent(15).mantissa(64)
   *
   * NOTE: mantissa has explicit 1
   */

  const uint8_t *p = num->b;
  int16_t  expo = (int16_t)(((uint16_t)(p[0] & 0x7f) << 8) | (uint16_t)p[1]) - 16383;
  uint64_t mant;
  uint32_t val;

  /* generate mantissa... */
  memcpy(&mant, p + 2, sizeof(mant));
  /* ...and swap bytes if necessary */
  if (!MACHINE_IS_BIG_ENDIAN) BYTESWAP_VAR(mant);

  /* mantissa has decimal point between bits 63 and 62
   * whereas we want it between bits 32 and 31 so we need
   * to bias expo by 31 bits */
  expo -= 31;

  /* shift mantissa appropriately */
  if      (expo < 0) mant >>= -expo;
  else if (expo > 0) mant <<=  expo;

  /* round 64-bit mantissa (which is now 32.32) and return integer part */
  val = (uint32_t)((mant + 0x80000000) >> 32);

  return val;
}

/*------------------------------------------------------------
  Function: Convert uint32_t to IEEE Extended
  ----------------------------------------------------------*/
void INT32uToIEEEExtended(uint32_t val, IEEEEXTENDED *num)
{
  int16_t expo = 0;
  int64_t mant = 0;

  memset(num, 0, sizeof(*num));

  while (!(val & 0x80000000))
  {
    val <<= 1;
    expo--;
  }

  mant  = (uint64_t)val << 32;
  expo += 31 + 16383;

  if (!MACHINE_IS_BIG_ENDIAN)
  {
    BYTESWAP_VAR(expo);
    BYTESWAP_VAR(mant);
  }

  memcpy(num->b, &expo, sizeof(expo));
  num->b[0] &= 0x7f;
  memcpy(num->b + 2, &mant, sizeof(mant));
}

std::string CreateIndent(const std::string& indent, uint_t count)
{
  uint_t len = indent.size();
  std::string str;
  char *buf;

  if ((count * len) > 0)
  {
    if ((buf = new char[count * len]) != NULL)
    {
      uint_t pos = 0, endpos = count * len;

      memcpy(buf, indent.c_str(), len);

      for (pos = len; pos < endpos;)
      {
        uint_t n = MIN(endpos - pos, pos);

        memcpy(buf + pos, buf, n);
        pos += n;
      }

      str.assign(buf, pos);
    }
  }

  return str;
}

/*--------------------------------------------------------------------------------*/
/** Printf for std::string
 *
 * @param str string to be added to
 * @param fmt printf-style format information
 *
 */
/*--------------------------------------------------------------------------------*/
void Printf(std::string& str, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  VPrintf(str, fmt, ap);
  va_end(ap);
}

/*--------------------------------------------------------------------------------*/
/** vprintf for std::string
 *
 * @param str string to be added to
 * @param fmt printf-style format information
 * @param ap ap_list of arguments
 *
 */
/*--------------------------------------------------------------------------------*/
void VPrintf(std::string& str, const char *fmt, va_list ap)
{
  char *buf = NULL;
  if (vasprintf(&buf, fmt, ap) > 0)
  {
    str += buf;
    free(buf);
  }
}


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
uint_t SplitString(const std::string& str, std::vector<std::string>& list, char delim, uint_t maxstrings)
{
  uint_t p = 0, l = str.length();

  while ((p < l) && (!maxstrings || (list.size() < maxstrings)))
  {
    // ignore whitespace before string
    while ((p < l) && ((str[p] == ' ') || (str[p] == '\t'))) p++;

    // detect opening quote
    char quote = ((str[p] == '\'') || (str[p] == '\"')) ? str[p] : 0;
    
    // skip quote, if any
    if (quote) p++;

    // save string start
    uint_t p1 = p;

    // advance until either end of string, delimiter is found or closing quote is found
    while ((p < l) && ((!quote && (str[p] != delim)) || (quote && (str[p] != quote)))) p++;

    // save string end
    uint_t p2 = p;

    // if quotes were *not* used, move string end back over any whitespace
    if (!quote)
    {
      while ((p2 > p1) && ((str[p2 - 1] == ' ') || (str[p2 - 1] == '\t'))) p2--;
    }

    // if string is not empty, add it to the list
    if (p2 > p1) list.push_back(str.substr(p1, p2 - p1));

    // if a closing quote was found, skip quote and then find delimiter
    if ((p < l) && quote && (str[p] == quote))
    {
      p++;

      while ((p < l) && (str[p] != delim)) p++;
    }

    // skip over delimiters
    while ((p < l) && (str[p] == delim)) p++;

    // ignore whitespace at end
    while ((p < l) && ((str[p] == ' ') || (str[p] == '\t'))) p++;
  }

  return p;
}

/*--------------------------------------------------------------------------------*/
/** Interpolate current towards target at rate coeff, protecting against denormals
 */
/*--------------------------------------------------------------------------------*/
void Interpolate(double& current, double target, double coeff, double limit)
{
  current += (target - current) * coeff;
  if (fabs(target - current) < limit) current = target;
}

const char *StringStream::eol = "\n";

StringStream::StringStream()
{
  clear();
}

StringStream::~StringStream()
{
}

StringStream& StringStream::operator << (const std::string& str)
{
  data += str;
  return *this;
}

StringStream& StringStream::operator << (const char *str)
{
  Printf(data, "%s", str);
  return *this;
}

StringStream& StringStream::operator << (sint_t n)
{
  Printf(data, "%d", n);
  return *this;
}

StringStream& StringStream::operator << (uint_t n)
{
  Printf(data, "%u", n);
  return *this;
}

StringStream& StringStream::operator << (slong_t n)
{
  Printf(data, "%ld", n);
  return *this;
}

StringStream& StringStream::operator << (ulong_t n)
{
  Printf(data, "%lu", n);
  return *this;
}

StringStream& StringStream::operator << (sllong_t n)
{
  Printf(data, "%lld", n);
  return *this;
}

StringStream& StringStream::operator << (ullong_t n)
{
  Printf(data, "%llu", n);
  return *this;
}

const char *StringStream::get() const
{
  return data.c_str();
}

void StringStream::clear()
{
  data = "";
}

void debug_msg(StringStream& str)
{
  ThreadLock lock(debuglock);
  const char *p = str.get();
  if (debughandler)
  {
    (*debughandler)(p, debughandler_context);
  }
  else
  {
    printf("%s\n", p);
    fflush(stdout);
  }

  FreeStrings();
  str.clear();
}

void debug_err(StringStream& str)
{
  ThreadLock lock(debuglock);
  const char *p = str.get();
  if (errorhandler)
  {
    (*errorhandler)(p, errorhandler_context);
  }
  else
  {
    printf("%s\n", p);
    fflush(stdout);
  }

  FreeStrings();
  str.clear();
}

/*--------------------------------------------------------------------------------*/
/** Prevent value becoming denormalized (bad for performance!)
 */
/*--------------------------------------------------------------------------------*/
float fix_denormal(float val)
{
  volatile float res = val;     // volatile here means 'don't optimize me!'

  // by adding and subtracting a small value, if the value is significantly less than the small value
  // the result will become zero instead of being a denormalized number
  res += 1.0e-31;
  res -= 1.0e-31;

  return res;
}

double fix_denormal(double val)
{
  volatile double res = val;    // volatile here means 'don't optimize me!'

  // by adding and subtracting a small value, if the value is significantly less than the small value
  // the result will become zero instead of being a denormalized number
  res += 1.0e-291;
  res -= 1.0e-291;

  return res;
}

/*--------------------------------------------------------------------------------*/
/** Add a list of parameters/controls to a list
 */
/*--------------------------------------------------------------------------------*/
void AddParametersToList(const PARAMETERDESC *parameters, uint_t n, std::vector<const PARAMETERDESC *>& list)
{
  uint_t i;

  for (i = 0; i < n; i++) list.push_back(parameters + i);
}

static const double mindb = -120.0;

/*--------------------------------------------------------------------------------*/
/** Convert dB to gain assuming <=-120 is cut
 */
/*--------------------------------------------------------------------------------*/
double dBToGain(double db)
{
  // treat -120 as cut
  if (db > mindb) return pow(10.0, .05 * db);
  return 0.0;
}

/*--------------------------------------------------------------------------------*/
/** Convert gain to dB to gain (-120 is the minimum, including cut)
 */
/*--------------------------------------------------------------------------------*/
double GainTodB(double gain)
{
  static const double mingain = pow(10.0, .05 * mindb);  // minimum value
  double db = mindb;

  gain = fabs(gain);
  if (gain >= mingain)
  {
    db = 20.0 * log10(gain);
    db = MAX(db, mindb);
  }

  return db;
}

/*--------------------------------------------------------------------------------*/
/** Attempt to evaluate values from strings
 */
/*--------------------------------------------------------------------------------*/
bool Evaluate(const std::string& str, bool& val)
{
  bool success = false;
  uint_t n;

  if (sscanf(str.c_str(), "%u", &n) > 0)
  {
    val = (n != 0);
    success = true;
  }
  else if (str == "true")
  {
    val = true;
    success = true;
  }
  else if (str == "false")
  {
    val = false;
    success = true;
  }

  return success;
}

bool Evaluate(const std::string& str, sint_t& val)
{
  return (sscanf(str.c_str(), "%d", &val) > 0);
}

bool Evaluate(const std::string& str, uint_t& val)
{
  return (sscanf(str.c_str(), "%u", &val) > 0);
}

bool Evaluate(const std::string& str, slong_t& val)
{
  return (sscanf(str.c_str(), "%ld", &val) > 0);
}

bool Evaluate(const std::string& str, ulong_t& val)
{
  return (sscanf(str.c_str(), "%lu", &val) > 0);
}

bool Evaluate(const std::string& str, double& val)
{
  // this will FAIL to compile as 32-bit code

  // values starting with a '#' are a 64-bit hex representation of the double value
  // otherwise try to scan the string as a double
  return (((str[0] == '#') && (sscanf(str.c_str() + 1, "%lx", (ulong_t *)&val) > 0)) ||
          ((str[0] != '#') && (sscanf(str.c_str(),     "%lf", &val) > 0)));
}

std::string StringFrom(bool val)
{
  std::string str;
  Printf(str, "%u", val ? 1 : 0);
  return str;
}

std::string StringFrom(sint_t val, const char *fmt)
{
  std::string str;
  Printf(str, fmt, val);
  return str;
}

std::string StringFrom(uint_t val, const char *fmt)
{
  std::string str;
  Printf(str, fmt, val);
  return str;
}

std::string StringFrom(slong_t val, const char *fmt)
{
  std::string str;
  Printf(str, fmt, val);
  return str;
}

std::string StringFrom(ulong_t val, const char *fmt)
{
  std::string str;
  Printf(str, fmt, val);
  return str;
}

std::string StringFrom(double val, const char *fmt)
{
  std::string str;
  // this will FAIL to compile as 32-bit code
  // print double as hex encoded double
  Printf(str, fmt, val);
  return str;
}

/*--------------------------------------------------------------------------------*/
/** Bog-standard string search and replace that *should* be in std::string!
 */
/*--------------------------------------------------------------------------------*/
std::string SearchAndReplace(const std::string& str, const std::string& search, const std::string& replace)
{
  std::string res = str;
  size_t p = 0;

  // make sure search doesn't end up in an endless loop by
  // gradually moving the search start through the string
  while ((p = res.find(search, p)) < std::string::npos)
  {
    res = res.substr(0, p) + replace + res.substr(p + search.length());
    p  += replace.length();     // start searching just after replacement
  }

  return res;
}

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
bool matchstring(const char *pat, const char *str)
{
  if      (!pat[0])       return !str[0];
  else if (!str[0])       return !pat[0];
  else if (pat[0] == '*') return (matchstring(pat, str + 1) || matchstring(pat + 1, str + 1));
  else if (pat[0] == '?') return matchstring(pat + 1, str + 1);

  return ((pat[0] == str[0]) && matchstring(pat + 1, str + 1));
}

BBC_AUDIOTOOLBOX_END
