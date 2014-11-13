/*  FILE:   OUTPUT_FEATS.CPP

    Copyright (c) 2008-2009 by Lone Wolf Development, Inc.  All rights reserved.

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


static void Output_Descriptors(T_Feat_Info * info)
{
    T_Int32U            i, chunk_count;
    T_Glyph_Ptr         chunks[1000];
    T_Glyph             buffer[100000];

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->descriptor, ",");

    /* Find an appropriate tag for each chunk
    */
    for (i = 0; i < chunk_count; i++)
        Output_Tag(info->node, "ftDescript", chunks[i], info, "ftdescript", NULL, true, true);
}


T_Status    C_DDI_Feats::Output_Entry(T_XML_Node root, T_Feat_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph_Ptr     ptr;
    T_Glyph         feat_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Generate a unique id for the feat - if we can't, get out
    */
    if (!Generate_Thing_Id(feat_id, "ft", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for feat %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(feat_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Feat", info->name, feat_id, info->description,
                                UNIQUENESS_USER_ONCE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* Add this feat to our internal list of feats
    */
    Mapping_Add("feat", info->name, m_pool->Acquire(feat_id));

    if ((info->tier != NULL) && (info->tier[0] != '\0'))
        Output_Tag(info->node, "Tier", info->tier, info, "tier");
    else
        Output_Tag(info->node, "Tier", "Heroic", info, "tier");

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* If this is a multiclass feat, add a tag - if we don't find a class with
        the appropriate id, assume it's a weapon mastery feat
        NOTE: also check the fake class list, to handle things like the
        fighter/warlord that were converted to essentials classes)
    */
    if ((info->multiclass != NULL) && (info->multiclass[0] != '\0') &&
        (strcmp(info->multiclass, "Encounter") != 0) &&
        (strcmp(info->multiclass, "Daily") != 0) &&
        (strcmp(info->multiclass, "Utility") != 0)) {
        ptr = Mapping_Find("class", info->multiclass);
        if (ptr == NULL)
            ptr = Mapping_Find("fakeclass", info->multiclass);
        if (ptr != NULL)
            Output_Tag(info->node, "Multiclass", ptr, info);
        else
            Output_Tag(info->node, "Multiclass", "WepMastery", info);
        }

    /* If we have a non-multiclass descriptor, output that instead as a
        dynamic tag
    */
    else if ((info->descriptor != NULL) && (info->descriptor[0] != '\0'))
        Output_Descriptors(info);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


static void Output_Power_Mods(T_Feat_Info * info)
{
    T_Int32U            i, count, length;
    T_Glyph_Ptr         ptr, end, sep, name_ptr, temp, script_ptr;
    T_Power_Info *      power_info;
    T_Glyph             script[10000], buffer[1000];

    /* Scan the feat description for the telltale string we look for - if not
        present, this feat doesn't modify other powers
    */
    if (info->description == NULL)
        return;
    ptr = strstr(info->description, "You gain a benefit with any of the following");
    if (ptr == NULL)
        return;

    /* Skip to the next line break in the description
    */
    ptr = strstr(ptr, "{br}");
    if (x_Trap_Opt(ptr == NULL))
        return;

    /* Skip past any boilerplate text - after this we should be ready to start
        parsing out changes to individual powers
    */
    while (strncmp(ptr, "{br}", 4) == 0)
        ptr += 4;
    if (strncmp(ptr, "Associated Powers:", 18) == 0)
        ptr += 18;
    while (*ptr == ' ')
        ptr++;
    while (strncmp(ptr, "{br}", 4) == 0)
        ptr += 4;

    script[0] = '\0';
    script_ptr = script;

    end = strstr(ptr, "{br}");
    while (true) {

        /* Get the name of the power to look for (before the first colon)
        */
        sep = strchr(ptr, ':');
        if (x_Trap_Opt(sep == NULL))
            return;
        name_ptr = ptr;
        ptr = sep + 1;
        while (*ptr == ' ')
            ptr++;

        /* Skip to after any <a href="blahblah"> tag to skip the link to the
            power
        */
        temp = strstr(name_ptr, "\">");
        if ((temp != NULL) && (temp < sep)) {
            name_ptr = temp + 2;
            while (*name_ptr == ' ')
                name_ptr++;
            }

        /* If there's a class specified in ()s, skip to before it
        */
        temp = strchr(name_ptr, '(');
        if ((temp != NULL) && (temp < sep)) {
            sep = temp;
            while (sep[-1] == ' ')
                sep--;
            }

        /* Same for an ending </a> tag
        */
        temp = strstr(name_ptr, "</a>");
        if ((temp != NULL) && (temp < sep)) {
            sep = temp;
            while (sep[-1] == ' ')
                sep--;
            }

        /* Parse the name out into a buffer
        */
        length = sep - name_ptr;
        strncpy(buffer, name_ptr, length);
        buffer[length] = '\0';

        /* Search for a power with the given name, for the given class, with a
            a non-zero id (a zero id means something weird is going on, so we
            should just skip the power)
        */
        count = C_DDI_Powers::Get_Crawler()->Get_Item_Count();
        for (i = 0; i < count; i++) {
            power_info = Get_Power(i);
            if (x_Trap_Opt(power_info == NULL))
                continue;
            if ((stricmp(buffer, power_info->name) == 0) && (power_info->id != 0))
                break;
            }
        if (x_Trap_Opt(i >= count))
            goto next_power;

        /* Generate a line of script text
        */
        if (end != NULL)
            *end = '\0';
        sprintf(script_ptr, "\n      #thingdescappend[%s, \"{b}Addition by %s{/b}: %s\"]", power_info->id, info->name, ptr);
        script_ptr += strlen(script_ptr);
        if (end != NULL)
            *end = '{';

        /* Go to the next line to parse a new power
        */
next_power:
        if (end == NULL)
            break;
        ptr = end + 4;
        end = strstr(ptr, "{br}");

        /* If this line break is just after a colon, it's a mistake, like:
            Power Name (class):{br} blah blah text here
            So skip it and find the next one.
        */
        if ((end != NULL) && (end[-1] == ':'))
            end = strstr(end + 1, "{br}");
        }

    /* Finally, output the completed eval script
    */
    Output_Eval(info->node, script, "Setup", "1000", info);
}


T_Status    C_DDI_Feats::Post_Process_Entry(T_XML_Node root, T_Feat_Info * info)
{
    T_Int32U        i;
    T_Power_Info *  power_info;
    bool            is_powers = false;

    /* Now that our feats all have unique ids, output our prereqs - we need to
        add identity tags for feats that may not have been processed yet, so
        this must be done in the Post_Process stage
    */
    Output_Prereqs(info, &m_id_list);

    /* Now that all our powers have been output, we can scan our feat text for
        power modifications and apply them.
    */
    Output_Power_Mods(info);

    /* Add bootstraps for all linked powers, now that they've been output and
        have ids
    */
    for (i = 0; i < info->power_count; i++) {
        power_info = Get_Power(info->powers[i]);
        if (x_Trap_Opt(power_info == NULL))
            continue;
        Output_Bootstrap(info->node, power_info->id, info);
        is_powers = true;
        }

    /* If we had any powers to add, we should probably hide ourselves by
        default
    */
    if (is_powers)
        Output_Tag(info->node, "Hide", "Special", info);

    x_Status_Return_Success();
}