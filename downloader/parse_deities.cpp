/*  FILE:   PARSE_DEITIES.CPP

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


template <class T> C_DDI_Crawler<T> *   C_DDI_Crawler<T>::s_crawler = NULL;


C_DDI_Deities::C_DDI_Deities(C_Pool * pool) : C_DDI_Crawler("deities", "deity", "Deity",
                                                            6, 2, false, pool)
{
}


C_DDI_Deities::~C_DDI_Deities()
{
}


T_Status        C_DDI_Deities::Parse_Index_Cell(T_XML_Node node, T_Deity_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;

    status = Get_Required_Child_PCDATA(&info->alignment, node, "Alignment");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Deities::Parse_Entry_Details(T_Glyph_Ptr contents, T_Deity_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Deity_Info> * extras)
{
    T_Status            status;
    T_Glyph_Ptr         end;

    ptr = Extract_Text(&info->flavor, m_pool, ptr, checkpoint, "<b>Type: </b>", "<br");

    ptr = Extract_Text(&info->alignment, m_pool, ptr, checkpoint, "<b>Alignment :</b>", "</b><br");

    ptr = Extract_Text(&info->gender, m_pool, ptr, checkpoint, "<b>Gender:</b>", "<br");

    ptr = Extract_Text(&info->sphere, m_pool, ptr, checkpoint, "<b>Sphere:</b>", "<br");

    ptr = Extract_Text(&info->dominion, m_pool, ptr, checkpoint, "<b>Dominion:</b>", "<br");

    ptr = Extract_Text(&info->priests, m_pool, ptr, checkpoint, "<b>Priests:</b>", "<br");

    ptr = Extract_Text(&info->adjective, m_pool, ptr, checkpoint, "<b>Adjective:</b>", "<br");

    /* The description text appears after the end of the paragraph node
    */
    ptr = Find_Text(ptr, "</p>", checkpoint, true, false);
    if (x_Trap_Opt(ptr == NULL))
        x_Status_Return_Success();
    end = Find_Text(ptr, "<p>", checkpoint, false, false);
    if (x_Trap_Opt(end == NULL))
        x_Status_Return_Success();
    *end = '\0';
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    *end = '<';

    x_Status_Return_Success();
}
