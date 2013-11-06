
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


#include <stdio.h>
#include <sockhop/SHStringMatcher.h>
#include "shRegex.h"

// Set to 1 to use pattern matching, 0 to just use strcmp()
#define USE_REGEXP 1

// If we're not compiling in string matching, disable all calls to regexp!
#if USE_REGEXP == 0
#define regfree(x)
#define regexec(RegExp, String, x, y, z) (((SimplePattern)&&(strcmp(String, SimplePattern) == 0)) ? REG_NOERROR : REG_NOMATCH)
#define regerror(RegExpStat, RegExp, Buf, BufSize)
#define regcomp( RegExp, Pattern, CaseSensitivity ) 0
#endif

#include <string.h>


SHStringMatcher::SHStringMatcher()
{
    _pattern = NULL;
    _simplePattern = NULL;
    _regExp = NULL;
    _regExpStat = 0;
    _caseSensitivity = true;

#if USE_REGEXP == 0
    printf("Warning, regular expression code not compiled in!  (using strcmp())\n");
#endif
} 

SHStringMatcher::~SHStringMatcher()
{
    if (_regExp != NULL) regfree(_regExp);
    delete _regExp;
    delete [] _pattern;
    delete [] _simplePattern;
}

void SHStringMatcher::SetCaseSensitivity(bool Sensitivity )
{
    _caseSensitivity = Sensitivity;
    if ( _simplePattern != NULL ) {
        SetRegularExpression( _simplePattern, false );
    } else if ( _pattern != NULL ) {
        char * string = new char [ strlen( _pattern ) + 1 ];
        strcpy( string, _pattern );
        SetRegularExpression( string );
        delete [] string;
    }
} 

bool SHStringMatcher::SetRegularExpression(const char * const String, bool DeleteSimple)
{
    if ( _regExp != NULL ) {
        regfree( _regExp );
        delete _regExp;
        _regExp = NULL;
    }
    delete [] _pattern;
    if (DeleteSimple == true) {
        delete [] _simplePattern;
        _simplePattern = NULL;
    }

    _pattern = new char [ strlen( String ) + 1 ];
    strcpy( _pattern, String );

    _regExp = new regex_t;
    _regExpStat = regcomp( _regExp, _pattern,_caseSensitivity ? 0 : REG_ICASE );
    if ( _regExpStat != 0 ) {
        regfree( _regExp );
        delete _regExp;
        _regExp = NULL;
        return false;
    }
    return true;
}


const char * const SHStringMatcher::GetRegularExpression() const
{
    return _pattern;
}


bool
SHStringMatcher::SetSimpleExpression(const char * const String) 
{
    delete _simplePattern;

    _simplePattern = new char [ strlen( String ) + 1 ];
    strcpy( _simplePattern, String );

    char * ptr = _simplePattern;
    char * patptr;
    char * pattern;

    int duals = 0;
    while ( *ptr != '\0' ) {
        switch ( *ptr ) {
        case ',':  // "\|"
        case '*':  // ".*"
        case '\\': // "\\"
        case '[':  // "\["
        case ']':  // "\]"
        case '.':  // "\."
            duals++;
            break;
        default:
            break;
        }
        ptr++;
    }

    int total = strlen( String ) + duals + 6; // real regexp size + init/exit
    pattern = new char [ total + 1 ];
    strcpy( pattern, "^\\(" );

    ptr = _simplePattern;
    patptr = pattern + 3;

    while ( *ptr != '\0' ) {
        switch ( *ptr ) {
//      translate
        case ',': *patptr++ = '\\';
                  *patptr = '|';
                  break;
        case '*': *patptr++ = '.';
                  *patptr = '*';
                  break;
        case '?': *patptr = '.';
                  break;
//      just quote, except when quoting the above specials
        case '\\': if ( ptr[1] == ',' ) {
                       ptr++;
                       *patptr = ',';
                       break;
                   } else if ( ptr[1] == '*' ) {
                       ptr++;
                       *patptr++ = '\\';
                       *patptr = '*';
                       break;
                   } else if ( ptr[1] == '?' ) {
                       ptr++;
                       *patptr++ = '\\';
                       *patptr = '?';
                       break;
                   }
        case '[':
        case ']':
        case '.':
                  *patptr++ = '\\';
                  *patptr = *ptr;
                  break;
//      copy verbatim
        default:  *patptr = *ptr;
                  break;
        }
        patptr++;
        ptr++;
    }
    *patptr = '\0';
    strcat( pattern, "\\)$" );

//    fprintf( stdout, "OUTPUT: pattern became '%s'.\n", pattern );
    bool retval = SetRegularExpression( pattern, false );
    delete [] pattern;
    return retval;
} 


const char * const SHStringMatcher::GetSimpleExpression() const
{
    return _simplePattern;
}


bool SHStringMatcher::Match(const char * const String)
{
    if (_regExp == NULL) return false;
    _regExpStat = regexec(_regExp, String, 0, NULL, 0);
    return (_regExpStat != REG_NOMATCH);
} 


void SHStringMatcher::GetErrorMessage(char * const Buf, const int BufSize) const
{
    if ( _pattern == NULL )
        strncpy( Buf, "couldn't allocate pattern", BufSize - 1 );
    else
        regerror( _regExpStat, _regExp, Buf, BufSize );
    Buf[ BufSize ] = 0;
} 

