/*  FILE:   UNIQUEID.C

    Copyright (c) 1993-2009 by Lone Wolf Development.  All rights reserved.

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

    Manipulation of 10-character unique ids.

    The structure of a unique id is to embed 10 characters into an 8-byte value.
    This 8-byte integer can then be used readily and efficiently within code.
    Each character of the id can be a letter, a digit, or an underscore. No
    other characters are valid. Each character is then converted into a 6-bit
    value, and then 10 6-bit values are merged to form a single 60-bit encoded
    id.

    To ensure that the sorting order of unique ids is consistent with the sort
    order of the equivalent text ids, the characters are encoded into the id in
    a left-to-right sequence, with the first character occupying the leftmost
    6-bit mask within the id. If a text id has fewer than 10 characters, the
    unused characters are treated as zero values within the id.

    The top four bits of the unique id are unused. This allows a client to use
    those bits for a type tag if desired. The four bits must be masked in and
    out independently from these routines, since they are assumed to be zero in
    all cases herein.

    As a convenience for handling wildcard searches using unique ids, extra
    routines are provided to facilitate this. The '?' character is treated as a
    wildcard that indicates matching of all ids with for which all preceeding
    characters match. For example, the string "bl?" would match "black" and
    "blue", but not "beige". These routines utilize the upper four bits of the
    unique id in which to store the offset where the wildcard is located, since
    the '?' character is not handled within the id itself. When using wildcards,
    be certain to utilize the routines that properly handle them, else errors
    will occur with the non-wildcard routines.
*/


#include    "private.h"


/*  define constants used below
*/
#define BITS_PER_CHARACTER  6
#define CHARACTER_MASK      0x3f

#define MAX_STRING_PARAMS   5


/*  declare static variables used below
*/
static  T_Int32U        l_mapping[256];
static  T_Glyph         l_reverse[64];


void    UniqueId_Initialize(void)
{
    T_Int8U     i;

    memset(l_mapping,0xff,sizeof(l_mapping));
    l_reverse[0] = '\0';
    for (i = 'A'; i <= 'Z'; i++) {
        l_mapping[i] = i - 'A' + 1;
        l_reverse[i - 'A' + 1] = i;
        }
    for (i = 'a'; i <= 'z'; i++) {
        l_mapping[i] = i - 'a' + 27;
        l_reverse[i - 'a' + 27] = i;
        }
    for (i = '0'; i <= '9'; i++) {
        l_mapping[i] = i - '0' + 53;
        l_reverse[i - '0' + 53] = i;
        }
    l_mapping['_'] = 63;
    l_reverse[63] = '_';
}


/* ***************************************************************************
    name

    .

    return      <-- ...
**************************************************************************** */

T_Boolean   UniqueId_Is_Valid_Char(T_Glyph ch)
{
    return(l_mapping[ch] <= CHARACTER_MASK);
}


T_Boolean   UniqueId_Is_Valid(const T_Glyph * text)
{
    T_Int32U    i;

    x_Trap_Opt(text == NULL);

    if (*text == '\0')
        return(FALSE);
    for (i = 0; (i < UNIQUE_ID_LENGTH) && (text[i] != '\0'); i++)
        if (l_mapping[text[i]] > CHARACTER_MASK)
            return(FALSE);
    if (text[i] != '\0')
        return(FALSE);
    return(TRUE);
}


T_Unique    UniqueId_From_Text(const T_Glyph * text)
{
    T_Unique    unique,value;
    T_Int32U    i;

    x_Trap_Opt(text == NULL);
    x_Trap_Opt(strlen(text) > UNIQUE_ID_LENGTH);
    for (i = 0, unique = 0; (i < UNIQUE_ID_LENGTH) && (text[i] != '\0'); i++) {
        if (x_Trap_Opt(l_mapping[text[i]] > CHARACTER_MASK))
            return((T_Unique) 0);
        value = l_mapping[text[i]];
        unique <<= BITS_PER_CHARACTER;
        unique |= value;
        }
    for ( ; i < UNIQUE_ID_LENGTH; i++)
        unique <<= BITS_PER_CHARACTER;
    return(unique);
}


T_Glyph_Ptr UniqueId_To_Text(T_Unique unique,const T_Glyph * text)
{
    static  T_Glyph     l_buffer[MAX_STRING_PARAMS][UNIQUE_ID_LENGTH + 1];
    static  T_Int32U    l_index = 0;

    T_Unique    value;
    T_Glyph_Ptr ptr,retval;
    T_Int32U    i;

    /* if the destination is NULL, render into it; otherwise, get the next
        string to render into and cycle to the next one
    */
    if (text != NULL)
        ptr = (T_Glyph_Ptr) text;
    else {
        ptr = l_buffer[l_index++];
        if (l_index >= MAX_STRING_PARAMS)
            l_index = 0;
        }

    /* handle the special case of an empty unique id
    */
    if (unique == 0) {
//        strcpy(ptr,"<empty>");
        *ptr = '\0';
        return(ptr);
        }

    /* render the unique id as text in the target buffer and return it
    */
    retval = ptr;
    ptr += UNIQUE_ID_LENGTH;
    *ptr-- = '\0';
    for (i = 0; i < UNIQUE_ID_LENGTH; i++) {
        value = unique & CHARACTER_MASK;
        *ptr-- = l_reverse[value];
        unique >>= BITS_PER_CHARACTER;
        }
    return(retval);
}
