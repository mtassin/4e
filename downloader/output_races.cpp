/*  FILE:   OUTPUT_RACES.CPP

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


/*  Include our template class definition - see the comment at the top of the
    crawler.h file for details. Summary: C++ templates are terrible.
*/
#include "crawler.h"


static void Output_Height_Weight(T_Race_Info * info)
{
    T_Int32U            chunk_count, value;
    T_Glyph_Ptr         chunks[1000], chunks2[1000];
    T_Glyph             buffer[100000], buffer2[1000];

    if (info->height != NULL) {
        chunk_count = 2;
        Split_To_Array(buffer, chunks, &chunk_count, info->height, "-");
        if (chunk_count < 2) {
            sprintf(buffer, "Couldn't extract height for race %s\n", info->name);
            Log_Message(buffer);
            return;
            }

        /* Now split our minimum to get the feet and inches, and output the
            total field value
        */
        chunk_count = 2;
        Split_To_Array(buffer2, chunks2, &chunk_count, chunks[0], "'");
        if (chunk_count < 2) {
            sprintf(buffer, "Couldn't extract minimum height for race %s\n", info->name);
            Log_Message(buffer);
            return;
            }
        value = atoi(chunks2[0]) * 12 + atoi(chunks2[1]);
        Output_Field(info->node, "racHtMin", As_Int(value), info);

        /* Same with our maximum to get the feet and inches, and output the
            total field value
        */
        chunk_count = 2;
        Split_To_Array(buffer2, chunks2, &chunk_count, chunks[1], "'");
        if (chunk_count < 2) {
            sprintf(buffer, "Couldn't extract maximum height for race %s\n", info->name);
            Log_Message(buffer);
            return;
            }
        value = atoi(chunks2[0]) * 12 + atoi(chunks2[1]);
        Output_Field(info->node, "racHtMax", As_Int(value), info);
        }

    if (info->weight != NULL) {
        chunk_count = 2;
        Split_To_Array(buffer, chunks, &chunk_count, info->weight, "-");
        if (chunk_count < 2) {
            sprintf(buffer, "Couldn't extract weight for race %s\n", info->name);
            Log_Message(buffer);
            return;
            }
        Output_Field(info->node, "racWtMin", chunks[0], info);
        Output_Field(info->node, "racWtMax", chunks[1], info);
        }

}


static void Output_Ability_Scores(T_Race_Info * info)
{
    T_Int32U            i, chunk_count;
    T_Int32S            modifier;
    T_Glyph_Ptr         ptr, end, ptr2, temp, temp2, chunks[1000];
    T_Glyph             buffer[100000], message[1000];

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->abilityscores, ",.");
    for (i = 0; i < chunk_count; i++) {

        /* Parse the value out of the string, then skip to the first space
        */
        modifier = atoi(chunks[i]);
        ptr = chunks[i];
        while ((*ptr != ' ') && (*ptr != '\0'))
            ptr++;
        while (*ptr == ' ')
            ptr++;

        /* Now look up the attribute to work out which field we modify on the race
        */
        temp = Mapping_Find("raceattr", ptr);
        if (temp != NULL) {
            Output_Field(info->node, temp, As_Int(modifier), info);
            continue;
            }

        /* If we couldn't find what we were looking for, it might be a
            "+2 X or +2 Y" ability, so try to parse that (except for the
            changeling, which handles it manually)
        */
        if (strcmp(info->name, "Changeling") == 0)
            continue;
        end = strchr(ptr, ' ');
        if (x_Trap_Opt(end == NULL))
            goto error;
        *end = '\0';
        temp = Mapping_Find("attr", ptr);
        if (temp == NULL)
            goto error;
        ptr2 = end + 7;
        temp2 = Mapping_Find("attr", ptr2);
        if (temp2 == NULL)
            goto error;
        sprintf(message, "component.Attribute & (thingid.%s | thingid.%s)", temp, temp2);
        Output_Bootstrap(info->node, "fRPlusAb", info, NULL, NULL, NULL, NULL, NULL,
                            NULL, NULL, "RaceBonus", temp, "RaceBonus", temp2);
        continue;

error:
        sprintf(message, "Couldn't extract ability score '%s' for race %s\n", ptr, info->name);
        Log_Message(message);
        }
}


static void Output_Languages(T_Race_Info * info, T_List_Ids * id_list, C_Pool * pool)
{
    T_Int32U            i, chunk_count, extra_langs;
    T_Glyph_Ptr         chunks[1000];
    T_Glyph             buffer[100000];

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->languages, ",.");
    extra_langs = 0;
    for (i = 0; i < chunk_count; i++) {

        /* If this entry has 'choice' or 'either' in it, it's a choice of
            language
        */
        if ((strstr(chunks[i], "Choice") != NULL) || (strstr(chunks[i], "choice") != NULL) ||
            (strstr(chunks[i], "Either") != NULL) || (strstr(chunks[i], "either") != NULL)) {
            if (strstr(chunks[i], "of three") != NULL)
                extra_langs += 3;
            else if (strstr(chunks[i], "of two") != NULL)
                extra_langs += 2;
            else
                extra_langs++;
            continue;
            }

        /* Otherwise, add the specific language
        */
        Output_Language(info->node, chunks[i], info, id_list, pool);
        }

    /* If we have a non-zero number of extra languages, output a field for that
    */
    if (extra_langs != 0)
        Output_Field(info->node, "racLang", As_Int(extra_langs), info);
}


static void Output_Skill_Bonuses(T_Race_Info * info)
{
    T_Int32U            i, chunk_count;
    T_Int32S            modifier;
    T_Glyph_Ptr         ptr, temp, chunks[1000];
    T_Glyph             buffer[100000], message[1000];

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->skillbonuses, ",.");
    for (i = 0; i < chunk_count; i++) {

        /* Parse the value out of the string, then skip to the first space
        */
        modifier = atoi(chunks[i]);
        ptr = chunks[i];
        while ((*ptr != ' ') && (*ptr != '\0'))
            ptr++;
        while (*ptr == ' ')
            ptr++;

        /* If we don't have a +2 bonus, that's a problem
        */
        if (modifier != 2) {
            sprintf(message, "Skill bonus for skill '%s' wasn't +2 for race %s\n", ptr, info->name);
            Log_Message(message);
            }

        /* Now look up the attribute to work out which field we modify on the race
        */
        temp = Mapping_Find("skill", ptr);
        if (temp != NULL)
            Output_Tag(info->node, "SkillBonus", temp, info);
        else {
            sprintf(message, "Couldn't extract skill '%s' for race %s\n", ptr, info->name);
            Log_Message(message);
            }
        }
}


T_Status    C_DDI_Races::Output_Entry(T_XML_Node root, T_Race_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    bool            is_copy;
    T_Glyph         race_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Generate a unique id for the race - if we can't, get out
    */
    if (!Generate_Thing_Id(race_id, "r", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for race %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(race_id);

    /* Add this race to our internal list of races
    */
    Mapping_Add("race", info->name, m_pool->Acquire(race_id));

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Race", info->name, race_id, info->description,
                                UNIQUENESS_UNIQUE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* If this is a copy, we'll fill it in later
    */
    is_copy = Mapping_Find("racecopies", info->name) != NULL;
    if (is_copy) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    if ((info->flavor != NULL) && (info->flavor[0] != '\0'))
        Output_Field(info->node, "racFlavor", info->flavor, info);

    Output_Ability_Scores(info);

    Output_Tag(info->node, "Size", info->size, info, "size");

    Output_Tag(info->node, "User", "StdRace", info);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    Output_Height_Weight(info);

    Output_Field(info->node, "racSpeed", As_Int(atoi(info->speed)), info);

    Output_Languages(info, &m_id_list, m_pool);

    if (stricmp(info->vision, "Normal") != 0)
        Output_Tag(info->node, "Vision", info->vision, info, "vision");

    Output_Skill_Bonuses(info);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


static bool Is_Power_Bootstrapped(T_Race_Info * info, T_Glyph_Ptr name)
{
    T_Int32U        i;
    T_Power_Info *  power_info;

    /* Return true if a power with the specified name is linked from this race.
    */
    for (i = 0; i < info->power_count; i++) {
        power_info = C_DDI_Powers::Get_Crawler()->Get_Item(info->powers[i]);
        if (x_Trap_Opt(power_info == NULL))
            continue;
        if (stricmp(power_info->name, name) == 0)
            return(true);
        }

    return(false);
}


static void Parse_Racial_Abilities(T_XML_Node root, T_Race_Info * info, C_Pool * pool,
                                    T_List_Ids * id_list)
{
    long            result;
    T_Int32U        length;
    T_Glyph_Ptr     ptr, name, descstart, descend;
    T_XML_Node      feature;
    bool            is_new;
    T_Glyph         buffer[500], id_buffer[UNIQUE_ID_LENGTH+1];

    if ((info->description == NULL) || (info->description[0] == '\0'))
        return;

    /* Find sets of "{b}Name{/b}: Description{br}" (the ending {br} is optional)
        in the description text
        NOTE: OK, some have appeared that look like "{b}Cast-Iron Mind: {/b}",
            and in those cases the description should end after the second
            line break after... these are referred to as the "new kind" below.
    */
    name = NULL;
    ptr = Extract_Text(&name, pool, info->description, NULL, "{b}", "{/b}");
    while ((name != NULL) && (ptr != NULL)) {

        /* Figure out if we're dealing with old or new style for this one
        */
        is_new = false;
        length = strlen(name);
        if (name[length-1] == ':') {
            is_new = true;
            name[length-1] = '\0';
            }

        /* First, see if there's a power already bootstrapped with the same
            name - if so, we can skip this feature, since all it is is "You can
            use <power X>."
        */
        if (Is_Power_Bootstrapped(info, name))
            goto next_feature;

        /* Skip past the closing {/b} tag, and any ':' character
        */
        ptr += 4;
        while ((*ptr == ':') || x_Is_Space(*ptr))
            ptr++;

        /* Find the description text - it's the text up until the end of the
            string, or the next {br} - and make sure it's null-terminated.
            NOTE: New-style means go to the end of the second line break.
        */
        descstart = ptr;
        descend = strstr(descstart, "{br}");
        if (is_new)
            descend = strstr(descend+1, "{br}");
        if (descend != NULL)
            *descend = '\0';

        /* Generate a unique id for this ability
            NOTE: We don't want it to try make up clever ids for race features
                because that messes things up for some reason, so specify false
                as the last parameter
        */
        sprintf(buffer, "fR%.2s", info->name);
        if (!Generate_Thing_Id(id_buffer, buffer, name, id_list, 3, NULL, false)) {
            sprintf(buffer, "Error generating unique id for race feature %s\n", name);
            Log_Message(buffer);
            goto next_feature;
            }

        /* Create a node for the ability
        */
        result = Create_Thing_Node(root, "race feature", "RaceFeat", name, id_buffer, descstart,
                            UNIQUENESS_NONE, NULL, &feature, info->is_partial);
        if (result != 0)
            goto next_feature;

        /* Bootstrap the node into the race node
        */
        result = Output_Bootstrap(info->node, id_buffer, info);
        if (x_Trap_Opt(result != 0))
            return;

        /* If our name is 'Oversized', add a script that adds the Oversized tag
            to the hero
        */
        if (strcmp(name, "Oversized") == 0)
            Output_Eval(feature, "perform hero.assign[Hero.Oversized]", "Setup", "10000", info);

        /* If we didn't have any more description text, we're done. Otherwise,
            un-terminate it.
        */
        if (descend == NULL)
            break;
        *descend = '{';

        /* Find the next match
        */
next_feature:
        name = NULL;
        ptr = Extract_Text(&name, pool, ptr, NULL, "{b}", "{/b}");
        }
}


T_Status    C_DDI_Races::Post_Process_Entry(T_XML_Node root, T_Race_Info * info)
{
    long            result;
    T_Int32U        i;
    T_Power_Info *  power_info;
    T_Race_Info *   source;
    T_Glyph_Ptr     copy_from;

    /* Add bootstraps for all linked powers, now that they've been output and
        have ids
    */
    for (i = 0; i < info->power_count; i++) {
        power_info = C_DDI_Powers::Get_Crawler()->Get_Item(info->powers[i]);
        if (x_Trap_Opt(power_info == NULL))
            continue;
        Output_Bootstrap(info->node, power_info->id, info);
        }

    /* Now parse racial abilities out of the description text - we have to wait
        to do this until after the powers are added, so as not to get duplicates
    */
    Parse_Racial_Abilities(root, info, m_pool, &m_id_list);

    /* Find the race to copy from, if any
        NOTE: This assumes that the race to copy from was defined before us,
            but that may not be true in future - if it's not, this will need
            to happen after the post-process step for races...
    */
    copy_from = Mapping_Find("racecopies", info->name);
    if (copy_from == NULL)
        x_Status_Return_Success();
    source = Find_Item(copy_from);
    if (x_Trap_Opt(source == NULL))
        x_Status_Return_Success();

    /* Duplicate the source node onto us, without overwriting any differences
    */
    result = XML_Duplicate_Node(source->node, &info->node, false);
    if (x_Trap_Opt(result != 0))
        Log_Message("Could not duplicate old race onto new.");

    /* Add a tag to indicate we count as the same race as the old one
    */
    Output_Tag(info->node, "Race", source->id, info);

    x_Status_Return_Success();
}