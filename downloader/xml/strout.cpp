/*  FILE:   STROUT.C

    Copyright (c) 2004-2009 by Lone Wolf Development.  All rights reserved.

    This code is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place, Suite 330, Boston, MA 02111-1307 USA

    You can find more information about this project here:

    http://code.google.com/p/ddidownloader/

    This file includes:

    Implementation of generic string rendering with positional parameters.
*/


#include    "private.h"

#include    <stdarg.h>
#include    <stdio.h>


/*  define constants used below
*/
#define ESCAPE_CHARACTER    '%'


//Syntax of format string is "xyz %n abc", where '%n' identifies parameter index 'n'.
//Just like printf, use '%%' to output a literal '%'.
T_Glyph_Ptr     PUBLISH String_Printf(T_Glyph_Ptr target,T_Glyph_Ptr format,...)
{
    va_list     arglist;
    T_Int32U    i;
    T_Glyph_Ptr dest,params[MAX_STRING_PARAMS];
    T_Glyph     ch;

    x_Trap_Opt((target == NULL) || (format == NULL));

    /* get the parameter list as a series of strings
        NOTE! We have to retrieve them all now since they will be accessed in
                any random order within the format string.
    */
    va_start(arglist,format);
    for (i = 0; i < MAX_STRING_PARAMS; i++)
        params[i] = va_arg(arglist,T_Glyph_Ptr);

    /* process the format string and params to generate the proper output
    */
    for (dest = target; *format != '\0'; format++) {

        /* if this isn't an escape character, just copy the character across
        */
        if (*format != ESCAPE_CHARACTER) {
            *dest++ = *format;
            continue;
            }

        /* if this is dual escape character, output the literal escape
        */
        ch = *++format;
        if (ch == ESCAPE_CHARACTER) {
            *dest++ = ESCAPE_CHARACTER;
            continue;
            }

        /* if this is not a valid position index, just output the character
        */
        if (x_Trap_Opt((ch < '0') || (ch >= '0' + MAX_STRING_PARAMS))) {
            *dest++ = ch;
            continue;
            }

        /* get the designated parameter and copy it to the output
        */
        ch -= '0';
        strcpy(dest,params[ch]);
        dest += strlen(dest);
        }

    /* null-terminate our output and return it
    */
    *dest = '\0';
    return(target);
}


T_Glyph_Ptr     PUBLISH As_Any(T_Glyph_Ptr format,...)
{
    static  T_Glyph     l_buffer[MAX_STRING_PARAMS][500];
    static  T_Int32U    l_index = 0;

    va_list     arglist;
    T_Glyph_Ptr ptr;

    /* get the string to render into and cycle to the next one
    */
    ptr = l_buffer[l_index++];
    if (l_index >= MAX_STRING_PARAMS)
        l_index = 0;

    /* render into the buffer and return the result
    */
    va_start(arglist,format);
    vsprintf(ptr,format,arglist);
    va_end(arglist);
    return(ptr);
}
