/*  FILE:   REGEXP_OSX.CPP
 
 Copyright (c) 2012 by Lone Wolf Development, Inc.  All rights reserved.
 
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
 */


#include <regex.h>

#include <string>


#include "private.h"


#define MAX_CAPTURES    20 // max capture groups

#define FIRST_MATCH     ((T_Int32U) -1)


struct T_Regexp {

    regex_t             rx;
    T_Glyph_CPtr        search_base;
    T_Glyph_CPtr        search_string;
    T_Int32U            next_capture;
    vector<int>         captures;
    regmatch_t          matches[MAX_CAPTURES];
};


static void Copy_Capture(T_Glyph_Ptr buffer, T_Glyph_CPtr text, regmatch_t * match)
{
    int         length;
    
    length = (int) (match->rm_eo - match->rm_so);
    strncpy(buffer, text + match->rm_so, length);
    buffer[length] = '\0';
}


bool     Is_Regex_Match(T_Glyph_Ptr regexp, T_Glyph_Ptr text, T_Glyph_Ptr capture1, T_Glyph_Ptr capture2)
{
    int         result, size;
    bool        is_match = false;
    regex_t     rx;
    regmatch_t  matches[3]; // entire string + 2 return parameters
    
    if (capture1 != NULL)
        capture1[0] = '\0';
    if (capture2 != NULL)
        capture2[0] = '\0';
        
    if (text[0] == '\0')
        return(false);

    result = regcomp(&rx, regexp, REG_EXTENDED);
    if (x_Trap_Opt(result != 0))
        return(false);
      
    /* If we didn't match, there are no capture groups to return. If the caller
        doesn't want us to capture anything, we can just pass 0 for the size.
    */
    size = ((capture1 == NULL) && (capture2 == NULL)) ? 0 : x_Array_Size(matches);
    result = regexec(&rx, text, size, matches, 0);
    if (result == REG_NOMATCH)
        goto cleanup_exit;
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;
        
    is_match = true;
    
    /* Copy our captures into the parameters, if provided.
        NOTE: matches[0] holds the entire matched string, so we only
            care about what's in matches[1] and following, which are
            the capture group results.
    */
    if (capture1 != NULL)
        Copy_Capture(capture1, text, &matches[1]);
    if (capture2 != NULL)
        Copy_Capture(capture2, text, &matches[2]);
        
cleanup_exit:
    regfree(&rx);
    return(is_match);
}


T_Void_Ptr      Regex_Create(T_Glyph_Ptr pattern)
{
    T_Regexp *  rx;
    int         result;
    T_Glyph     message[500];
    
    rx = new T_Regexp;
    if (x_Trap_Opt(rx == NULL))
        return(NULL);

    result = regcomp(&rx->rx, pattern, REG_EXTENDED);
    if (x_Trap_Opt(result != 0)) {
        sprintf(message, "Couldn't create regular expression: error %d\n", result);
        Log_Message(message, TRUE);
        delete rx;
        return(NULL);
        }
    
    return(rx);
}


static string   Next_Match_Internal(T_Regexp * rx)
{
    int         result;
    bool        is_execute = false;
    T_Glyph     temp[5000];
    
    /* Check if we need to do a fresh search.
    */
    if ((rx->next_capture == FIRST_MATCH) ||
        (rx->next_capture >= rx->captures.size())) {
        
        /* If we finished the capture groups we were interested in, we need to
            move where we look further into the string to find the next match
        */
        if (rx->next_capture != FIRST_MATCH)
            rx->search_string += rx->matches[0].rm_eo;
        
        /* We'll need to do a fresh search for the first capture group again
        */
        is_execute = true;
        rx->next_capture = 0;
        }
    if (is_execute) {
        if (rx->search_string[0] == '\0')
            return("");

        result = regexec(&rx->rx, rx->search_string, MAX_CAPTURES, rx->matches, 0);
        if (result == REG_NOMATCH)
            return("");
        if (x_Trap_Opt(result != 0))
            return("");
        }
        
    if (x_Trap_Opt(rx->next_capture >= MAX_CAPTURES)) {
        Log_Message("Not enough possible capture groups to match expression\n", TRUE);
        return("");
        }
        
    /* At this point we've either got new matches, or we're re-using the ones
        we found previously. Find the nth match and then set our next capture to
        be the next one in the vector.
    */
    Copy_Capture(temp, rx->search_string, &rx->matches[rx->captures[rx->next_capture]]);
    rx->next_capture++;
    return(string(temp));
}


string          Regex_Find_Match_Captures(T_Void_Ptr rx_ptr, T_Glyph_Ptr search_string, const vector<int> & captures)
{
    T_Regexp *  rx = (T_Regexp *) rx_ptr;
    
    if (x_Trap_Opt(rx == NULL))
        return("");
    
    /* Initialize the structure for matching.
    */
    rx->search_base = search_string;
    rx->search_string = search_string;
    rx->captures = captures;
    rx->next_capture = FIRST_MATCH;
    
    return(Next_Match_Internal(rx));
}


string          Regex_Next_Match_Capture(T_Void_Ptr rx_ptr)
{
    T_Regexp *  rx = (T_Regexp *) rx_ptr;
    
    return(Next_Match_Internal(rx));
}


void            Regex_Destroy(T_Void_Ptr rx_ptr)
{
    T_Regexp *  rx = (T_Regexp *) rx_ptr;
    
    regfree(&rx->rx);
    delete rx;
}
