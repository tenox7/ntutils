//
// General Logging... IN A CLASS!  :-)
//

/********************************************************************

The author, Darin McBride, explicitly places this module under the
LGPL license.  This module must remain free for use, but will not
cause software that uses it to be required to be under the GPL or
any of its derivitives.

********************************************************************/

/**

Class-based, OO-based logging

This is intended to be used as a trace utility in C++, if you follow the
following conventions.  Note that all macros are intended to work like
function calls - so you still need the terminating semicolon.

At the top of each function, you must use STARTFUNC(x).  The parameter to
this macro is the name of the function.  For example, STARTFUNC("main").

At the end of each function, or wherever you return from, use one of:

ENDFUNCRC - trace the return code
ENDFUNCRC_SAFE - same as above, but can be used when returning the
                 value of something with side effects
ENDFUNCAS - trace the return code, but pretend it's a different type
ENDFUNCAS_SAFE - trace the return code of something with side effects,
                 as if it were another type.

Finally, to log trace points throughout your code, use the LOG() macro
as if it were an ostream.  To terminate each line, do not use endl, but
use the ENDLINE macro.  Yes, currently it's the same thing, but I'm
reserving the right to change that in the future.

For example:

int main()
{
    STARTFUNC("main");

    LOG << "About to call foo" << ENDLINE;
    SomeObjectType baz;
    foo(baz);

    ENDFUNCRC(0);
    // no return - the macro does this for us.
}

void foo(SomeObjectType bar)
{
    STARTFUNC("foo")
    // assumes bar has some meaningful way to be put into an ostream:
    LOG << "bar = " << bar << ENDLINE;
    // as void, no need to endfunc.
}

ENDFUNCRC_SAFE is used such as:

ENDFUNCRC_SAFE(foo++); // side effect only happens once.
// was: return foo++

The AS versions are only used to log as a different type than it currently
is.  For example, to log a HANDLE as if it were an unsigned long:

HANDLE foo;
ENDFUNCAS(unsigned long, foo);
// was: return foo

Finally, ENDFUNCAS_SAFE:

ENDFUNCAS_SAFE(HANDLE, unsigned long, GetNextHandle());
// was: return GetNextHandle()

*/


#ifndef LOGGING_HPP
#define LOGGING_HPP

#if defined(NO_NEW_CPP_FEATURES)
#include <fstream.h>
#include <assert.h>
#else
#include <fstream>
#include <cassert>
#endif

#ifndef FTE_NO_LOGGING

#if !defined(NO_NEW_CPP_FEATURES)
using namespace std;
#endif


/**
 * GlobalLog handles the actual logging.
 */
class GlobalLog
{
    friend class FunctionLog;
private:
    char*	m_strLogFile;
    ofstream    m_ofsLog;

    bool        m_bOpened;

    int         indent;

    ostream&    operator()();

public:
    GlobalLog() : m_strLogFile(NULL), m_bOpened(false) {}
    GlobalLog(char const* strLogFile);

    virtual ~GlobalLog();
    void SetLogFile(char const* strNewLogFile);
    operator bool() { return !m_ofsLog.fail(); }

protected:
    bool OpenLogFile(); // actually open it
};

extern GlobalLog globalLog;

/**
 * FunctionLog is the local object that handles logging inside a function.
 * All work is put through here.
 */
class FunctionLog
{
private:
    GlobalLog&  log;
    char const* func;
    int         myIndentLevel;
    char        indentChar;
public:
    // Enter:
    FunctionLog(GlobalLog& gl, const char* funcName, unsigned long line);

    // Exit:
    ~FunctionLog();

    // RC?
    ostream& RC(unsigned long line);

private:
    ostream& OutputLine()
    { return OutputIndent(log()) << '[' << func << "] "; }

public:
    // output line.
    ostream& OutputLine(unsigned long line)
    { return OutputLine() << '{' << line << "} "; }

private:
    ostream& OutputIndent(ostream& os);
};

#define LOGOBJNAME functionLog__obj
#define LOG LOGOBJNAME.OutputLine(__LINE__)
#define ENDLINE endl

#define STARTFUNC(func) FunctionLog LOGOBJNAME(globalLog, func, __LINE__)
#define ENDFUNCRC(rc) do { LOGOBJNAME.RC(__LINE__) << (rc) << ENDLINE; return (rc); } while (0)
#define ENDFUNCRC_SAFE(type,rc) do { type LOG__RC = (rc); LOGOBJNAME.RC(__LINE__) << LOG__RC << ENDLINE; return LOG__RC; } while (0)
#define ENDFUNCAS(type,rc) do { LOGOBJNAME.RC(__LINE__) << (type)(rc) << ENDLINE; return (rc); } while (0)
#define ENDFUNCAS_SAFE(logtype,rctype,rc) do { rctype LOG__RC = (rc); LOGOBJNAME.RC(__LINE__) << (logtype)LOG__RC << ENDLINE; return LOG__RC; } while (0)
#define BOOLYESNO(x) ((x) ? "YES" : "NO")

/********************************************************************/

/* Utility ostream functions.

These are functions that only have anything to do with logging because
logging is the only place we use IO streams.  They're here to make tracing
easier to capture more information.

**********************************************************************
FUNCTION
    BinChar(char)

CLASS
    IO Manipulator

SAMPLE
    LOG << "this char is: " << BinChar(CharP) << ENDLINE;

DESCRIPTION
    BinChar will insert the character followed by the hex value in
    brackets.  If the character is non-printable, it will display as a
    dot.
**********************************************************************
FUNCTION
    FillChar(char, length)

CLASS
    IO Manipulator

SAMPLE
    LOG << FillChar('*', 50) << ENDLINE;

DESCRIPTION
    FillChar is available for pretty-printing.  Use sparingly, if at
    all.
**********************************************************************
FUNCTION
    LOGBINARYDATA(char*, int len)

CLASS
    Macro

SAMPLE
    LOG << "The value of some_binary_data is:" << ENDLINE
    LOGBINARYDATA(some_binary_data, int len);

DESCRIPTION
    This macro will log some structure or array in basically a
    memory-dump style: 8 characters, followed by 8 hex values.  If
    any character is non-printable, it will display as a dot.
**********************************************************************

*/
#define DECLARE_OSTREAM_FUNC1(type1) \
    class ostream_func1_##type1 { \
    private: \
        ostream& (*osfunc)(ostream&, type1 const&); \
        type1 const& o1; \
    public: \
        ostream_func1_##type1(ostream& (*osfunc_)(ostream&, type1 const&), type1 const& o1_) : osfunc(osfunc_), o1(o1_) {} \
        ostream& operator()(ostream& os) const { return osfunc(os, o1); } \
    }; \
    inline ostream& operator <<(ostream& os, ostream_func1_##type1 const& ofunc) \
    { return ofunc(os); }

#define DECLARE_OSTREAM_FUNC2(type1,type2) \
    class ostream_func2_##type1##_##type2 { \
    private: \
        ostream& (*osfunc)(ostream&, type1 const&, type2 const&); \
        type1 const& o1; \
        type2 const& o2; \
    public: \
        ostream_func2_##type1##_##type2(ostream& (*osfunc_)(ostream&, type1 const&, type2 const&), \
                                        type1 const& o1_, type2 const& o2_) : \
                                            osfunc(osfunc_), o1(o1_), o2(o2_) {} \
        ostream& operator()(ostream& os) const { return osfunc(os, o1, o2); } \
    }; \
    inline ostream& operator <<(ostream& os, ostream_func2_##type1##_##type2 const& ofunc) \
    { return ofunc(os); }

DECLARE_OSTREAM_FUNC1(char);
DECLARE_OSTREAM_FUNC2(char, size_t);

ostream& Log__osBinChar(ostream&, char const&);
inline ostream_func1_char BinChar(char c)
{ return ostream_func1_char(Log__osBinChar, c); }

ostream& Log__osFillChar(ostream&, char const&, size_t const&);
inline ostream_func2_char_size_t FillChar(char const& c, size_t const& num)
{ return ostream_func2_char_size_t(Log__osFillChar, c, num); }


void Log__BinaryData(FunctionLog&, void* bin_data, size_t len, unsigned long line);
#define LOGBINARYDATA(bin_data,len) Log__BinaryData(LOGOBJNAME,bin_data,len, __LINE__)

#else // defined FTE_NO_LOGGING

#include <iostream>

#define LOG while (0) { std::cout
#define ENDLINE std::endl; }

#define STARTFUNC(func) 
#define ENDFUNCRC(rc) return rc
#define ENDFUNCRC_SAFE(type,rc) return rc
#define ENDFUNCAS(type,rc) return rc
#define ENDFUNCAS_SAFE(logtype,rctype,rc) return rc
#define BOOLYESNO(x) ((x) ? "YES" : "NO")

//Replacements for utility functions.
#define BinChar(c) c
#define LOGBINARYDATA(b,l)

#endif // FTE_NO_LOGGING

#endif // LOGGING_HPP
