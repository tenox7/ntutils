/********************************************************************

The author, Darin McBride, explicitly places this module under the
LGPL license.  This module must remain free for use, but will not
cause software that uses it to be required to be under the GPL or
any of its derivitives.

********************************************************************/

#include "log.h"

#ifndef FTE_NO_LOGGING

#include <time.h>
#include <ctype.h>
#include "sysdep.h"

#if defined(NO_NEW_CPP_FEATURES)
#include <iomanip.h>
#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#else
#include <iomanip>
#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace std;
#endif


/*********************************************************************
 *
 * Required variables:
 *
 *********************************************************************/

GlobalLog globalLog;

GlobalLog::GlobalLog(char const* strLogFile) : m_strLogFile(strdup(strLogFile)), m_bOpened(false)
{
}

bool GlobalLog::OpenLogFile()
{
    if (!m_bOpened && m_strLogFile != NULL)
    {
        m_ofsLog.open(m_strLogFile, ios::out /*| ios::text*/ | ios::app /*append*/);
        if (!m_ofsLog)
        {
            m_strLogFile = NULL;
            m_bOpened = false;
        }
        else
        {
            m_bOpened = true;
	    //no way with gcc3.0 m_ofsLog.setbuf(NULL, 0);
	}
    }
    return m_bOpened;
}

//
// Operator():
//

// useful variable for returning an invalid ofstream to kill off any
// output to the logfile with wrong loglevel.
static ofstream ofsInvalid;

GlobalLog::~GlobalLog()
{
    free(m_strLogFile);
}

ostream& GlobalLog::operator()()
{
    // Ensure the current file is open:
    if (!OpenLogFile()) // if it can't be opened, shortcut everything:
        return ofsInvalid;

    time_t tNow = time(NULL);
    struct tm* ptm = localtime(&tNow);

    char cOldFill = m_ofsLog.fill('0');
    m_ofsLog << setw(4) << ptm->tm_year + 1900 << '-'
        << setw(2) << ptm->tm_mon  << '-'
        << setw(2) << ptm->tm_mday << ' '
        << setw(2) << ptm->tm_hour << ':'
        << setw(2) << ptm->tm_min  << ':'
        << setw(2) << ptm->tm_sec  << ' '
        << "FTE" << ' ';
    m_ofsLog.fill(cOldFill);
    return m_ofsLog;
}

void GlobalLog::SetLogFile(char const* strNewLogFile)
{
    if (m_strLogFile == NULL ||
	strNewLogFile == NULL ||
	strcmp(m_strLogFile,strNewLogFile) != 0)
    {
	free((void*)m_strLogFile);
	m_strLogFile = strNewLogFile == NULL ? (char *)NULL : strdup(strNewLogFile);
	m_bOpened    = false;
    }
}

FunctionLog::FunctionLog(GlobalLog& gl, const char* funcName, unsigned long line)
    : log(gl), func(funcName), myIndentLevel(++log.indent), indentChar('+')
{
    OutputLine(line) << "Entered function" << ENDLINE;
}

FunctionLog::~FunctionLog()
{
    indentChar = '+';
    OutputLine() << "Exited function" << ENDLINE;
    --log.indent;
}

ostream& FunctionLog::RC(unsigned long line)
{
    indentChar = '!';
    return OutputLine() << "{" << line << "} Returning rc = ";
}

ostream& FunctionLog::OutputIndent(ostream& os)
{
    os << FillChar('|', myIndentLevel - 1);
    //for (int i = 1; i < myIndentLevel; ++i)
    //    os << '|';
    os << indentChar << ' ';
    indentChar = '|'; // reset it to |'s.
    return os;
}

ostream& Log__osBinChar(ostream& os, char const& c)
{
    char const cOldFill = os.fill('0');
    os << (isprint(c) ? c : '.') <<
        " [0x" << hex << (int)c << dec << "]";
    os.fill(cOldFill);
    return os;
}

ostream& Log__osFillChar(ostream& os, char const& c, size_t const& len)
{
    for (size_t i = 0; i < len; ++i)
        os << c;
    return os;
}

#define LINE_LENGTH 8
void Log__BinaryData(FunctionLog& LOGOBJNAME, void* bin_data, size_t len, unsigned long line)
{
    for (size_t i = 0; i < len; i += LINE_LENGTH)
    {
        ostream& os = LOGOBJNAME.OutputLine(line);
        size_t j;

        // as characters
        for (j = i; j < i + LINE_LENGTH; ++j)
        {
            if (j < len)
            {
                char const c = ((char*)bin_data)[i+j];
                os << (isprint(c) ? c : '.');
            }
            else
            {
                os << ' ';
            }
        }
        os << "  [";
        // as hex values
        char const cOldFill = os.fill('0');
        for (j = i; j < i + LINE_LENGTH; ++j)
        {
            if (j < len)
            {
                int const c = ((char*)bin_data)[i+j];
                if (j != i) os << ',';
                os << hex << setw(2) << c << dec;
            }
            else
            {
                os << "   ";
            }
        }
        os.fill(cOldFill);
        os << ']' << ENDLINE;
    }
}

#endif // FTE_NO_LOGGING
