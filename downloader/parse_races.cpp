/*  FILE:   PARSE_RACES.CPP

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


C_DDI_Races::C_DDI_Races(C_Pool * pool) : C_DDI_Crawler("races", "race", "Race",
                                                        1, 3, false, pool)
{
}


C_DDI_Races::~C_DDI_Races()
{
}


T_Status        C_DDI_Races::Parse_Index_Cell(T_XML_Node node, T_Race_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;

    status = Get_Required_Child_PCDATA(&info->abilityscores, node, "DescriptionAttribute");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->size, node, "Size");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Races::Parse_Entry_Details(T_Glyph_Ptr contents, T_Race_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Race_Info> * extras)
{
    T_Status            status;
    T_Glyph_Ptr         end, powercheck, flavorcheck;
    T_Power_Info        power_info;
    bool                is_copy;

    /* Find a special checkpoint for our flavor, since otherwise we will find
        some unrelated italics way down in the text
    */
    flavorcheck = strstr(ptr, "<p class=\"flavor\">");

    /* If this race copies from another one, it's fine for there to be no
        useful information...
    */
    is_copy = Mapping_Find("racecopies", info->name) != NULL;
    if (!is_copy && x_Trap_Opt(flavorcheck == NULL))
        x_Status_Return(LWD_ERROR);
    if (flavorcheck == NULL) {
        info->flavor = "";
        ptr = strstr(ptr, "<br/>");
        if (x_Trap_Opt(ptr == NULL))
            x_Status_Return(LWD_ERROR);
        }
    else {
        ptr = Extract_Text(&info->flavor, m_pool, ptr, flavorcheck, "<i>", "</i>");
        Strip_Bad_Characters(info->flavor);
        }

    ptr = Extract_Text(&info->height, m_pool, ptr, checkpoint, "<b>Average Height</b>:", "<br");

    ptr = Extract_Text(&info->weight, m_pool, ptr, checkpoint, "<b>Average Weight</b>:", "<br");

    /* This was parsed from the index above, but we might as well grab it again
        here, in case there's more information
    */
    ptr = Extract_Text(&info->abilityscores, m_pool, ptr, checkpoint, "<b>Ability scores</b>:", "<br");

    /* This was parsed from the index above, but we might as well grab it again
        here, in case there's more information
    */
    ptr = Extract_Text(&info->size, m_pool, ptr, checkpoint, "<b>Size</b>:", "<br");

    ptr = Extract_Text(&info->speed, m_pool, ptr, checkpoint, "<b>Speed</b>:", "<br");

    ptr = Extract_Text(&info->vision, m_pool, ptr, checkpoint, "<b>Vision</b>:", "<br");

    ptr = Extract_Text(&info->languages, m_pool, ptr, checkpoint, "<b>Languages</b>:", "<br");

    ptr = Extract_Text(&info->skillbonuses, m_pool, ptr, checkpoint, "<b>Skill Bonuses</b>:", "<br");

    /* Everything from here to the end of the paragraph is description text.
        (Some badly formed races end at a </div>, not a </p> - we have to set
        the "find at checkpoint ok" flag for these.)
    */
    end = Find_Text(ptr, "</p>", checkpoint, false, false);
    if (end == NULL)
        end = Find_Text(ptr, "</div>", checkpoint, false, true);
    if (x_Trap_Opt(end == NULL))
        x_Status_Return_Success();
    *end = '\0';
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    *end = '<';

    /* If we have a power span, extract the power and all following powers
    */
    ptr = strstr(ptr, "<h1 class=\"");
    while ((ptr != NULL) && (ptr < checkpoint)) {
        memset(&power_info, 0, sizeof(power_info));
        if (x_Trap_Opt(info->power_count >= x_Array_Size(info->powers)))
            break;

        /* Get the new checkpoint, which is the first "Published in..." or <h1
            or the SECOND </span section we find (the first is part of the
            power title) or maybe even some random close paragraph / line break
            tags.
        */
        powercheck = strstr(ptr+1, "<h1");
        if (powercheck == NULL) {
            powercheck = strstr(ptr, "</span");
            if (powercheck != NULL)
                powercheck = strstr(powercheck+1, "</span");
            }
        if (powercheck == NULL)
            powercheck = strstr(ptr, "</p><br/><br/>");
        if (powercheck == NULL)
            powercheck = strstr(ptr, "<p>Published in");
        if (powercheck == NULL)
            powercheck = checkpoint;

        /* You can grab the name after the end of the first span
        */
        Extract_Text(&power_info.name, m_pool, ptr, powercheck, "</span>", "</h1>");
        Strip_Bad_Characters(power_info.name);
        power_info.forclass = "Race";
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
        ptr = strstr(powercheck, "<h1 class=\"");
        }

    x_Status_Return_Success();
}
