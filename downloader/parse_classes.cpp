/*  FILE:   PARSE_CLASSES.CPP

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


C_DDI_Classes::C_DDI_Classes(C_Pool * pool) : C_DDI_Crawler("classes", "class", "Class",
                                                            2, 4, false, pool)
{
}


C_DDI_Classes::~C_DDI_Classes()
{
}


T_Status        C_DDI_Classes::Parse_Index_Cell(T_XML_Node node, T_Class_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;
    T_Glyph_Ptr     start, end, origname;

    /* If our name has parentheses in it like "Cleric (Templar)", change it to
        just the name in the parens to keep it consistent with how we used to
        do stuff
        NOTE: Don't do this for hybrid classes, as it causes problems for stuff
            like "Hybrid Paladin (Cavalier)"
    */
    start = strchr(info->name, '(');
    end = strchr(info->name, ')');
    if ((start != NULL) && (end != NULL) && (strncmp(info->name, "Hybrid", 6) != 0)) {
        origname = info->name;
        info->name = start + 1;
        *end = '\0';

        /* Keep a note of our original name for later
        */
        while (x_Is_Space(start[-1]))
            start--;
        *start = '\0';
        info->original_name = origname;
        }

    /* Parse other misc data
    */
    status = Get_Required_Child_PCDATA(&info->role, node, "RoleName");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->keyabilities, node, "KeyAbilities");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Classes::Parse_Entry_Details(T_Glyph_Ptr contents, T_Class_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Class_Info> * extras)
{
    T_Status            status;
    T_Glyph             backup;
    T_Glyph_Ptr         end, powercheck, temp;
    T_Power_Info        power_info;

    /* The class of the first h1 tag gives us the type of class it is.
    */
    ptr = Extract_Text(&info->type, m_pool, ptr, checkpoint, "<h1 class=\"", "\"");

    /* The flavor text is the PCDATA of the first <i> node after that
        Never mind... flavor text doesn't seem to be included any more...
    */
//    ptr = Extract_Text(&info->flavor, m_pool, ptr, checkpoint, "<i>", "</i>");
//    Strip_Bad_Characters(info->flavor);
    info->flavor = "";

    /* This was parsed from the index above, but we might as well grab it again
        here, in case there's more information
    */
    ptr = Extract_Text(&info->role, m_pool, ptr, checkpoint, "<b>Role: </b>", "<br");
    Strip_Bad_Characters(info->role);

    ptr = Extract_Text(&info->powersource, m_pool, ptr, checkpoint, "<b>Power Source: </b>", "<br");
    Strip_Bad_Characters(info->powersource);

    /* This was parsed from the index above, but we might as well grab it again
        here, in case there's more information
    */
    ptr = Extract_Text(&info->keyabilities, m_pool, ptr, checkpoint, "<b>Key Abilities: </b>", "<br");

    ptr = Extract_Text(&info->armorprofs, m_pool, ptr, checkpoint, "<b>Armor Proficiencies: </b>", "<br");

    ptr = Extract_Text(&info->weaponprofs, m_pool, ptr, checkpoint, "<b>Weapon Proficiencies: </b>", "<br");

    ptr = Extract_Text(&info->implement, m_pool, ptr, checkpoint, "<b>Implement: </b>", "<br");

    ptr = Extract_Text(&info->defbonuses, m_pool, ptr, checkpoint, "<b>Bonus to Defense: </b>", "<br");

    ptr = Extract_Number(&info->hp_level1, m_pool, ptr, checkpoint, "<b>Hit Points at 1st Level");

    ptr = Extract_Number(&info->hp_perlevel, m_pool, ptr, checkpoint, "<b>Hit Points per Level");

    ptr = Extract_Number(&info->surges, m_pool, ptr, checkpoint, "<b>Healing Surges");

    ptr = Extract_Text(&info->skl_trained, m_pool, ptr, checkpoint, "<b>Trained Skills</b>:", "<br");

    T_Glyph_Ptr find_array[] = {"<i>Class Skills</i>:", "<b>Class Skills</b>:"};
    T_Glyph_Ptr end_array[] = {"<br"};
    ptr = Extract_Text(&info->skl_class, m_pool, ptr, checkpoint, find_array,
                        x_Array_Size(find_array), end_array, x_Array_Size(end_array));

    ptr = Extract_Text(&info->features, m_pool, ptr, checkpoint, "<b>Class features: </b>", "<br/>");

    ptr = Extract_Text(&info->hybrid_talents, m_pool, ptr, checkpoint, "<b>Hybrid Talent Options</b>: ", "<br/>");

    /* Skip past any line breaks to the next point
    */
    while (strncmp(ptr, "<br/>", 5) == 0)
        ptr += 5;

    /* Description comes after the end of the class features
    */
    end = Find_Text(ptr,"<br/>",checkpoint,false,false);
    if (end == NULL)
        x_Status_Return_Success();
    *end = '\0';
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    *end = '<';

    /* General details text we pull class features out of
    */
    backup = *checkpoint;
    *checkpoint = '\0';
    info->details = m_pool->Acquire(ptr);
    Strip_Bad_Characters(info->details);
    *checkpoint = backup;

    /* If we have a power span, extract the power and all following powers -
        note that most powers are already present in the powers list, so we
        don't often need to do this! We only output powers that are specifically
        missing in our list.
    */
    ptr = strstr(ptr, "<h1 class=\"");
    while ((ptr != NULL) && (ptr < checkpoint)) {
        memset(&power_info, 0, sizeof(power_info));

        /* Get the new checkpoint, which is the start of the next power or the
            first "Published in..." section we find
        */
        powercheck = strstr(ptr+1, "<h1");
        temp = strstr(ptr, "</p></p><br/>");
        if ((powercheck == NULL) || ((temp != NULL) && (temp < powercheck)))
            powercheck = temp;
        if (powercheck == NULL)
            powercheck = strstr(ptr, "<p>Published in");
        if (powercheck == NULL)
            powercheck = checkpoint;

        /* You can grab the name after the end of the first span
        */
        Extract_Text(&power_info.name, m_pool, ptr, powercheck, "</span>", "</h1>");
        Strip_Bad_Characters(power_info.name);
        power_info.forclass = info->name;
        power_info.parent_info = info;

        /* Parse things to extract the rest of the data
        */
        C_DDI_Powers::Get_Crawler()->Parse_Entry_Details(ptr, &power_info, ptr, powercheck, NULL);

        /* If this power's name is in the 'class feature' list, change the power
            type to 'Feature'
        */
        if (strstr(info->features, power_info.name) != NULL)
            power_info.type = "Feature";
        else
            power_info.type = "Build Option";

        /* Add the power to our 'possible powers' list - it gets added to the
            list of regular powers only if it wouldn't be a duplicate of
            something already there
        */
        C_DDI_Powers::Get_Crawler()->Append_Potential_Item(&power_info);

        /* Try to find following powers!
        */
        ptr = strstr(powercheck, "<h1 class=\"");
        }

    x_Status_Return_Success();
}


