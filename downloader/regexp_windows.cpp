/*  FILE:   REGEXP_WINDOWS.CPP

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


#include <regex>

#include <string>

#include "private.h"


struct T_Regexp {

    T_Regexp(T_Glyph_Ptr pattern) : rx(pattern) { }

    regex                       rx;
    string                      search_string;
    sregex_token_iterator       iter;
    const sregex_token_iterator end;
};


bool     Is_Regex_Match(T_Glyph_Ptr regexp, T_Glyph_Ptr text, T_Glyph_Ptr capture1, T_Glyph_Ptr capture2)
{
    regex       rx(regexp);
    cmatch      match;

    if (capture1 != NULL)
        capture1[0] = '\0';
    if (capture2 != NULL)
        capture2[0] = '\0';

    if (!regex_search(text, match, rx))
        return(false);

    if (capture1 != NULL)
        strcpy(capture1, match[1].str().c_str());
    if (capture2 != NULL)
        strcpy(capture2, match[2].str().c_str());
    return(true);
}


T_Void_Ptr      Regex_Create(T_Glyph_Ptr pattern)
{
    T_Regexp *  rx;

    rx = new T_Regexp(pattern);
    if (x_Trap_Opt(rx == NULL))
        return(NULL);
    return(rx);
}


string          Regex_Find_Match_Captures(T_Void_Ptr rx_ptr, T_Glyph_Ptr search_string, const vector<int> & captures)
{
    T_Regexp *  rx = (T_Regexp *) rx_ptr;

    /* Save the search string we iterate over in the object, otherwise it goes
        out of scope when we return and causes problems
    */
    rx->search_string = search_string;
    rx->iter = sregex_token_iterator(rx->search_string.cbegin(), rx->search_string.cend(), rx->rx, captures);
    if (rx->iter == rx->end)
        return("");
    return(rx->iter->str());
}


string          Regex_Next_Match_Capture(T_Void_Ptr rx_ptr)
{
    T_Regexp *  rx = (T_Regexp *) rx_ptr;

    rx->iter++;
    if (rx->iter == rx->end)
        return("");
    return(rx->iter->str());
}


void            Regex_Destroy(T_Void_Ptr rx_ptr)
{
    T_Regexp *  rx = (T_Regexp *) rx_ptr;

    delete rx;
}
