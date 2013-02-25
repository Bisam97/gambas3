/***************************************************************************

  (c) 2012 Adrien Prokopowicz <prokopy@users.sourceforge.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#include "utils.h"
#include <cstdio>

wchar_t nextUTF8Char(const char *&data, size_t len)
{
        register unsigned char c = *data;
        if (c <= 0x7f){//first byte
            data++;
            return (wchar_t)c;
        }
        else if (c <= 0xdf && c >= 0xbf && 1 < len){//2byte sequence start
            register wchar_t c2 = (unsigned char)data[1];
            data += 2;
            return (((c & 0x1f) << 6) | (c2 & 0x3f));
        }
        else if (c <= 0xef  && c >= 0xbf && 2 < len){//3byte sequence start
            register wchar_t c2 = (unsigned char)data[1];
            register wchar_t c3 = (unsigned char)data[2];
            data += 3;
            return (((((c & 0x1f) << 6) | (c2 & 0x3f)) << 6)| (c3 & 0x3f));
            //w = c & 0x0f;
        }
        else if (c <= 0xf7  && c >= 0xbf && 3 < len){//4byte sequence start
            register wchar_t c2 = (unsigned char)data[1];
            register wchar_t c3 = (unsigned char)data[2];
            register wchar_t c4 = (unsigned char)data[3];
            data += 4;
            return (((((((c & 0x1f) << 6) | (c2 & 0x3f)) << 6)| (c3 & 0x3f)) << 6) | (c4 & 0x3f));
            //w = c & 0x07;
        }
        else
        {
           return CHAR_ERROR;
        }
}


/* http://www.w3.org/TR/REC-xml/#NT-NameStartChar

  NameStartChar	::=  ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] |
  [#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] |
  [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
  */

#define INTER(min, max) (car >= min && car <= max)
#define CAR(c) (car == c)

bool isNameStartChar(const wchar_t car)
{
    if(INTER('a', 'z')) return true;

    return CAR(':') || INTER('A', 'Z') || CAR('_') || 
            INTER(0xC0, 0xD6) || INTER(0xD8, 0xF6) || INTER(0xF8, 0x2FF) ||
            INTER(0x370, 0x37D) || INTER(0x37F, 0x1FFF) || INTER(0x200C, 0x200D) ||
            INTER(0x2070, 0x218F) || INTER(0x2C00, 0x2FEF) || INTER(0x3001, 0xD7FF) ||
            INTER(0xF900, 0xFDCF) || INTER(0xFDF0, 0xFFFD) || INTER(0x10000, 0xEFFFF);


}

/* http://www.w3.org/TR/REC-xml/#NT-NameChar

  NameChar ::= 	NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]

  */

bool isNameChar(const wchar_t car)
{
    if(INTER('a', 'z')) return true;
    if(isNameStartChar(car)) return true;
    return  CAR('-') || CAR('.') || INTER('0', '9') ||
            (car == 0xB7) || INTER(0x0300, 0x036F) || INTER(0x203F, 0x2040);
}
/* http://www.w3.org/TR/REC-xml/#NT-S

    S ::= (#x20 | #x9 | #xD | #xA)+

  */
bool isWhiteSpace(const wchar_t s)
{
    register const wchar_t car = s;

    return (car == 0x20) || (car == 0x9) || (car == 0xD) || (car == 0xA);
}

bool isWhiteSpace(const char s)
{
    register const char car = s;

    return (car == 0x20) || (car == 0x9) || (car == 0xD) || (car == 0xA);
}

const void* memchrs(const char *source, size_t lensource, const char *comp, size_t lencomp)
{
    const char *pos = source - 1;
    do
    {
        pos = (char*)(memchr((void*)(pos + 1), ((comp))[0], lensource));
        if(!pos) return 0;
        if(pos + lencomp > source + lensource) return 0;
        if(memcmp(pos, comp, lencomp) != 0) continue;
        return pos;

    }while(1);
}

const void* memrchrs(const void *source, size_t lensource, const void *comp, size_t lencomp)
{
    char *pos = (char*)source;
    do
    {
        pos = (char*)(memrchr((void*)pos, ((char*)(comp))[lencomp - 1], lensource));
        if(!pos) return 0;
        if(pos - lencomp < source) return 0;
        if(memcmp(pos - lencomp, comp, lencomp) != 0) continue;
        return pos;
    }while(1);
}

void Trim(const char *&str, size_t &len)
{
    while(isWhiteSpace(*str) && len)
    {
        ++str;
        --len;
    }
    
    if(!len) return;
    
    while(isWhiteSpace(*(str + len - 1)) && len > 0)
        --len;
}

void insertString(char *&src, size_t &lenSrc, const char *insert, size_t lenInsert, char *&posInsert)
{
    size_t iPosInsert = posInsert - src;
    lenSrc += lenInsert;
    src = (char*)realloc(src, lenSrc);
    posInsert = src + iPosInsert;
    memmove(posInsert + lenInsert, posInsert, lenSrc - lenInsert - iPosInsert);
    memcpy(posInsert, insert, lenInsert);
}


/************************************ Error Management ************************************/

XMLParseException::XMLParseException(const char *nerror, const char *data, const size_t lenData, const char *posFailed) throw()
    : exception(), near(0), error(0), lenError(0), lenNear(0), line(1), column(1)
{
    lenError = strlen(nerror) + 1;
    error = (char*) malloc(lenError);
    memcpy(error, nerror, lenError);
    
    //Parse error : (errorText) !\n Line 123456789 , Column 123456789 : \n (near)
    
    if(posFailed == 0)
    {
        errorWhat = (char*)malloc(17 + lenError);
        sprintf(errorWhat, "Parse error : %s !", error);
        errorWhat[16 + lenError] = 0;
        return;
    }
    else if(!data || !lenData)
    {
        errorWhat = (char*)malloc(37 + lenError);
        sprintf(errorWhat, "Parse error : %s !\n Position %zu", error, (size_t)posFailed);
        errorWhat[36 + lenError] = 0;
        return;
    }
    if(posFailed > data + lenData || posFailed < data) return;
    AnalyzeText(data, lenData, posFailed);


    errorWhat = (char*)malloc(61 + lenError + lenNear);
    memset(errorWhat, 0, 61 + lenError + lenNear);
    sprintf(errorWhat, "Parse error : %s !\n Line %zu , Column %zu : \n %s", error, line, column, near);
    errorWhat[60 + lenError + lenNear] = 0;
}

XMLParseException::XMLParseException(const char *nerror, size_t posFailed) throw()
    : exception(), near(0), error(0), lenError(0), lenNear(0), line(1), column(1)
{
    lenError = strlen(nerror) + 1;
    error = (char*) malloc(lenError);
    memcpy(error, nerror, lenError);

    //Parse error : (errorText) !\n Line 123456789 , Column 123456789 : \n (near)

    errorWhat = (char*)malloc(37 + lenError);
    sprintf(errorWhat, "Parse error : %s !\n Position %zu", error, posFailed);
    errorWhat[36 + lenError] = 0;
}

XMLParseException::~XMLParseException() throw()
{
    if(near) free(near);
    if(error) free(error);
}

void XMLParseException::AnalyzeText(const char *text, const size_t lenText, const char *posFailed) throw()
{
    for(const char *pos = text; pos < posFailed; ++pos)
    {
        ++column;
        if(*pos == SCHAR_N)
        {
            column = 1;
            ++line;
        }
        else if(*pos == SCHAR_R)
        {
            if(*(pos + 1) == SCHAR_N) ++pos;
            column = 1;
            ++line;
        }
    }
    
    lenNear = text + lenText <= posFailed + 20 ?  text + lenText - posFailed : 20;
    if(lenNear == 0) return;
    near = (char*)malloc(lenNear + 1);
    memcpy(near, posFailed, lenNear);
    near[lenNear] = 0;
}

const char* XMLParseException::what() const throw()
{
    return errorWhat;
}


