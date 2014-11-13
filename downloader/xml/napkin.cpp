/*  FILE:   NAPKIN.CPP

    Copyright (c) 2000-2009 by Lone Wolf Development.  All rights reserved.

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

    Implementation of a class for managing an accumulated allocation string that
    gets slowly built and then thrown away en masse when no longer needed.
*/


#include    "private.h"


/*  Instantiate the object, specifying the initial size and increment to grow by
    when the current allocation is exhausted.
    Note! We use malloc for portability. This code is published as part of the
            XML source for AB.
*/
C_Napkin::C_Napkin(T_Int32U start,T_Int32U grow)
{
    m_buffer = (T_Glyph *) malloc(start);
    m_size = start;
    m_grow = grow;
    m_buffer[0] = '\0';
    m_position = 0;
    m_ptr = m_buffer;
}


C_Napkin::~C_Napkin()
{
    free(m_buffer);
}


void        C_Napkin::Grow(T_Int32U minsize)
{
    T_Int32U    size;
    T_Glyph *   enlarged;

    /* since we always need extra space to null-terminate the string, pad the
        requested minimum size by a single character
    */
    minsize++;

    /* increase the size of the buffer until it's at least the given minimum
    */
    size = m_size;
    while (size < minsize)
        size += m_grow;

    /* increase the size of the buffer; if failed, throw an exception
    */
    enlarged = (T_Glyph *) realloc(m_buffer,size);
    if (enlarged == NULL)
        x_Exception(-1000);

    /* update the new buffer and the current pointer, as the buffer MAY have
        been moved during the realloc
    */
    m_buffer = enlarged;
    m_size = size;
    m_ptr = m_buffer + m_position;
}


C_Napkin *  C_Napkin::Append(T_Glyph ch)
{
    /* if we don't have storage for the character, go get it
    */
    if (m_position + 1 >= m_size)
        Grow(m_position + 1);

    /* append the character to the buffer
    */
    *m_ptr++ = ch;
    *m_ptr = '\0';
    m_position++;
    return(this);
}


C_Napkin *  C_Napkin::Append(const T_Glyph * text)
{
    T_Int32U    length;

    /* if the text is NULL, there is nothing for us to do; we COULD assume that
        the text is never NULL, except that the string template treats an empty
        string as NULL, so we have to handle that case gracefully
    */
    if (text == NULL)
        return(this);

    /* figure out how much space we need, then ensure we have the storage needed
    */
    length = strlen(text);
    if (length + m_position >= m_size)
        Grow(length + m_position);

    /* append the string to the buffer, always keeping the buffer properly
        null-terminated
    */
    memcpy(m_ptr,text,length);
    m_ptr += length;
    *m_ptr = '\0';
    m_position += length;
    return(this);
}
