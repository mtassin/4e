/*  FILE:   XMLHELP.CPP

    Copyright (c) 2000-2012 by Lone Wolf Development, Inc.  All rights reserved.

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

    XML helper functions.
*/


#include    "private.h"


T_Glyph_Ptr XML_Safe(T_Glyph_Ptr dest,T_Glyph_Ptr text)
{
    T_Glyph_Ptr start;

    for (start = dest; *text != '\0'; text++)
        switch (*text) {
            case '<' :  strcpy(dest,"&lt;");    dest += 4;  break;
            case '>' :  strcpy(dest,"&gt;");    dest += 4;  break;
            case '&' :  strcpy(dest,"&amp;");   dest += 5;  break;
            case '\"' : strcpy(dest,"&quot;");  dest += 6;  break;
            case '\'' : strcpy(dest,"&apos;");  dest += 6;  break;
            default :
                *dest++ = *text;
                break;
            }
    *dest = '\0';
    return(start);
}


/* ***************************************************************************
    XML_Read_Text_Attribute

    Retrieve the contents of the named attribute from the given node, placing
    the results in the buffer and returning a pointer to that buffer. The caller
    must ensure that the buffer is large enough to contain the attribute value.

    xml         --> node to extract the attribute from
    name        --> name of the attribute
    buffer      --> buffer into which the attribute contents are written
    return      <-- pointer to buffer containing attribute contents
**************************************************************************** */

T_Glyph *   XML_Read_Text_Attribute(T_XML_Node xml,T_Glyph * name,T_Glyph * buffer)
{
    long    result;

    result = XML_Get_Attribute(xml,name,buffer);
    if (result != 0)
        throw(-1);
    return(buffer);
}


/* ***************************************************************************
    XML_Read_Boolean_Attribute

    Retrieve the contents of the named attribute from the given node, treating
    it as a boolean value of either "yes" or "no", and converting it to a
    true/false return value.

    xml         --> node to extract the attribute from
    name        --> name of the attribute
    return      <-- whether attribute is yes/true or no/false
**************************************************************************** */

bool        XML_Read_Boolean_Attribute(T_XML_Node xml,T_Glyph * name)
{
    T_Glyph     buffer[MAX_XML_BUFFER_SIZE+1];

    XML_Read_Text_Attribute(xml,name,buffer);
    if (strcmp(buffer,"yes") == 0)
        return(true);
    if (strcmp(buffer,"no") == 0)
        return(false);
    String_Printf(buffer,x_Internal_String(STRING_ATTRIBUTE_YES_NO),name);
    XML_Set_Error(buffer);
    throw(-1);
    return(false);  // to shut the compiler up
}


/* ***************************************************************************
    XML_Read_Integer_Attribute

    Retrieve the contents of the named attribute from the given node, treating
    it as an integer value, and converting it to an integer return value.

    xml         --> node to extract the attribute from
    name        --> name of the attribute
    return      <-- converted integer value of attribute
**************************************************************************** */

long        XML_Read_Integer_Attribute(T_XML_Node xml,T_Glyph * name)
{
    long        result,value;
    T_Glyph     buffer[MAX_XML_BUFFER_SIZE+1];

    result = XML_Get_Attribute(xml,name,buffer);
    if (result != 0)
        throw(-1);
    value = atol(buffer);
    return(value);
}


/* ***************************************************************************
    XML_Read_Float_Attribute

    Retrieve the contents of the named attribute from the given node, treating
    it as a floating point value, and converting it to a floating point return
    value.

    xml         --> node to extract the attribute from
    name        --> name of the attribute
    return      <-- converted floating point value of attribute
**************************************************************************** */

double      XML_Read_Float_Attribute(T_XML_Node xml,T_Glyph * name)
{
    long        result;
    T_Glyph     buffer[MAX_XML_BUFFER_SIZE+1];
    double      value;

    result = XML_Get_Attribute(xml,name,buffer);
    if (result != 0)
        throw(-1);
    value = (double) atof(buffer);
    return(value);
}


/* ***************************************************************************
    XML_Write_Text_Attribute

    Write the new text value out for the named attribute of the given node.

    xml         --> node to write the attribute to
    name        --> name of the attribute
    value       --> new value for the attribute
**************************************************************************** */

void        XML_Write_Text_Attribute(T_XML_Node xml,T_Glyph * name,const T_Glyph * value)
{
    long    result;

    result = XML_Set_Attribute(xml,name,value);
    if (x_Trap_Opt(result != 0))
        throw(-1);
}


/* ***************************************************************************
    XML_Write_Boolean_Attribute

    Write the new boolean value out for the named attribute of the given node,
    converting the true/false state to the corresponding "yes" or "no" value.

    xml         --> node to write the attribute to
    name        --> name of the attribute
    is_true     --> new value for the attribute
**************************************************************************** */

void        XML_Write_Boolean_Attribute(T_XML_Node xml,T_Glyph * name,bool is_true)
{
    long    result;

    result = XML_Set_Attribute(xml,name,(is_true) ? "yes" : "no");
    if (x_Trap_Opt(result != 0))
        throw(-1);
}


/* ***************************************************************************
    XML_Write_Integer_Attribute

    Write the new integer value out for the named attribute of the given node,
    converting the integer value to text.

    xml         --> node to write the attribute to
    name        --> name of the attribute
    value       --> new value for the attribute
**************************************************************************** */

void        XML_Write_Integer_Attribute(T_XML_Node xml,T_Glyph * name,long value)
{
    long    result;
    char    buffer[20];

    sprintf(buffer,"%ld",value);
    result = XML_Set_Attribute(xml,name,buffer);
    if (x_Trap_Opt(result != 0))
        throw(-1);
}


/* ***************************************************************************
    XML_Write_Float_Attribute

    Write the new floating point value out for the named attribute of the given
    node, converting the floating point value to text.

    xml         --> node to write the attribute to
    name        --> name of the attribute
    value       --> new value for the attribute
**************************************************************************** */

void        XML_Write_Float_Attribute(T_XML_Node xml,T_Glyph * name,double value)
{
    long        result;
    char        buffer[20];
    char *      ptr;

    /* synthesize the value as a string and strip off all trailing zeroes
    */
    sprintf(buffer,"%#f",value);
    ptr = buffer + strlen(buffer) - 1;
    while (*ptr == '0')
        *ptr-- = '\0';

    /* output the value as the generated string
    */
    result = XML_Set_Attribute(xml,name,buffer);
    if (x_Trap_Opt(result != 0))
        throw(-1);
}


/* ***************************************************************************
    XML_Extract_Attribute_Value

    Copies the source buffer into the destination, converting special codes
    as it goes (e.g., &lt; is translated into the < symbol).

    dest        --> Destination buffer to copy into
    src         --> Source buffer to extract from
    return      <-- 0 if successful, non-zero otherwise (if a code couldn't
                    be translated, for example).
**************************************************************************** */

long        XML_Extract_Attribute_Value(T_Glyph * dest, const T_Glyph * src)
{
    long            result,value;
    T_Glyph         glyph;
    const T_Glyph * start;

    result = 0;
    while (*src != '\0') {
        if (*src != '&') {
            *dest++ = *src++;
            continue;
            }
        src++;
        if (strncmp(src,"lt;",3) == 0) {
            *dest++ = '<';
            src += 3;
            }
        else if (strncmp(src,"gt;",3) == 0) {
            *dest++ = '>';
            src += 3;
            }
        else if (strncmp(src,"amp;",4) == 0) {
            *dest++ = '&';
            src += 4;
            }
        else if (strncmp(src,"quot;",5) == 0) {
            *dest++ = '\"';
            src += 5;
            }
        else if (strncmp(src,"apos;",5) == 0) {
            *dest++ = '\'';
            src += 5;
            }

        /* if this is an explicit character code, extract it properly
        */
        else if (*src == '#') {
            start = src++;
            if (*src != 'x')
                for (value = 0; x_Is_Digit(*src); src++)
                    value = (value * 10) + (*src - '0');
            else {
                for (src++, value = 0; true; ) {
                    glyph = *src++;
                    if ((glyph >= '0') && (glyph <= '0'))
                        glyph -= '0';
                    else if ((glyph >= 'a') && (glyph <= 'f'))
                        glyph -= 'a' - 10;
                    else if ((glyph >= 'A') && (glyph <= 'F'))
                        glyph -= 'A' - 10;
                    else
                        break;
                    value = (value * 16) + glyph;
                    }
                src--;
                }
            if (x_Trap_Opt((value == 0) || (value > 255) || (*src++ != ';'))) {
                result = -40;
                value = '&';
                src = start;
                }
            *dest++ = (T_Glyph) value;
            }

        /* If we hit a code we don't know of, set our result to 'failed' and
            continue processing.
        */
        else {
            x_Trap_Opt(1);
            result = -40;
            *dest++ = '&';
            }
        }
    *dest = '\0';
    return(result);
}
