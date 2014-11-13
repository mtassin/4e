/*  FILE:   PARSE_RITUALS.CPP

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


C_DDI_Rituals::C_DDI_Rituals(C_Pool * pool) : C_DDI_Crawler("rituals", "ritual",
                                                            "Ritual", 7, 4, false,
                                                            pool)
{
}


C_DDI_Rituals::~C_DDI_Rituals()
{
}


T_Status        C_DDI_Rituals::Parse_Index_Cell(T_XML_Node node, T_Ritual_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;

    status = Get_Required_Child_PCDATA(&info->level, node, "Level");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->price, node, "Price");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    /* If our level is '0', that's obviously nonsense so get
        rid of it
    */
    if ((info->level[0] == '0') && (info->level[1] == '\0'))
        info->level = "";

    status = Get_Required_Child_PCDATA(&info->componentcost, node, "ComponentCost");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->keyskill, node, "KeySkillDescription");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Rituals::Parse_Entry_Details(T_Glyph_Ptr contents, T_Ritual_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Ritual_Info> * extras)
{
    T_Status            status;
    T_Glyph_Ptr         end;
    T_Glyph_Ptr         starts[] = { "???", };
    T_Glyph_Ptr         ends[] = { "<br", "</p", "</span" };

    ptr = Extract_Text(&info->flavor, m_pool, ptr, checkpoint, "<span class=\"flavor\">", "</span>");
    Strip_Bad_Characters(info->flavor);

    /* This was parsed from the index above, but we might as well grab it again
        here, in case there's more information
    */
    starts[0] = "<b>Component Cost</b>:";
    ptr = Extract_Text(&info->componentcost, m_pool, ptr, checkpoint, starts, x_Array_Size(starts),
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->componentcost);

    /* Although we've parsed the skill out of the index, replace it if we can -
        the one on the actual page has more data (e.g. "no check" info).
    */
    starts[0] = "<b>Key Skill</b>:";
    ptr = Extract_Text(&info->keyskill, m_pool, ptr, checkpoint, starts, x_Array_Size(starts),
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->keyskill);

    /* Although we've parsed the level out of the index, replace it if we can -
        the one on the actual page has more data.
    */
    starts[0] = "<b>Level</b>:";
    ptr = Extract_Text(&info->level, m_pool, ptr, checkpoint, starts, x_Array_Size(starts),
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->level);

    starts[0] = "<b>Category</b>:";
    ptr = Extract_Text(&info->category, m_pool, ptr, checkpoint, starts, x_Array_Size(starts),
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->category);

    starts[0] = "<b>Time</b>:";
    ptr = Extract_Text(&info->time, m_pool, ptr, checkpoint, starts, x_Array_Size(starts),
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->time);

    starts[0] = "<b>Duration</b>:";
    ptr = Extract_Text(&info->duration, m_pool, ptr, checkpoint, starts, x_Array_Size(starts),
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->duration);

    starts[0] = "<b>Prerequisite</b>:";
    ptr = Extract_Text(&info->prerequisite, m_pool, ptr, checkpoint, starts, x_Array_Size(starts),
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->prerequisite);

    /* Parse out any paragraph close tag we encounter here
    */
    if (strnicmp(ptr, "</p>", 4) == 0)
        ptr += 4;

    /* Now parse the rest of the description - it might end in a </p> tag, or
        it might not! Who knows!
    */
    end = Find_Text(ptr, "</p>", checkpoint, false, false);
    if (end == NULL)
        end = checkpoint;
    *end = '\0';
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    *end = '<';

    x_Status_Return_Success();
}
