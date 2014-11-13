/*  FILE:   PARSE_BACKGROUNDS.CPP

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
*/


#include "private.h"


/*  Include our template class definition - see the comment at the top of the
    crawler.h file for details. Summary: C++ templates are terrible.
*/
#include "crawler.h"


template<class T> C_DDI_Crawler<T> *    C_DDI_Crawler<T>::s_crawler = NULL;


C_DDI_Backgrounds::C_DDI_Backgrounds(C_Pool * pool) : C_DDI_Crawler("backgrounds", "background",
                                                        "Background", 4, 4, true, pool)
{
}


C_DDI_Backgrounds::~C_DDI_Backgrounds()
{
}


T_Status        C_DDI_Backgrounds::Parse_Index_Cell(T_XML_Node node, T_Background_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;

    status = Get_Required_Child_PCDATA(&info->type, node, "Type");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    status = Get_Required_Child_PCDATA(&info->campaign, node, "Campaign");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    status = Get_Required_Child_PCDATA(&info->skills, node, "Skills");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Backgrounds::Parse_Entry_Details(T_Glyph_Ptr contents, T_Background_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Background_Info> * extras)
{
    T_Status            status;
    T_Glyph_Ptr         temp, end;

    /* Get any pre-requisite for the background
    */
    ptr = Extract_Text(&info->prerequisite, m_pool, ptr, checkpoint, "<b>Prerequisite: </b>", "<br");

    /* The benefit appears at the end for some silly reason
    */
    Extract_Text(&info->benefit, m_pool, ptr, checkpoint, "<i>Benefit: </i>", "<br/><p");

    /* Just get everything as the description text for now
    */
    temp = strstr(ptr, "<br/></p>");
    if (temp != NULL)
        ptr = temp + 9;
    end = strstr(ptr, "<p>Published in");
    if (end != NULL)
        *end = '\0';
    end = strstr(ptr, "<p class=\"publishedIn\">Published i");
    if (end != NULL)
        *end = '\0';
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    x_Status_Return_Success();
}
