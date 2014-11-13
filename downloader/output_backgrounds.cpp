/*  FILE:   OUTPUT_BACKGROUNDS.CPP

    Copyright (c) 2008-2010 by Lone Wolf Development, Inc.  All rights reserved.

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


#define SKILL_SCRIPT    "#traitmodify[%s,trtBonus,%s,\"\"]\n"


static T_Glyph_Ptr      Get_Earliest(T_Glyph_Ptr pos1, T_Glyph_Ptr pos2)
{
    if (pos1 == NULL)
        return(pos2);
    if (pos2 == NULL)
        return(pos1);

    if (pos1 < pos2)
        return(pos1);
    return(pos2);
}


static void Output_Options(T_Background_Info * info, bool is_skill, T_List_Ids * id_list, C_Pool * pool)
{
    T_Glyph         backup;
    T_Glyph_Ptr     text, temp, end, table, search;
    bool            is_skip;
    T_Glyph         buffer[500];

    if (info->description == NULL)
        return;

    if (is_skill) {
        search = "Associated Skills: {/i}";
        table = "skill";
        }
    else {
        search = "Associated Languages: {/i}";
        table = "language";
        }

    text = strstr(info->description, search);
    if (text == NULL)
        return;
    text += strlen(search);
    while (x_Is_Space(*text))
        text++;

    /* Parse out the next skill name
    */
    while (true) {
        end = strchr(text, ',');
        end = Get_Earliest(end, strchr(text, '.'));
        end = Get_Earliest(end, strchr(text, '{'));
        if (end != NULL) {
            backup = *end;
            *end = '\0';
            }


        /* Note that Output_Language takes care of checking whether the
            language exists for us
        */
        if (!is_skill) {
            is_skip = (Mapping_Find("bglangskip", text, false, false) != NULL);
            if (!is_skip)
                Output_Language(info->node, text, info, id_list, pool, "User", "BackLang");
            }
        else {
            temp = Mapping_Find(table, text, false, true);
            if (temp != NULL)
                Output_Tag(info->node, "BackSkill", temp, info);
            else {
                sprintf(buffer, "Skill not found for backgrounds: %s\n", text);
                Log_Message(buffer);
                }
            }

        if (end != NULL)
            *end = backup;

        if ((end == NULL) || (*end == '{') || (*end == '\0'))
            return;

        /* Skip over spaces, commas, etc, to next skill
        */
        text = end + 1;
        while (x_Is_Space(*text) || x_Is_Punct(*text))
            text++;

        /* If we're at the end of the string, or if we hit a newline, we're
            done
        */
        if ((*text == '\0') || (*text == '{'))
            return;
        }
}


T_Status    C_DDI_Backgrounds::Output_Entry(T_XML_Node root, T_Background_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph         background_id[UNIQUE_ID_LENGTH+1], buffer[10000];

    /* Generate a unique id for the background - if we can't, get out
    */
    if (!Generate_Thing_Id(background_id, "bg", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for background %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(background_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Background", info->name, background_id, info->description,
                                UNIQUENESS_UNIQUE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    if ((info->type != NULL) && (info->type[0] != '\0'))
        Output_Tag(info->node, "BackType", info->type, info, "backtype", NULL, true, true);
    if ((info->campaign != NULL) && (info->campaign[0] != '\0'))
        Output_Tag(info->node, "BackCamp", info->campaign, info, "backcamp", NULL, true, true);

    Output_Options(info, true, &m_id_list, m_pool);
    Output_Options(info, false, &m_id_list, m_pool);

    /* Add this path to our internal list of backgrounds
    */
    Mapping_Add("background", info->name, m_pool->Acquire(background_id));

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


static T_Glyph_Ptr  First_Word_Before(T_Glyph_Ptr buffer, T_Glyph_Ptr end, T_Glyph_Ptr limit)
{
    T_Glyph_Ptr     ptr;
    T_Glyph         backup;

    while (end[-1] == ' ')
        end--;

    /* Get to the start of the word, ignoring stuff like spaces, + signs, etc
    */
    ptr = end;
    while ((ptr[-1] != ' ') && (ptr[-1] != '+') && (ptr >= limit))
        ptr--;

    backup = *end;
    *end = '\0';
    strcpy(buffer, ptr);
    *end = backup;
    return(ptr);
}


static T_Glyph_Ptr  First_Word_After(T_Glyph_Ptr buffer, T_Glyph_Ptr start)
{
    T_Glyph_Ptr     ptr;
    T_Glyph         backup;

    /* Skip past any spaces until we get to a real character, then save our
        start position and read the word
    */
    ptr = start;
    while ((*ptr == ' ') && (*ptr != '\0'))
        ptr++;
    start = ptr;
    while (!isspace(*ptr) && !ispunct(*ptr) && (*ptr != '\0'))
        ptr++;

    backup = *ptr;
    *ptr = '\0';
    strcpy(buffer, start);
    *ptr = backup;
    return(ptr);
}


static void Generate_Mechanics(T_Background_Info * info, T_List_Ids * id_list, C_Pool * pool)
{
    T_Glyph_Ptr         ptr, temp, temp2, script;
    bool                is_hidden = true;
    T_Glyph             buffer[500], buffer2[500], buffer3[500];

    if ((info->benefit == NULL) || (info->benefit[0] == '\0'))
        goto done;

    /* Search for a series of strings and do stuff based on them - first,
        simple bonus languages
    */
    ptr = strstr(info->benefit, "know one additional language");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "gain one additional language");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "Learn an extra language");
    if (ptr != NULL)
        Output_Field(info->node, "bgLang", "1", info);

    /* Second, more complex things, like adding specific languages
    */
    ptr = strstr(info->benefit, "to your list of languages");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "as an additional language");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "to your list of known languages");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "to your list of  known languages");
    if (ptr != NULL) {
        First_Word_Before(buffer, ptr, info->benefit);
        if (strcmp(buffer, "Speech") == 0)
            strcpy(buffer, "Deep Speech");
        if (buffer[0] != '\0')
            Output_Language(info->node, buffer, info, id_list, pool);
        }

    /* Initial health
    */
    ptr = strstr(info->benefit, "substitute your highest ability score for Constitution to determine your initial hit points");
    if (ptr != NULL)
        Output_Eval(info->node, "perform hero.child[trHealth].assign[Helper.BestAbil]", "Traits", "1000", info, "Class HP final");

    /* Class skills
    */
    ptr = strstr(info->benefit, "skill to your class skill");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "to your class skill");
    if (ptr != NULL) {
        temp2 = First_Word_Before(buffer, ptr, info->benefit);
        if (buffer[0] != '\0')
            Output_Tag(info->node, "Skill", buffer, info, "skill");

        /* If the word before that was "and", add another skill
        */
        temp2 = First_Word_Before(buffer, temp2, info->benefit);
        if (strcmp(buffer, "and") == 0) {
            First_Word_Before(buffer, temp2, info->benefit);
            if (buffer[0] != '\0')
                Output_Tag(info->node, "Skill", buffer, info, "skill");
            }
        }

    /* Initiative
    */
    ptr = strstr(info->benefit, "bonus on initiative checks");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "bonus to initiative checks");
    if (ptr != NULL) {
        First_Word_Before(buffer, ptr, info->benefit);
        if (buffer[0] != '\0')
            Output_Field(info->node, "bgInit", buffer, info);
        }

    /* Proficiencies
    */
    ptr = strstr(info->benefit, "gain proficiency in a simple or military weapon");
    if (ptr != NULL)
        Output_Bootstrap(info->node, "fBgSmWep", info);

    /* Uses - if we have either of these we don't want to hide our ability
    */
    ptr = strstr(info->benefit, "nce per encounter");
    if (ptr != NULL) {
        Output_Tag(info->node, "PowerUse", "Encounter", info);
        is_hidden = false;
        }
    ptr = strstr(info->benefit, "nce per day");
    if (ptr != NULL) {
        Output_Tag(info->node, "PowerUse", "Daily", info);
        is_hidden = false;
        }

    /* Anything with "reroll the check" or other language indicating that this
        is an interesting ability must be not hidden
    */
    ptr = strstr(info->benefit, "reroll the check");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "bonus to saving throw");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "spend an action");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "skill challenge");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "keep the second result");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "death saves");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "construct your own");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "knowledge check");
    if (ptr == NULL)
        ptr = strstr(info->benefit, "damage rolls");
    if (ptr != NULL)
        is_hidden = false;

    /* Skill bonus - parse as "X bonus to [all|your] <skill name>", but make
        sure we're not trying to parse an initiative or speed bonus by accident
    */
    ptr = strstr(info->benefit, "bonus to ");
    if ((ptr != NULL) && (strncmp(ptr+9, "initiative", 10) == 0))
        ptr = strstr(ptr+1, "bonus to ");
    if ((ptr != NULL) && (strncmp(ptr+9, "speed", 5) == 0))
        ptr = strstr(ptr+1, "bonus to ");
    if (ptr == NULL)
        goto done;

    /* Get our bonus number
    */
    First_Word_Before(buffer, ptr, info->benefit);
    ptr += 9;
    if (strncmp(ptr, "all ", 4) == 0)
        ptr += 4;
    if (strncmp(ptr, "your ", 5) == 0)
        ptr += 5;
    ptr = First_Word_After(buffer2, ptr);
    if (buffer2[0] == '\0')
        goto done;
    temp = Mapping_Find("skill", buffer2);
    if (temp == NULL)
        goto done;
    sprintf(buffer3, SKILL_SCRIPT, temp, buffer);
    script = buffer3 + strlen(buffer3);

    /* We're looking at skills, so make sure it wasn't qualified earlier with
        "While doing X thing"
    */
    if ((strstr(info->benefit, "while ") != NULL) || (strstr(info->benefit, "While ") != NULL)) {
        is_hidden = false;
        goto done;
        }

    /* If the next word is "and", parse out another skill bonus (skip the word
        checks, which is sometimes present)
    */
    while (isspace(*ptr))
        ptr++;
    if (strncmp(ptr, "checks ", 7) == 0)
        ptr += 7;
    if (strncmp(ptr, "and ", 4) != 0)
        goto output_script;
    ptr += 4;

    /* The next word will be another skill name, so look it up
    */
    ptr = First_Word_After(buffer2, ptr);
    if (buffer2[0] == '\0')
        goto output_script;
    temp = Mapping_Find("skill", buffer2);
    if (temp == NULL)
        goto output_script;
    sprintf(script, SKILL_SCRIPT, temp, buffer);
    script += strlen(script);

    /* If the next words are "checks when" there's a condition, so we don't add
        a script
    */
output_script:
    while (isspace(*ptr))
        ptr++;
    if (strncmp(ptr, "checks when", 11) == 0) {
        is_hidden = false;
        goto done;
        }
    if (strncmp(ptr, "made due to", 11) == 0) {
        is_hidden = false;
        goto done;
        }
    if (strncmp(ptr, "made to", 7) == 0) {
        is_hidden = false;
        goto done;
        }
    Output_Eval(info->node, buffer3, "Traits", "1000", info, "Derived trtFinal");

    /* Backgrounds should be hidden by default, only a few need to be shown
    */
done:
    if (is_hidden)
        Output_Tag(info->node, "Hide", "Special", info);
}


T_Status    C_DDI_Backgrounds::Post_Process_Entry(T_XML_Node root, T_Background_Info * info)
{
    /* Now that our feats all have unique ids, output our prereqs - we need to
        add identity tags for feats that may not have been processed yet, so
        this must be done in the Post_Process stage
    */
    Output_Prereqs(info, &m_id_list);

    /* Generate scripts / tags / fields / etc for everything
    */
    Generate_Mechanics(info, &m_id_list, m_pool);

    x_Status_Return_Success();
}