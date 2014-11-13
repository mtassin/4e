/*  FILE:   TEXT.CPP

    Copyright (c) 2008-2012 by Lone Wolf Development, Inc.  All rights reserved.

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

    Text-processing functions for the D&DI Crawler.
*/


#include "private.h"

#include    <stdarg.h>


/* This function takes unicode (or whatever) sequences like — that appear in
    the web pages we download, and turns them into nice equivalents.
*/
void        Strip_Bad_Characters(T_Glyph_Ptr buffer)
{
    T_Glyph             endquote[10], uaccent[10], caccent[10], bullet[10], wtf[10];
    T_Glyph_Ptr         search[] = { "—", "–", "’", "“", endquote, uaccent, caccent, bullet, wtf, "<br/>", "<td>", "</td>", "\t", "&nbsp;", };
    T_Glyph_Ptr         replace[] = { " - ", "-", "\'", "\"", "\"", "u", "c", "*", " ", "{br}", "", "", "", "", };
    T_Glyph             temp[200000];

    if (buffer == NULL)
        return;

    /* Set up horrible strings that can't be represented as plain text. The
        first one is a very odd end quote mark.
    */
    endquote[0] = (T_Glyph) 226;
    endquote[1] = (T_Glyph) 128;
    endquote[2] = (T_Glyph) 157;
    endquote[3] = '\0';

    /* Now a u with an accent over it.
    */
    uaccent[0] = (T_Glyph) 195;
    uaccent[1] = (T_Glyph) 187;
    uaccent[2] = '\0';

    /* c with an accent over it.
    */
    caccent[0] = (T_Glyph) 195;
    caccent[1] = (T_Glyph) 167;
    caccent[2] = '\0';

    /* A bullet.
    */
    bullet[0] = (T_Glyph) 226;
    bullet[1] = (T_Glyph) 128;
    bullet[2] = (T_Glyph) 162;
    bullet[3] = '\0';

    /* I don't even know what this is. It's in the Prestigiditation power details.
    */
    wtf[0] = (T_Glyph) 239;
    wtf[1] = (T_Glyph) 128;
    wtf[2] = (T_Glyph) 160;
    wtf[3] = '\0';

    Text_Find_Replace(temp, buffer, strlen(buffer), x_Array_Size(search), search, replace);
    strcpy(buffer, temp);
}


T_Status        Parse_Description_Text(T_Glyph_Ptr * final, T_Glyph_Ptr text, C_Pool * pool,
                                        bool is_close_p_newlines, T_Glyph_Ptr find_end)
{
    T_Glyph_Ptr     src, converted, collapsed, dest, end, ptr;
    T_Int32U        count;
    T_Glyph         backup;
    bool            is_in_tag;
    T_Glyph_Ptr     search[] = { "\r", "<br>", "<br/>", "<br/>", "<br >", "<br />", "<b>", "</b>", "<i>", "</i>", "</td><td>", "</th><th>", "</tr><tr>", "<img src=\"http://www.wizards.com/dnd/images/symbol/x.gif\" />", "[replaced]", "[replaced]" };
    T_Glyph_Ptr     replace[] = { "", "\n", "\n", "\n", "\n", "\n", "{b}", "{/b}", "{i}", "{/i}", " - ", " - ", "{br}", "*", "[replaced]", "[replaced]" };
    T_Glyph_Ptr     search2[] = { "\n", "{br}{br}{br}" };
    T_Glyph_Ptr     replace2[] = { "{br}", "{br}{br}" };

    /* If we want </p> tags and </h2> tags to turn into two newlines, add them
        to the last entry in the replacement array - otherwise reduce our count
        by 2 to ignore them
    */
    count = x_Array_Size(search);
    if (!is_close_p_newlines)
        count -= 2;
    else {
        search[count-1] = "</p>";
        replace[count-1] = "\n\n";
        search[count-2] = "</h2>";
        replace[count-2] = ": ";
        }

    /* If we have an end to find, find it
    */
    if (find_end != NULL) {
        end = strstr(text, find_end);
        if (x_Trap_Opt(end == NULL))
            x_Status_Return(LWD_ERROR);
        backup = *end;
        *end = NULL;
        }

    /* First, stick the text into a growstring and convert all the tags we care
        about - note that we add 1000 characters of padding just in case
    */
    converted = new T_Glyph[strlen(text) + 1000];
    if (x_Trap_Opt(converted == NULL)) {
        *end = backup;
        x_Status_Return(LWD_ERROR);
        }
    converted[0] = '\0';
    Text_Find_Replace(converted, text, strlen(text) + 1000, count, search, replace);

    /* Restore any character we terminated
    */
    if (find_end != NULL)
        *end = backup;

    /* Now zip through the remaining text, deleting all tags altogether
    */
    src = converted;
    dest = converted;
    is_in_tag = false;
    while (*src != '\0') {
        if (!is_in_tag) {
            if (*src == '<')
                is_in_tag = true;
            else
                *dest++ = *src;
            }
        else if (*src == '>')
            is_in_tag = false;
        src++;
        }
    *dest = '\0';

    /* Trim excess whitespace, then replace all \ns with {br}s - note that we
        add 1000 characters of padding just in case
    */
    Text_Trim_Leading_Space(converted);
    collapsed = new T_Glyph[strlen(converted) + 1000];
    if (x_Trap_Opt(collapsed == NULL))
        x_Status_Return(LWD_ERROR);
    collapsed[0] = '\0';
    Text_Find_Replace(collapsed, converted, strlen(converted) + 1000, x_Array_Size(search2), search2, replace2);

    /* Stick the description where it belongs, and trim out all bad characters
    */
    *final = pool->Acquire(collapsed);
    Strip_Bad_Characters(*final);
    delete [] converted;
    delete [] collapsed;

    /* Trim all trailing "{br}" sequences
    */
    ptr = *final + strlen(*final);
    while (strcmp(ptr-4, "{br}") == 0) {
        ptr -= 4;
        *ptr = '\0';
        }

    x_Status_Return_Success();
}


T_Glyph_Ptr     Find_Text(T_Glyph_Ptr haystack, T_Glyph_Ptr needle, T_Glyph_Ptr checkpoint,
                            bool is_move_past, bool is_find_checkpoint_ok)
{
    T_Glyph_Ptr         ptr;

    /* If we don't have anything to find, something is wrong
    */
    if (x_Trap_Opt(needle[0] == '\0'))
        return(NULL);

    /* If we just need to find one character, call strchr; otherwise, use
        strstr.
    */
    if (needle[1] == '\0')
        ptr = strchr(haystack, needle[0]);
    else
        ptr = strstr(haystack, needle);
    if (ptr == NULL)
        return(NULL);

    /* Check to see if we're past our checkpoint; if so, just return NULL. If
        our 'find checkpoint ok' flag is set, it's ok to find an item AT the
        checkpoint.
    */
    if ((checkpoint != NULL) &&
        ((!is_find_checkpoint_ok && (ptr >= checkpoint)) ||
         (is_find_checkpoint_ok && (ptr > checkpoint))))
        return(NULL);

    /* Otherwise, move past what we searched for
    */
    if (is_move_past)
        ptr += strlen(needle);
    return(ptr);
}


T_Glyph_Ptr     Extract_Text(T_Glyph_Ptr * final, C_Pool * pool, T_Glyph_Ptr text,
                                T_Glyph_Ptr checkpoint, T_Glyph_Ptr find, T_Glyph_Ptr find_end)
{
    T_Glyph_Ptr find_array[] = { find };
    T_Glyph_Ptr end_array[] = { find_end };

    return(Extract_Text(final, pool, text, checkpoint, find_array, 1, end_array, 1));
}


/* This function searches for all of the strings in find_sequence, appearing
    one after another in the text, and terminates them with the earliest
    sequence found in end_options.
    NOTE: This function returns the new position of the pointer after the text
        has been extracted.
*/
T_Glyph_Ptr     Extract_Text(T_Glyph_Ptr * final, C_Pool * pool, T_Glyph_Ptr text,
                                T_Glyph_Ptr checkpoint, T_Glyph_Ptr find_sequence[],
                                T_Int32U find_count, T_Glyph_Ptr end_options[],
                                T_Int32U end_count)
{
    T_Int32U            i;
    T_Glyph_Ptr         ptr, end, temp;
    T_Glyph             backup;

    /* Validate our input parameters
    */
    if (x_Trap_Opt((final == NULL) || (pool == NULL) || (text == NULL) ||
         (find_sequence == NULL) || (find_count == 0) ||
         (end_options == NULL) || (end_count == 0)))
         return(text);

    /* Find a pattern within the text that matches - if we don't find something
        we're looking for, just degrade gracefully and return the initial text
        position. If we have more than one entry to find, search for the first,
        then the second appearing after the first, etc.
        NOTE: If we are given blank text to find, assume we're at the right
            place already.
    */
    ptr = text;
    if (find_sequence[0][0] != '\0')
        for (i = 0; i < find_count; i++) {
            ptr = Find_Text(text, find_sequence[i], checkpoint, true, false);
            if (ptr != NULL)
                break;
            }
    if (ptr == NULL)
        return(text);

    /* Now find the end of the sequence. If we're given multiple options, find
        and return the earliest of them.
    */
    end = NULL;
    for (i = 0; i < end_count; i++) {
        temp = Find_Text(ptr, end_options[i], checkpoint, false, true);
        if ((temp != NULL) && ((end == NULL) || (temp < end)))
            end = temp;
        }
    if (x_Trap_Opt(end == NULL))
        return(text);

    /* Make a copy of the text and return the new pointer position at the end
        we just found
    */
    backup = *end;
    *end = '\0';
    *final = pool->Acquire(ptr);
    Text_Trim_Leading_Space(*final);
    *end = backup;
    return(end);
}


/* NOTE: This text returns the new position of the pointer after the text has
    been extracted.
*/
T_Glyph_Ptr     Extract_Number(T_Glyph_Ptr * final, C_Pool * pool, T_Glyph_Ptr text,
                                T_Glyph_Ptr checkpoint, T_Glyph_Ptr find)
{
    T_Glyph_Ptr         ptr, end;
    T_Glyph             backup;

    if (x_Trap_Opt((final == NULL) || (pool == NULL) || (text == NULL) ||
         (checkpoint == NULL) || (find == NULL)))
         return(text);

    /* Find a pattern within the text that matches - if we don't find something
        we're looking for, just degrade gracefully and return the initial text
        position.
    */
    ptr = Find_Text(text, find, checkpoint, true, false);
    if (ptr == NULL)
        return(text);

    /* Skip along until we find a number
    */
    while (!x_Is_Digit(*ptr) && (ptr < checkpoint))
        ptr++;

    /* Now skip to the end of the number
    */
    end = ptr;
    while (x_Is_Digit(*end) && (end < checkpoint))
        end++;

    /* If we have a decimal point now, see if there are numbers on the other
        side of it, since this could be a floating-point number.
    */
    if ((*end == '.') && x_Is_Digit(end[1]) && (end < checkpoint))
        end++;
    while (x_Is_Digit(*end) && (end < checkpoint))
        end++;

    /* Make sure we didn't run past our checkpoint - if we did, we didn't find
        what we were looking for
    */
    if (x_Trap_Opt(end > checkpoint))
        return(text);

    /* Make a copy of the text and return the new pointer position after the
        text has been consumed
    */
    backup = *end;
    *end = '\0';
    *final = pool->Acquire(ptr);
    *end = backup;
    return(end);
}


/*  Compare the rightmost characters of a string to another string.
*/
T_Int32S    stricmpright(T_Glyph_Ptr target, T_Glyph_Ptr compare)
{
    T_Int32U        target_length, compare_length;

    target_length = strlen(target);
    compare_length = strlen(compare);
    if (target_length <= compare_length)
        return(stricmp(target, compare));
    return(stricmp(target + target_length - compare_length, compare));
}


/*  Strip all trailing spaces from the string.
*/
void    Text_Trim_Trailing_Space(T_Glyph_Ptr src)
{
    T_Glyph_Ptr ptr;

    /* strip all trailing whitespace from the end of the string
    */
    if (*src != '\0') {
        for (ptr = src + strlen(src) - 1; x_Is_Space(*ptr); ptr--);
        ptr[1] = '\0';
        }
}


/*  Strip all leading and trailing spaces from the string.
*/
void    Text_Trim_Leading_Space(T_Glyph_Ptr src)
{
    T_Glyph_Ptr ptr;

    /* skip over all whitespace and collapse the string to eliminate it
    */
    for (ptr = src; (*ptr != '\0') && x_Is_Space(*ptr); ptr++);
    if (ptr != src)
        strcpy(src,ptr);

    /* Now trim trailing whitespace
    */
    Text_Trim_Trailing_Space(src);
}


//Returns the number of replacements made
//NOTE! The client is responsible for ensuring the destination buffer is large
//      enough to hold the final text, since no tests are made herein.
T_Int32U    Text_Find_Replace(T_Glyph_Ptr dest,T_Glyph_Ptr src,T_Glyph_Ptr find,T_Glyph_Ptr replace)
{
    T_Int32U    count,length,skip;

    /* scan the entire source buffer, looking for matches and replacing them
    */
    length = strlen(find);
    skip = strlen(replace);
    for (count = 0; *src != '\0'; ) {

        /* if this character is not a match for the string we're looking to
            replace, just copy it across
        */
        if (*src != *find) {
            *dest++ = *src++;
            continue;
            }

        /* see if we've found a match for our search string, if not, copy the
            character and continue
        */
        if (strncmp(src,find,length) != 0) {
            *dest++ = *src++;
            continue;
            }

        /* a match was found, so copy the replacement text into the buffer and
            skip over the matched string in the source
        */
        strcpy(dest,replace);
        dest += skip;
        src += length;
        }

    /* null-terminate the buffer and return the number of replacements made
    */
    *dest = '\0';
    return(count);
}


void    Text_Find_Replace(T_Glyph_Ptr dest,T_Glyph_Ptr src,T_Int32U length,
                                T_Int32U count,T_Glyph_Ptr * find,T_Glyph_Ptr * replace)
{
    T_Status    status;
    T_Int32U    i;
    T_Glyph_Ptr from,target;
    T_Glyph_Ptr buffer1,buffer2;

    /* allocate intermediate buffers to be used during the operation
    */
    status = Mem_Acquire(length+1,(T_Void_Ptr *) &buffer1);
    if (x_Trap_Opt(!x_Is_Success(status)))
        return;
    memset(buffer1, 0, length+1);
    status = Mem_Acquire(length+1,(T_Void_Ptr *) &buffer2);
    if (x_Trap_Opt(!x_Is_Success(status)))
        return;
    memset(buffer2, 0, length+1);

    /* perform the set of find and replace operations
    */
    for (i = 0, from = target = NULL; i < count; i++) {

        /* setup the source location to copy from properly, starting with the
            provided source and otherwise swapping between internal buffers
            NOTE! The source sequence is: src, buf1, buf2, buf1, buf2, etc.
        */
        if (i == 0)
            from = src;
        else if (from == buffer1)
            from = buffer2;
        else
            from = buffer1;

        /* setup the target location to copy into properly, swapping between the
            internal buffers and ending with the provided destination
            NOTE! The target sequence is: buf1, buf2, buf1, buf2, dest.
        */
        if (i == count - 1)
            target = dest;
        else if (target == buffer1)
            target = buffer2;
        else
            target = buffer1;

        /* perform the search and replace of the next pair
        */
        Text_Find_Replace(target,from,find[i],replace[i]);
        }

    /* release our intermediate buffers
    */
    Mem_Release(buffer1);
    Mem_Release(buffer2);
}
