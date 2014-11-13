/*  FILE:   PARSE_FEATS.CPP

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


C_DDI_Feats::C_DDI_Feats(C_Pool * pool) : C_DDI_Crawler("feats", "feat", "Feat",
                                                        4, 2, true, pool)
{
}


C_DDI_Feats::~C_DDI_Feats()
{
}


T_Status        C_DDI_Feats::Parse_Index_Cell(T_XML_Node node, T_Feat_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;

    status = Get_Required_Child_PCDATA(&info->tier, node, "TierName");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Feats::Parse_Entry_Details(T_Glyph_Ptr contents, T_Feat_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Feat_Info> * extras)
{
    T_Status            status;
    T_Glyph_Ptr         saved, end, temp, temp2, powercheck, index_tier;
    T_Power_Info        power_info;

    /* See if we can find "[Multiclass X]" before the end of the first h1 tag.
        If we can, this is a special multiclass feat.
    */
    saved = ptr;
    end = strstr(ptr, "</h1>");
    if (end != NULL) {
        *end = '\0';
        ptr = Extract_Text(&info->descriptor, m_pool, ptr, checkpoint, "[", "]");
        if ((info->descriptor != NULL) && (strnicmp(info->descriptor, "Multiclass ", 11) == 0))
            info->multiclass = info->descriptor + 11;
        *end = '<';
        ptr = saved;
        }

    /* Skip past the "<h1 class" at the start of the record, so we don't
        mistake it for the start of a power later
    */
    ptr = strstr(ptr, "<h1 class");
    if (ptr == NULL)
        ptr = saved;
    else
        ptr += 11;

    /* We can't reliably get the tier, so do the best we can, then check for
        'Tier' within the text - if we don't find it, scrub it.
    */
    saved = ptr;
    index_tier = info->tier;
    ptr = Extract_Text(&info->tier, m_pool, ptr, checkpoint, "<b>", "</b>");
    if (strstr(info->tier, "Tier") == NULL) {
        info->tier = index_tier;
        ptr = saved;
        }

    T_Glyph_Ptr find_array[] = {"<b>Prerequisite</b>:"};
    T_Glyph_Ptr end_array[] = {"<br/>", "<br/>"};
    ptr = Extract_Text(&info->prerequisite, m_pool, ptr, checkpoint, find_array,
                        x_Array_Size(find_array), end_array, x_Array_Size(end_array));
    Strip_Bad_Characters(info->prerequisite);

    /* Before we go on to the rest of the description, skip a closing bold
        tag if we have it
    */
    if (strncmp(ptr, "</b>", 4) == 0)
        ptr += 4;

    /* If we have a closing header tag (without a matching opening header tag)
        before the checkpoint, skip past it, otherwise we'll include a lot of
        junk in the description
    */
    end = Find_Text(ptr, "</p>", checkpoint, false, false);
    temp = Find_Text(ptr, "<p>", checkpoint, false, false);
    if ((end == NULL) || ((temp != NULL) && (temp > end)))
        end = temp;
    if (end == NULL)
        end = checkpoint;
    *end = '\0';
    saved = ptr;
    temp2 = strstr(ptr, "<h1");
    ptr = strstr(ptr, "</h1>");
    if ((ptr == NULL) || ((temp2 != NULL) && (ptr > temp2)))
        ptr = saved;
    else
        ptr += 5;

    /* Now parse the rest of the description - grab text until the first
        opening or closing paragraph tag, whichever is later.
    */
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    *end = '<';

    /* Extract any powers that happen to be here
    */
    ptr = strstr(ptr, "<h1 class=\"");
    while (ptr != NULL) {
        memset(&power_info, 0, sizeof(power_info));

        /* Get the new checkpoint, which is the first "First published in..."
            section we find
        */
        powercheck = strstr(ptr, "<i>First published in");
        if (powercheck == NULL)
            powercheck = checkpoint;

        /* You can grab the name after the end of the first span
        */
        Extract_Text(&power_info.name, m_pool, ptr, powercheck, "</span>", "</h1>");
        Strip_Bad_Characters(power_info.name);
        power_info.forclass = "Feat";
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
