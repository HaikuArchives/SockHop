
/**************************************************************************
SockHop (libsockhop.so):  Distributed network programming system for BeOS
Copyright (C) 1999 by Jeremy Friesner (jaf@chem.ucsd.edu)

This library is free software; you can redistribute it and/or 
modify it under the terms of the GNU Library General Public 
License as published by the Free Software Foundation; either 
version 2 of the License, or (at your option) any later version. 

This library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
Library General Public License for more details. 

You should have received a copy of the GNU Library General Public 
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
Boston, MA  02111-1307, USA. 
**************************************************************************/


#ifndef SHStringMatcher_h
#define SHStringMatcher_h

#include <sys/types.h>

////////////////////////////////////////////////////////////////////////////
//
// NOTE:  This class is essentially the same as the psStringMatcher v1.3 class 
//        developed by Lars Jørgen Aas <larsa@tihlde.hist.no> for the
//        Prodigal Software File Requester.  Used by permission.
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// SHStringMatcher
//
// A utility class for doing regular expression matching.  Used by the
// SHWildPathSorter class, but also exposed here so that all SockHop 
// applications can use it if they want
//
////////////////////////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHStringMatcher;
#else
#pragma export on
#endif 

struct re_pattern_buffer;
typedef struct re_pattern_buffer regex_t;

class SHStringMatcher 
{
public:
    SHStringMatcher();
    // Defaults to case sensitive matching
    
    ~SHStringMatcher();

    void SetCaseSensitivity(bool caseSensitive = true);

    bool SetSimpleExpression(const char * const expression);
    // This is generally the method to use.  Call it once to set your
    // pattern (e.g. "Foo*"), then call Match() on each string you
    // want to test against the pattern.
    
    const char * const GetSimpleExpression() const;

    bool SetRegularExpression(const char * const expression, bool deleteSimpleExpression = true);
    // For formal regular expressions.
    
    const char * const GetRegularExpression() const;

    bool Match(const char * const string);
    // Returns true iff (string) is matched by the current expression.
    
    void GetErrorMessage(char * const buf, const int bufSize) const;

protected:
    char * _pattern;
    char * _simplePattern;

    regex_t * _regExp;
    int _regExpStat;

    bool _caseSensitivity;
}; 

#ifndef __INTEL__
#pragma export reset
#endif

#endif