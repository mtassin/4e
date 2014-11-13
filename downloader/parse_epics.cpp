/*  FILE:   PARSE_EPICS.CPP

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


C_DDI_Epics::C_DDI_Epics(C_Pool * pool) : C_DDI_Crawler("destinies", "epicdestiny",
                                                        "EpicDestiny", 9, 2, false,
                                                        pool)
{
}


C_DDI_Epics::~C_DDI_Epics()
{
}


T_Status        C_DDI_Epics::Parse_Index_Cell(T_XML_Node node, T_Epic_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;

    status = Get_Required_Child_PCDATA(&info->prerequisite, node, "Prerequisite");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Epics::Parse_Entry_Details(T_Glyph_Ptr contents, T_Epic_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Epic_Info> * extras)
{
    T_Status            status;
    T_Glyph_Ptr         end, saved;
    T_Glyph_Ptr         starts[] = { "???", };
    T_Glyph_Ptr         ends[] = { "???", "???", "???" };

    /* The flavor text is the PCDATA of the first <i> node after that
    */
    ptr = Extract_Text(&info->flavor, m_pool, ptr, checkpoint, "<i>", "</i>");
    Strip_Bad_Characters(info->flavor);

    ptr = Extract_Text(&info->prerequisite, m_pool, ptr, checkpoint, "<b>Prerequisite: </b>", "<br");

    saved = ptr;
    starts[0] = "FEATURES</h3>";
    ends[0] = "<p><h1";
    ends[1] = "<p>Published in";
    ends[2] = "<p class=\"publishedIn\">";
    ptr = Extract_Text(&info->features, m_pool, ptr, checkpoint, starts, 1, ends, 3);
    Strip_Bad_Characters(info->features);

    /* Now parse the rest of the description - if we find an h1 tag or a
        "published in" section, we're done
    */
    ptr = saved;
    end = Find_Text(ptr, "<p><h1", checkpoint, false, false);
    if (end == NULL)
        end = Find_Text(ptr, "<p>Published in", checkpoint, false, false);
    if (end == NULL)
        end = Find_Text(ptr, "<p class=\"publishedIn\">", checkpoint, false, false);
    if (x_Trap_Opt(end == NULL))
        x_Status_Return_Success();
    *end = '\0';
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    *end = '<';

    /* If we have a power span, extract the power and all following powers
    */
// I guess we don't need this any more - all the powers must be included in the
// powers list now
#if 0
    ptr = strstr(ptr, "<h1 class=\"");
    while ((ptr != NULL) && (ptr < checkpoint)) {
        memset(&power_info, 0, sizeof(power_info));
        power_info.forclass = info->name;

        /* Get the new checkpoint, which is the first "First published in..."
            section we find
        */
        powercheck = strstr(ptr, "<p>Published in");
        if (powercheck == NULL)
            powercheck = checkpoint;

        /* You can grab the name after the end of the first span
        */
        saved = ptr;
        Extract_Text(&power_info.name, m_pool, ptr, powercheck, "</span>", "</h1>");

        /* If our name is empty, this power is one of the weird ones that is
            written differently (and wrongly); the <span> comes after the power
            name.
        */
        if (power_info.name[0] == '\0') {
            ptr = saved;
            Extract_Text(&power_info.name, m_pool, ptr, powercheck, ">", "<span");
            if (x_Trap_Opt(power_info.name[0] == '\0'))
                goto next_power;
            ptr = strstr(ptr, "</span>");
            if (x_Trap_Opt((ptr == NULL) || (ptr >= powercheck)))
                goto next_power;
            }

        Strip_Bad_Characters(power_info.name);
        power_info.parent_info = info;

        /* Parse things to extract the rest of the data
        */
        C_DDI_Powers::Get_Crawler()->Parse_Entry_Details(ptr, &power_info, ptr, powercheck, NULL);

        /* Add a record to the race, so we can bootstrap the power once it gets
            processed
        */
        info->powers[info->power_count++] = C_DDI_Powers::Get_Crawler()->Append_Item(&power_info);;

        /* Try to find following powers!
        */
next_power:
        ptr = strstr(powercheck, "<h1 class=\"");
        }
#endif

    x_Status_Return_Success();
}
