/*  FILE:   PARSE_SKILLS.CPP

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


/* Skills are now on the Glossary tab for some bizarre reason
*/
C_DDI_Skills::C_DDI_Skills(C_Pool * pool) : C_DDI_Crawler("skills", "skill", "Glossary",
                                                            6, 2, false, pool)
{
}


C_DDI_Skills::~C_DDI_Skills()
{
}


void            C_DDI_Skills::Get_URL(T_Int32U index, T_Glyph_Ptr url,
                                T_Glyph_Ptr names[], T_Glyph_Ptr values[], T_Int32U * value_count)
{
    if (x_Trap_Opt(index >= Get_URL_Count())) {
        url[0] = '\0';
        return;
        }

    /* Prepare the URL to use and the POST parameters
    */
    strcpy(url, INSIDER_URL "CompendiumSearch.asmx/KeywordSearchWithFilters");
    names[0] = "Tab";
    values[0] = m_tab_post_param;
    names[1] = "Keywords";
    values[1] = "null";
    names[2] = "Filters";
    values[2] = "Skills|null|null";
    names[3] = "NameOnly";
    values[3] = "false";
    *value_count = 4;
}


T_Status        C_DDI_Skills::Parse_Index_Cell(T_XML_Node node, T_Skill_Info * info,
                                                T_Int32U subtype)
{
    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Skills::Parse_Entry_Details(T_Glyph_Ptr contents, T_Skill_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Skill_Info> * extras)
{
    T_Status            status;
    T_Glyph_Ptr         end, temp;

    /* The linked attribute for the skill starts within () after the h1 tag
    */
    T_Glyph_Ptr find_array[] = {"(", "<h1 class=\""};
    T_Glyph_Ptr end_array[] = {")"};
    ptr = Extract_Text(&info->attribute, m_pool, ptr, checkpoint, find_array,
                        x_Array_Size(find_array), end_array, x_Array_Size(end_array));

    /* The description text is the PCDATA of the first <p> node after that
    */
    ptr = Find_Text(ptr, "<p>", checkpoint, true, false);
    if (x_Trap_Opt(ptr == NULL))
        x_Status_Return_Success();

    /* If there's an armor check penalty, skip that part and set our 'armor
        check' flag
    */
    temp = Find_Text(ptr, "<i>Armor Check Penalty</i>", checkpoint, true, false);
    if (temp != NULL) {
        ptr = temp;
        info->armorcheck = true;
        }

    /* Now parse the rest of the description
    */
    end = Find_Text(ptr, "</p>", checkpoint, false, false);
    if (x_Trap_Opt(end == NULL))
        x_Status_Return_Success();
    *end = '\0';
    status = Parse_Description_Text(&info->description, ptr, m_pool);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
    *end = '<';

    x_Status_Return_Success();
}
