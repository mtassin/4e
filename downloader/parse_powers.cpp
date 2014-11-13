/*  FILE:   PARSE_POWERS.CPP

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


/*  Define subtypes for powers
*/
#define SUB_UTILITY     0
#define SUB_ATWILL      1
#define SUB_DAILY       2
#define SUB_ENCOUNTER   3
#define SUB_ENCSPECIAL  4

#define SUB_COUNT       (SUB_ENCSPECIAL + 1)


template<class T> C_DDI_Crawler<T> *    C_DDI_Crawler<T>::s_crawler = NULL;


C_DDI_Powers::C_DDI_Powers(C_Pool * pool) : C_DDI_Crawler("powers", "power", "Power",
                                                            3, 4, false, pool)
{
}


C_DDI_Powers::~C_DDI_Powers()
{
}


T_Int32U        C_DDI_Powers::Get_URL_Count(void)
{
    /* Normal tabs only have one type of thing we care about
    */
    return(SUB_COUNT);
}


void            C_DDI_Powers::Get_URL(T_Int32U index, T_Glyph_Ptr url,
                                T_Glyph_Ptr names[], T_Glyph_Ptr values[], T_Int32U * value_count)
{
    if (x_Trap_Opt(index >= Get_URL_Count())) {
        url[0] = '\0';
        return;
        }

    /* Prepare the URL to use and the common POST parameters
    */
    strcpy(url, INSIDER_URL "CompendiumSearch.asmx/KeywordSearchWithFilters");
    names[0] = "Tab";
    values[0] = m_tab_post_param;
    names[1] = "Keywords";
    values[1] = "null";
    names[2] = "Filters";
    switch (index) {
        case SUB_UTILITY :
            values[2] = "null|null|Utility|-1|-1|null|null";
            break;
        case SUB_ATWILL :
            values[2] = "null|null|null|-1|-1|null|At-Will";
            break;
        case SUB_DAILY :
            values[2] = "null|null|null|-1|-1|null|Daily";
            break;
        case SUB_ENCOUNTER :
            values[2] = "null|null|null|-1|-1|null|Encounter";
            break;
        case SUB_ENCSPECIAL :
            values[2] = "null|null|null|-1|-1|null|Encounter (Special)";
            break;
        default :
            x_Trap_Opt(1);
            break;
        }
    names[3] = "NameOnly";
    values[3] = "false";
    *value_count = 4;
}


T_Status        C_DDI_Powers::Parse_Index_Cell(T_XML_Node node, T_Power_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;
    T_Int32U        i, count;
    T_Power_Info *  compare;

    status = Get_Required_Child_PCDATA(&info->forclass, node, "ClassName");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->level, node, "Level");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->action, node, "ActionType");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    /* Based on our subtype, set the power type if it hasn't already been set
    */
    if (info->type == NULL) {

        /* For utility powers, just set our type to 'utility' - we'll set the
            power use later
        */
        if (subtype == SUB_UTILITY)
            info->type = "Utility";

        /* For all other powers, set the power use
        */
        else {
            info->type = "Autodetect";
            switch (subtype) {
                case SUB_ATWILL :
                    info->use = POWER_ATWILL;
                    break;
                case SUB_DAILY :
                    info->use = POWER_DAILY;
                    break;
                case SUB_ENCOUNTER :
                    info->use = POWER_ENCOUNTER;
                    break;
                case SUB_ENCSPECIAL :
                    info->use = POWER_ENCOUNTER;
                    break;
                }

            /* This power might be a utility power, and the only way to find
                out is to look up the list of powers and see if there's a
                utility power with the same name. If there is, set its "power
                use" text to whatever ours is, so it'll display properly. We
                don't need to worry about finding ourself, because we haven't
                been added to the list of powers yet.
            */
            count = m_list.size();
            for (i = 0; i < count; i++) {
                compare = &m_list[i];

                /* Make sure this is the same power by comparing the name, the
                    class, and making sure that the reference power is actually
                    a utility power.
                */
                if ((strcmp(info->name, compare->name) == 0) &&
                    (strcmp(info->forclass, compare->forclass) == 0) &&
                    ((compare->type != NULL) && (strcmp(compare->type, "Utility") == 0))) {
                    compare->use = info->use;

                    /* Also, set our 'duplicate' flag, so we know this power is
                        a duplicate and shouldn't be output.
                    */
                    info->is_duplicate = true;
                    break;
                    }
                }
            }
        }

    x_Status_Return_Success();
}


static T_Status     Parse_Power_Keywords(T_Glyph_Ptr text, T_Power_Info * info, C_Pool * pool)
{
    T_Glyph_Ptr     ptr, end;
    T_Glyph_Ptr     search[] = { "<b>", "</b>", };
    T_Glyph_Ptr     replace[] = { "", "", };
    T_Glyph_Ptr     dest;

    /* Skip to the first <b> tag in the text - if none, we have no keywords
    */
    ptr = strstr(text, "<b>");
    if (ptr == NULL)
        x_Status_Return_Success();

    /* Replace all bold start and end tags with the empty string
    */
    dest = new T_Glyph[strlen(ptr) + 1];
    if (x_Trap_Opt(dest == NULL))
        x_Status_Return(LWD_ERROR);
    dest[0] = '\0';
    Text_Find_Replace(dest, ptr, strlen(ptr) + 1, x_Array_Size(search), search, replace);
    ptr = dest;

    /* Grab every keyword from the text and stick it into our power info
    */
    while (*ptr != '\0') {

        /* Put this keyword into the power info
        */
        end = strchr(ptr, ',');
        if (end != NULL)
            *end = '\0';
        info->keywords[info->keyword_count++] = pool->Acquire(ptr);

        /* Skip to the start of the next keyword
        */
        if (end == NULL)
            break;
        ptr = end + 1;
        while (!x_Is_Alnum(*ptr) && (*ptr != '\0'))
            ptr++;
        }

    delete [] dest;
    x_Status_Return_Success();
}


static T_Status     Parse_Power_Range(T_Glyph_Ptr text, T_Power_Info * info, C_Pool * pool)
{
    T_Glyph_Ptr     search[] = { "<b>", "</b>", };
    T_Glyph_Ptr     replace[] = { "", "", };
    T_Glyph_Ptr     dest;

    /* Replace all bold start and end tags with the empty string
    */
    dest = new T_Glyph[strlen(text) + 1];
    if (x_Trap_Opt(dest == NULL))
        x_Status_Return(LWD_ERROR);
    dest[0] = '\0';
    Text_Find_Replace(dest, text, strlen(text) + 1, x_Array_Size(search), search, replace);

    /* Stick the range into our info structure, trimming off excess whitespace
    */
    info->range = pool->Acquire(dest);
    Text_Trim_Leading_Space(info->range);
    delete [] dest;

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Powers::Parse_Entry_Details(T_Glyph_Ptr contents, T_Power_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Power_Info> * extras)
{
    T_Status            status;
    T_Int32U            length, templength;
    T_Glyph_Ptr         end, saved, temp, index_action;
    T_Glyph_Ptr         starts[] = { "???", "???", "???", };
    T_Glyph_Ptr         ends[] = { "<br", "</p" };

    /* The class of the first h1 tag gives us the type of power it is.
    */
    ptr = Extract_Text(&info->use, m_pool, ptr, checkpoint, "<h1 class=\"", "\"");

    /* Move past the end of the h1 tag - next is a <span> tag. the PCDATA of
        this tag is something like "Wizard Attack 22" or "Fighter Utility 7".
    */
    ptr = Extract_Text(&info->type, m_pool, ptr, checkpoint, "<span class=\"level\">", "</span>");

    /* The flavor text is the PCDATA of the first <i> node after that - it
        might have <span class="flavor">[text]</span> after it, so strip those
        if it does.
    */
    ptr = Extract_Text(&info->flavor, m_pool, ptr, checkpoint, "<i>", "</i>");
    temp = "<span class=\"flavor\">";
    templength = strlen(temp);
    if (info->flavor != NULL) {
        if (strncmp(info->flavor, temp, templength) == 0) {
            info->flavor += strlen(temp);
            length = strlen(info->flavor);
            temp = "</span>";
            templength = strlen(temp);
            if ((length >= templength) && (strcmp(info->flavor + length - templength, temp) == 0))
                info->flavor[length-templength] = '\0';
            }
        Strip_Bad_Characters(info->flavor);
        }

    /* After that, there's an <img> element. Between that element and the next
        <br/> element lie the power keywords. If we don't have an <img element
        at all, or if it's past the checkpoint, that's fine - there are no
        keywords for this power.
    */
    saved = ptr;
    ptr = strstr(ptr, "<img");
    if ((ptr == NULL) || (ptr > checkpoint)) {

        /* Skip past the power type, which we don't care about - if we don't do
            this, it messes up searching for our action type, below
        */
//can we comment this out? If not, messes up Dwarven Resilience. I don't think
//we need this any more...
//        ptr = strstr(saved, "&nbsp;&nbsp;");
//        if (ptr == NULL)
            ptr = saved;
        }
    else {
        end = strstr(ptr, "<br");
        if (x_Trap_Opt(end == NULL))
            x_Status_Return(LWD_ERROR);
        *end = '\0';
        status = Parse_Power_Keywords(ptr, info, m_pool);
        if (x_Trap_Opt(!x_Is_Success(status)))
            x_Status_Return(status);
        *end = '<';
        }

    /* The action is in bold on the line right after that - it can be either
        after a br tag with a space, or without a space (thanks wizards). If we
        don't find either, just revert to whatever we parsed out of the index.
    */
    index_action = info->action;
    ptr = Extract_Text(&info->action, m_pool, ptr, checkpoint, "<br/><b>", "</b>");
    if (info->action == NULL)
        ptr = Extract_Text(&info->action, m_pool, ptr, checkpoint, "<br/><b>", "</b>");
    if ((info->action == NULL) || (info->action[0] == '\0'))
        info->action = index_action;

    /* The range is just after a "&nbsp;<b>" sequence; consume the tab, but not the
        bold, so we can key on it later
    */
    saved = ptr;
    ptr = strstr(ptr, "&nbsp;<b>");
    if (ptr == NULL)
        ptr = saved;
    else {
        ptr += 6;
        end = strstr(ptr, "<br");
        temp = strstr(ptr, "</br");
        if ((end == NULL) || ((temp != NULL) && (temp < end)))
            end = temp;
        temp = strstr(ptr, "</p");
        if ((end == NULL) || ((temp != NULL) && (temp < end)))
            end = temp;
        if (x_Trap_Opt(end == NULL))
            x_Status_Return(LWD_ERROR);
        *end = '\0';
        status = Parse_Power_Range(ptr, info, m_pool);
        Strip_Bad_Characters(info->range);
        if (x_Trap_Opt(!x_Is_Success(status)))
            x_Status_Return(status);
        *end = '<';
        }

    /* Check for our limits on the power - first, Channel Divinity
    */
    saved = ptr;
    ptr = strstr(ptr, "<b>Channel Divinity</b>");
    if (ptr == NULL)
        ptr = saved;
    else {
        ptr += 3;
        end = strstr(ptr, "</b>");
        if (x_Trap_Opt(end == NULL))
            x_Status_Return(LWD_ERROR);
        *end = '\0';
        info->limit = m_pool->Acquire(ptr);
        *end = '<';
        }

    /* Now the special text (there may not be one). Restore the saved pointer
        afterwards - the special text can be before or after the main
        description.
    */
    saved = ptr;
    starts[0] = "<b>Special</b>:";
    ptr = Extract_Text(&info->special, m_pool, ptr, checkpoint, starts, 1,
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->special);
    ptr = saved;

    starts[0] = "<b>Requirement</b>:";
    starts[1] = "<b>Requirements</b>:";
    ptr = Extract_Text(&info->requirement, m_pool, ptr, checkpoint, starts, 2,
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->requirement);

    starts[0] = "<b>Target</b>:";
    starts[1] = "<b>Targets</b>:";
    starts[2] = "<b>Primary Target</b>:";
    ptr = Extract_Text(&info->target, m_pool, ptr, checkpoint, starts, 3,
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->target);

    starts[0] = "<b>Attack</b>:";
    starts[1] = "<b>Primary Attack</b>:";
    ptr = Extract_Text(&info->attack, m_pool, ptr, checkpoint, starts, 2,
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->attack);

    /* Now the description, cutting off the 'first published' details and any
        trailing 'special' info.
    */
    *checkpoint = '\0';
    saved = ptr;
    ptr = strstr(saved, "<br");
    temp = strstr(saved, "<p class=\"powerstat\">");
    if (temp != NULL)
        temp += 21;
    if ((ptr == NULL) || ((temp != NULL) && (temp < ptr)))
        ptr = temp;
    temp = strstr(saved, "<p class=\"flavor\">");
    if (temp != NULL)
        temp += 18;
    if ((ptr == NULL) || ((temp != NULL) && (temp < ptr)))
        ptr = temp;
    if (ptr != NULL) {
        end = strstr(ptr, "<b>Special</b>:");
        if (end == NULL)
            end = strstr(ptr, "<i>First published in");
        if (end == NULL)
            end = strstr(ptr, "<p>Published in");
        if (end == NULL)
            end = strstr(ptr, "<p class=\"publishedIn\">Published in");
        if (end != NULL)
            *end = '\0';
        status = Parse_Description_Text(&info->description, ptr, m_pool, true);
        if (end != NULL)
            *end = '<';
        if (x_Trap_Opt(!x_Is_Success(status)))
            x_Status_Return(status);
        }
    *checkpoint = '<';

    x_Status_Return_Success();
}


/* For some reason if you try to call C_DDI_Powers::Get_Crawler()->Get_Item
    from within another file, like output_feats.cpp, xcode doesn't link
    properly in release mode. As a workaround we define a function here that
    does exactly the same thing, but presumably works because it's in this
    file with other references to C_DDI_Powers.
*/
T_Power_Info * Get_Power(T_Int32U index)
{
    return(C_DDI_Powers::Get_Crawler()->Get_Item(index));
}