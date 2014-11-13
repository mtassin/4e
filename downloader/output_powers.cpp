/*  FILE:   OUTPUT_POWERS.CPP

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


#include <string>


#include "private.h"


static T_Int32U Output_Power_Special(T_XML_Node node, T_Power_Info * info)
{
    T_Int32S        result;
    T_XML_Node      child, timing;

    if (strstr(info->special, "You can use this power twice per encounter")) {
        result = Output_Field(node, "spcMax", "2", info);
        if (x_Trap_Opt(result != 0))
            return(result);
        }

    if (strstr(info->special, "At 16th level, you can use this power three times per encounter")) {
        result = XML_Create_Child(node, "eval", &child);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new eval script node.");
            return(result);
            }
        XML_Write_Text_Attribute(child, "value", "1");
        XML_Write_Text_Attribute(child, "phase", "Traits");
        XML_Write_Text_Attribute(child, "priority", "1000");
        XML_Set_PCDATA(child,
        "if (hero.tagvalue[Level.?] >= 16) then\n" \
        "  field[spcMax].value += 1\n" \
        "  endif");
        result = XML_Create_Child(child, "after", &timing);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new timing node.");
            return(result);
            }
        XML_Write_Text_Attribute(timing, "name", "Level final");
        }

    return(0);
}


static void Consume_Characters(T_Glyph_Ptr * ptr, T_Glyph consume)
{
    T_Glyph_Ptr     temp;

    temp = *ptr;
    while ((*temp != '\0') && (*temp == consume))
        *temp++;
    *ptr = temp;
}


static void Consume_Characters(T_Glyph_Ptr * ptr, T_Glyph_Ptr consume)
{
    T_Glyph_Ptr     temp;

    temp = *ptr;
    while ((*temp != '\0') && (strchr(consume, *temp) != NULL))
        *temp++;
    *ptr = temp;
}


enum E_Power_Damage {
    e_damage_fixed,
    e_damage_weapon,
    e_damage_beast
};


static T_Glyph_Ptr  Parse_Damage(T_Glyph_Ptr fixed, T_Glyph_Ptr multiplier,
                                    T_Glyph_Ptr text, E_Power_Damage * type)
{
    T_Glyph_Ptr     end, followend;

    fixed[0] = '\0';
    multiplier[0] = '\0';
    end = strchr(text, ' ');
    if (end == NULL)
        return(text);
    *end = '\0';

    /* If the damage has a 'd' in it, it's fixed damage
    */
    if (x_Is_Digit(*text) && (strchr(text, 'd') != NULL)) {
        strcpy(fixed, text);
        *type = e_damage_fixed;
        goto cleanup_exit;
        }

    /* Otherwise, parse the number out, since we're something like 2[W] or 1[B]
    */
    multiplier[0] = text[0];
    multiplier[1] = '\0';

    /* Check for [W] or [B] and set the type appropriately
    */
    if (strncmp(text+1, "[W]", 3) == 0)
        *type = e_damage_weapon;
    else if (strncmp(text+1, "[B]", 3) == 0)
        *type = e_damage_beast;
    else {
        multiplier[0] = '\0';
        goto cleanup_exit;
        }

    /* If there's a following " + 1d6" or something after it, parse it out into
        the fixed buffer
    */
    text = end + 1;
    Consume_Characters(&text, ' ');
    if (*text != '+')
        goto cleanup_exit;
    text++;
    Consume_Characters(&text, ' ');
    followend = strchr(text, ' ');
    if (end == NULL)
        goto cleanup_exit;
    *followend = '\0';
    if (!x_Is_Digit(*text) || (strchr(text, 'd') == NULL)) {
        *followend = ' ';
        goto cleanup_exit;
        }
    strcpy(fixed, text);
    *end = ' ';
    end = followend;

cleanup_exit:
    *end = ' ';
    return (end + 1);
}


static T_Glyph_Ptr  Parse_Attributes(T_Power_Info * info, T_Glyph_Ptr text,
                                        vector<string> * attrs)
{
    T_Glyph_Ptr     end;

    Consume_Characters(&text, ' ');

    /* Make sure the next character is a plus sign, indicating that an
        attribute follows
    */
    if (*text != '+')
        return(text);
    text++;
    Consume_Characters(&text, ' ');

    /* Extract the attribute name
    */
    end = strchr(text, ' ');
    if (end == NULL)
        return(text);
    *end = '\0';
    attrs->push_back(string(text));
    *end = ' ';
    
    /* Parse out the word 'modifier' if present
    */
    text = end+1;
    if (strncmp(text, "modifier", 8) == 0) {
        text += 8;
        Consume_Characters(&text, ' ');
        }

    /* Try and parse out a second attribute if we have one
    */
    return(Parse_Attributes(info, text, attrs));
}


static T_Glyph_Ptr  Parse_Increase(T_Glyph_Ptr fixed, T_Glyph_Ptr multiplier, T_Glyph_Ptr ptr,
                                    T_Int32U * level, E_Power_Damage * type, T_Power_Info * info)
{
    vector<std::string> attrs;

    *level = 0;
    Consume_Characters(&ptr, ' ');

    /* Parse out our damage and any attribute increase - we don't actually do
        anything with the attributes right now, since we assume it'll stay the
        same throughout the life of the power
    */
    ptr = Parse_Damage(fixed, multiplier, ptr, type);
    if ((fixed[0] == '\0') && (multiplier[0] == '\0'))
        return(ptr);
    ptr = Parse_Attributes(info, ptr, &attrs);

    /* Parse out the word 'damage' if present
    */
    if (strncmp(ptr, "damage", 6) == 0) {
        ptr += 6;
        Consume_Characters(&ptr, ' ');
        }

    /* Now we're looking for the phrase "at XXth/st/nd level" to confirm the increase
    */
    if (strncmp(ptr, "at", 2) != 0)
        return(ptr);
    ptr += 2;
    Consume_Characters(&ptr, ' ');
    if (!x_Is_Digit(*ptr))
        return(ptr);
    *level = atoi(ptr);
    while (x_Is_Alnum(*ptr) && (*ptr != '\0'))
        ptr++;
    Consume_Characters(&ptr, ' ');
    if (strncmp(ptr, "level", 5) == 0) {
        ptr += 5;
        Consume_Characters(&ptr, ' ');
        }
    return(ptr);
}


static void         Generate_Eval_Script(T_Glyph_Ptr script, T_Int32U level,
                                            T_Glyph_Ptr fixed, T_Glyph_Ptr multiplier,
                                            E_Power_Damage type)
{
    T_Glyph_Ptr         ptr;
    bool                is_append;

    is_append = (*script != '\0');

    ptr = script + strlen(script);
    sprintf(ptr, "\n%sif (#level[] >= %lu) then\n", (is_append ? "else" : ""), level);
    ptr += strlen(ptr);
    if (fixed[0] != '\0') {
        sprintf(ptr, "  field[pwDamBase].text = \"%s\"\n", fixed);
        ptr += strlen(ptr);
        }
    if (type != e_damage_fixed) {
        strcpy(ptr, "  perform delete[Damage.?]\n");
        ptr += strlen(ptr);
        if (type == e_damage_weapon)
            sprintf(ptr, "  perform assign[Damage.Weapon%s]\n", multiplier);
        else if (type == e_damage_beast)
            sprintf(ptr, "  perform assign[Damage.Beast%s]\n", multiplier);
        ptr += strlen(ptr);
        }
}


static T_Glyph_Ptr  Parse_Further_Increases(T_Glyph_Ptr text, T_Glyph_Ptr script,
                                            T_Power_Info * info)
{
    T_Int32U            level;
    E_Power_Damage      type;
    T_Glyph             fixed[500], multiplier[500];

    if (strnicmp(text, "and to", 6) == 0)
        text += 6;
    else if (strnicmp(text, ", and", 5) == 0)
        text += 5;
    else if (*text == ',')
        text++;
    else
        return(text);

    /* Parse out this damage increase
    */
    text = Parse_Increase(fixed, multiplier, text, &level, &type, info);
    if (((fixed[0] == '\0') && (multiplier[0] == '\0')) || (level == 0))
        return(text);

    /* Recursively parse out any further increases
    */
    text = Parse_Further_Increases(text, script, info);

    /* Append it to the script
    */
    Generate_Eval_Script(script, level, fixed, multiplier, type);
    return(text);
}


static T_Glyph_Ptr  Parse_Damage_Increase(T_Power_Info * info, T_Glyph_Ptr text)
{
    T_Int32U            level;
    T_Glyph_Ptr         ptr, script_end;
    E_Power_Damage      type;
    T_Glyph             fixed[500], multiplier[500], script[10000];

    script[0] = '\0';

    ptr = strstr(text, "Increase damage to");
    if (ptr == NULL)
        return(text);
    ptr += 18;

    /* Parse out the details of the increase
    */
    ptr = Parse_Increase(fixed, multiplier, ptr, &level, &type, info);
    if (((fixed[0] == '\0') && (multiplier[0] == '\0')) || (level == 0))
        return(ptr);

    /* Recursively parse out any further increases
    */
    ptr = Parse_Further_Increases(ptr, script, info);

    /* Generate an eval script to change the damage at the appropriate level
    */
    Generate_Eval_Script(script, level, fixed, multiplier, type);

    /* Finish the eval script and output it
    */
    script_end = script + strlen(script);
    strcpy(script_end, "  endif\n");
    Output_Eval(info->node, script, "Traits", "10000", info);

    return(ptr);
}


static void Output_Damage(T_Power_Info * info)
{
    T_Glyph_Ptr         ptr;
    E_Power_Damage      type;
    vector<std::string> attrs;
    T_Glyph             buffer[500], fixed[500], multiplier[500];

    if ((info->description == NULL) || (info->description[0] == '\0'))
        return;

    /* We need to parse out the damage values from the power description. This
        can look like:
        {b}Hit{/b}: 1d6 + Dexterity modifier damage
        {b}Hit{/b}: 2[W] + Dexterity modifier + Strength modifier damage
        {b}Hit{/b}: 1[W] + Intelligence modifier force damage
        {b}Hit{/b}: 1d12 + Wisdom modifier radiant damage

        So we need to parse out:
        * Damage (either a dice text expression, or 1[W], or 1[B] for ranger
            beast thingies
        * Attribute(s) that add to the damage
        * Any damage increases

        We don't need to parse out the damage type because it's in the power
        keywords that have already been parsed.

        And then add them as tags / fields on the thing. Start by finding the
        "{b}Hit{/b}:" sequence in the power description.
    */
    ptr = strstr(info->description, "{b}Hit{/b}:");
    if (ptr == NULL)
        return;
    ptr += 11;
    Consume_Characters(&ptr, ' ');

    /* Parse out our damage
    */
    ptr = Parse_Damage(fixed, multiplier, ptr, &type);
    if ((fixed[0] == '\0') && (multiplier[0] == '\0'))
        return;

    /* If the damage is fixed, stick it in a field - otherwise, output a damage
        tag appropriately
    */
    if (fixed[0] != '\0')
        Output_Field(info->node, "pwDamBase", fixed, info);
    switch (type) {
        case e_damage_fixed :
            /* do nothing else */
            break;

        case e_damage_weapon :
            sprintf(buffer, "Weapon%s", multiplier);
            Output_Tag(info->node, "Damage", buffer, info);
            break;

        case e_damage_beast :
            sprintf(buffer, "Beast%s", multiplier);
            Output_Tag(info->node, "Damage", buffer, info);
            break;

        default :
            x_Trap_Opt(1);
            break;
        }

    /* Now parse out the attribute modifiers recursively and apply them
    */
    ptr = Parse_Attributes(info, ptr, &attrs);
    for (vector<std::string>::iterator it = attrs.begin(); it != attrs.end(); ++it)
        Output_Tag(info->node, "DamageAttr", (T_Glyph_Ptr) it->c_str(), info, "attr");

    /* Check to see if this power increases in damage at higher levels.
    */
    ptr = Parse_Damage_Increase(info, ptr);
}


T_Status    C_DDI_Powers::Output_Entry(T_XML_Node root, T_Power_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32U        i;
    long            result;
    T_Glyph         backup;
    T_Glyph_Ptr     ptr, end, temp, class_id, class_abbrev, type, use, tag_id, group;
    T_XML_Node      node;
    T_Tuple *       tuple;
    bool            is_paragon = false, is_epic = false;
    T_Glyph         power_id[UNIQUE_ID_LENGTH+1], range1[500], range2[500], buffer[500];

    /* If we don't have a class, something odd is going on...
    */
    if ((info->forclass == NULL) || (info->forclass[0] == '\0')) {
        sprintf(buffer, "No class specified for power %s.\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Find the class id and abbreviation - if we can't find one, we can't
        generate a unique id for this power
    */
    class_id = NULL;
    class_abbrev = NULL;
    tuple = Mapping_Find_Tuple("class", info->forclass);
    if (tuple != NULL) {
        class_id = tuple->b;
        class_abbrev = tuple->c;
        }

    /* If we didn't find one, look in our paragon paths list
    */
    if (class_id == NULL) {
        tuple = Mapping_Find_Tuple("path", info->forclass);
        if (tuple != NULL) {
            class_id = tuple->b;
            class_abbrev = "Par";
            is_paragon = true;
            }
        }

    /* If we didn't find one, look in our epic destinies list
    */
    if (class_id == NULL) {
        tuple = Mapping_Find_Tuple("destiny", info->forclass);
        if (tuple != NULL) {
            class_id = tuple->b;
            class_abbrev = "Epc";
            is_epic = true;
            }
        }

    /* If we still didn't find it in our 'real classes' list, look in our 'fake
        classes' list
    */
    if (class_id == NULL) {
        tuple = Mapping_Find_Tuple("fakeclass", info->forclass);
        if (tuple != NULL) {
            class_id = tuple->b;
            class_abbrev = tuple->c;
            }

        /* If we have a fake class of "Multiclass", then we need to extract the
            *real* power class from the power type - the type will be something
            like "Spellscarred Attack 3", so just chop off the first word. This
            lets us distinguish spellscarred stuff from other weird powers,
            etc.
        */
        if ((class_id != NULL) && (stricmp(class_id, "Multiclass") == 0)) {
            if ((info->type == NULL) || (info->type[0] == '\0')) {
                sprintf(buffer, "Unable to determine real power class for power %s\n", info->name);
                Log_Message(buffer);
                }
            else {
                strcpy(buffer, info->type);
                temp = strchr(buffer, ' ');
                if (temp != NULL)
                    *temp = '\0';
                tuple = Mapping_Find_Tuple("fakeclass", buffer);
                if (tuple != NULL) {
                    class_id = tuple->b;
                    class_abbrev = tuple->c;
                    }
                }
            }
        }

    if (class_id == NULL) {
        sprintf(buffer, "No class id found for %s (power %s)\n", info->forclass, info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Initialize the class id buffer with the class abbreviation
    */
    buffer[0] = 'p';
    strcpy(buffer+1, class_abbrev);

    /* Generate a unique id for the power - if we can't, get out
    */
    if (!Generate_Thing_Id(power_id, buffer, info->name, &m_id_list, 3, info->idnumber)) {
        sprintf(buffer, "Error generating unique id for power %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(power_id);

    /* Create our node and stick all the standard stuff into it
    */
    result = Create_Thing_Node(root, m_term, "Power", info->name, power_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;
    node = info->node;

    /* If we have a fake class of "Skill Power", then we need to extract the
        skill from the power type - the type will be something like "Thievery
        Utility 6", so just chop off the first word.
    */
    if ((class_id != NULL) && (stricmp(class_id, "SkillPower") == 0)) {
        if ((info->type == NULL) || (info->type[0] == '\0')) {
            sprintf(buffer, "Unable to determine real power skill for power %s\n", info->name);
            Log_Message(buffer);
            }
        else {
            strcpy(buffer, info->type);
            temp = strchr(buffer, ' ');
            if (temp != NULL)
                *temp = '\0';
            Output_Tag(info->node, "ReqSkill", buffer, info, "skill");
            }
        }

    /* Parse out damage from the power
    */
    Output_Damage(info);

    /* Add general tags
    */
    if ((info->level != NULL) && (info->level[0] != '\0')) {
        if (atoi(info->level) != 0)
            Output_Tag(node, "ReqLevel", info->level, info);
        }
    if ((info->action != NULL) && (info->action[0] != '\0'))
        Output_Tag(node, "ActionType", info->action, info, "action");
    ptr = (T_Glyph_Ptr) (is_paragon ? "PowerPath" : (is_epic ? "PowerDest" : "PowerClass"));
    result = Output_Tag(node, ptr, class_id, info);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* See if this is a Utility power or not. If it is, we know our type.
        Otherwise, we need to check our power use to find our power type.
    */
    type = NULL;
    use = NULL;
    if (info->type == NULL) {
        sprintf(buffer, "No power type found for power %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    if (strstr(info->type, "Utility") != NULL)
        type = "Utility";
    else if (strstr(info->type, "Feature") != NULL)
        type = "Feature";
    else if (strstr(info->type, "Build Option") != NULL)
        type = "BuildOpt";
    temp = Mapping_Find("poweruse", info->use);
    if (temp != NULL) {
        if (type == NULL)
            type = temp;
        else
            use = temp;
        }
    if (type == NULL) {
        sprintf(buffer, "No power type found for %s\n", info->use);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Output tags for our power type and use, as necessary
    */
    result = Output_Tag(node, "PowerType", type, info);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;
    if (use != NULL) {
        result = Output_Tag(node, "PowerUse", use, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        }

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* If we have some sort of power limit, add a tag now that our thing node
        has been created
    */
    if (info->limit != NULL)
        Output_Tag(node, "PowerLink", info->limit, info, "powerlimit", NULL, true);

    /* If we have some special text, output it and scan it for anything
        special. For example, it might specify that the power can be used
        multiple times.
    */
    if (info->special != NULL) {
        result = Output_Field(node, "pwSpecial", info->special, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        result = Output_Power_Special(node, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        }

    /* Add fields
    */
    if ((info->flavor != NULL) && (info->flavor[0] != '\0')) {
        result = Output_Field(node, "pwFlavor", info->flavor, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        }
    if ((info->target != NULL) && (info->target[0] != '\0')) {
        result = Output_Field(node, "pwTarget", info->target, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        }
    if ((info->requirement != NULL) && (info->requirement[0] != '\0')) {
        result = Output_Field(node, "pwRequire", info->requirement, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        }

    /* Now output tags for all our keywords. Keywords can be in 4 different
        arrays, so check them all.
    */
    for (i = 0; i < info->keyword_count; i++) {
        group = NULL;
        tag_id = Mapping_Find("powersrc", info->keywords[i]);
        if (tag_id != NULL)
            group = "PowerSrc";
        if (group == NULL) {
            temp = Mapping_Find("accessory", info->keywords[i]);
            if (temp != NULL) {
                tag_id = temp;
                group = "PowerAcc";
                }
            }
        if (group == NULL) {
            temp = Mapping_Find("damagetype", info->keywords[i]);
            if (temp != NULL) {
                tag_id = temp;
                group = "DamageType";
                }
            }
        if (group == NULL) {
            temp = Mapping_Find("effecttype", info->keywords[i]);
            if (temp != NULL) {
                tag_id = temp;
                group = "EffectType";
                }
            }
        if (group == NULL) {
            sprintf(buffer, "No keyword found for %s (power %s)\n", info->keywords[i], info->name);
            Log_Message(buffer);
            continue;
            }
        Output_Tag(node, group, tag_id, info);
        }

    /* Output our range - try and find it in the array first
    */
    if ((info->range != NULL) && (info->range[0] != '\0')) {
        tag_id = Mapping_Find("powerrange", info->range);
        range1[0] = '\0';
        range2[0] = '\0';

        /* If we didn't find one, check for a special range like "Melee 2" or
            "Area Burst 3 Within 10 Squares"
        */
        if (tag_id == NULL) {
            if (strnicmp(info->range, "Melee ", 6) == 0) {
                tag_id = "Melee";
                strcpy(range1, info->range + 6);
                }
            else if (strnicmp(info->range, "Ranged ", 7) == 0) {
                tag_id = "Range";
                strcpy(range1, info->range + 7);
                }
            else if (strnicmp(info->range, "Close Burst ", 12) == 0) {
                tag_id = "CloseBurst";
                strcpy(range1, info->range + 12);
                }
            else if (strnicmp(info->range, "Close Blast ", 12) == 0) {
                tag_id = "CloseBlast";
                strcpy(range1, info->range + 12);
                }
            else if (strnicmp(info->range, "Area Burst ", 11) == 0) {
                tag_id = "AreaBurst";
                strcpy(range1, info->range + 11);
                ptr = range1;
                while ((*ptr != '\0') && (*ptr != ' '))
                    ptr++;
                if (*ptr == '\0') {
                    sprintf(buffer, "Bad range id found for %s\n", info->range);
                    Log_Message(buffer);
                    goto cleanup_exit;
                    }
                *ptr++ = '\0';
                while (!x_Is_Digit(*ptr) && (*ptr != '\0'))
                    ptr++;
                strcpy(range2, ptr);
                ptr = range2;
                while ((*ptr != '\0') && (*ptr != ' '))
                    ptr++;
                *ptr++ = '\0';
                }
            else if (strnicmp(info->range, "Area Wall ", 10) == 0) {
                tag_id = "AreaWall";
                strcpy(range1, info->range + 10);
                ptr = range1;
                while ((*ptr != '\0') && (*ptr != ' '))
                    ptr++;
                if (*ptr == '\0') {
                    sprintf(buffer, "Bad range id found for %s\n", info->range);
                    Log_Message(buffer);
                    goto cleanup_exit;
                    }
                *ptr++ = '\0';
                while (!x_Is_Digit(*ptr) && (*ptr != '\0'))
                    ptr++;
                strcpy(range2, ptr);
                ptr = range2;
                while ((*ptr != '\0') && (*ptr != ' '))
                    ptr++;
                *ptr++ = '\0';
                }
            else if (strnicmp(info->range, "Area ", 5) == 0) {
                tag_id = "Area";
                strcpy(range1, info->range + 5);
                ptr = range1;
                while ((*ptr != '\0') && (*ptr != ' '))
                    ptr++;
                if (*ptr == '\0') {
                    }
                *ptr++ = '\0';
                while (!x_Is_Digit(*ptr) && (*ptr != '\0'))
                    ptr++;
                strcpy(range2, ptr);
                ptr = range2;
                while ((*ptr != '\0') && (*ptr != ' '))
                    ptr++;
                *ptr++ = '\0';
                }
            else {
                sprintf(buffer, "No range id found for %s (power %s)\n", info->range, info->name);
                Log_Message(buffer);
                goto cleanup_exit;
                }
            }

        /* If we still can't find one, that's a problem
        */
        if (tag_id == NULL) {
            sprintf(buffer, "No range id found for %s\n", info->range);
            Log_Message(buffer);
            goto cleanup_exit;
            }

        /* If our range is 'or Ranged weapon', and our attack extra text contains
            the string "or Dexterity", we're a special attack that switches
            to Dexterity if a ranged weapon is used with us, so add a tag to
            signify that.
        */
        if ((stricmp(range1, "or ranged weapon") == 0) &&
            (info->attack != NULL) &&
            (strstr(info->attack, "or Dexterity") != NULL)) {
            Output_Tag(node, "User", "RangeDex", info);
            Output_Tag(node, "AttackType", "Range", info);
            strcpy(range1, "");
            }

        /* Output our range tags
        */
        Output_Tag(node, "AttackType", tag_id, info);
        if (range1[0] != '\0') {
            result = Output_Field(node, "pwRange1", range1, info);
            if (x_Trap_Opt(result != 0))
                goto cleanup_exit;
            }
        if (range2[0] != '\0') {
            result = Output_Field(node, "pwRange2", range2, info);
            if (x_Trap_Opt(result != 0))
                goto cleanup_exit;
            }
        }

    /* If we're an attack, output it
    */
    if ((info->attack != NULL) && (info->attack[0] != '\0')) {

        /* If this is a beast power, handle it
        */
        if (strnicmp(info->attack, "Beast's attack bonus", 20) == 0) {
            Output_Tag(node, "Attack", "Beast", info);
            end = info->attack + 20;
            }

        /* Otherwise, assume the first word is the attack attribute
        */
        else {
            end = strchr(info->attack, ' ');
            if (end == NULL) {
                sprintf(buffer, "No attack attribute found for %s\n", info->attack);
                Log_Message(buffer);
                goto cleanup_exit;
                }
            *end = '\0';
            Output_Tag(node, "Attack", info->attack, info, "attr");
            *end = ' ';
            }

        /* See if there's a bonus / penalty just after the attribute, like
            "Intelligence + 2" or "Wisdom -1", and output it. Note that
            there's sometimes a space between the sign and the bonus, and
            sometimes not.
        */
        ptr = end + 1;
        while ((*ptr == ' ') && (*ptr != '\0'))
            *ptr++;
        if ((*ptr == '+') || (*ptr == '-')) {
            temp = ptr;
            ptr++;
            while ((*ptr == ' ') && (*ptr != '\0'))
                *ptr++;
            end = ptr;
            while ((*end != ' ') && (*end != '\0'))
                *end++;
            if (*end != '\0') {
                *end = '\0';
                sprintf(buffer, "%c%s", *temp, ptr);
                result = Output_Field(node, "pwAtkMod", buffer, info);
                *end = ' ';
                ptr = end + 1;
                }
            }

        /* Parse out any "vs" that happens next
        */
        while (*ptr == ' ')
            ptr++;
        if (strncmp(ptr, "vs.", 3) == 0)
            ptr += 3;
        else if (strncmp(ptr, "vs", 2) == 0)
            ptr += 2;
        while (*ptr == ' ')
            ptr++;

        /* Everything until the next punctuation mark or space is our defense -
            if we can parse it as a defense, skip past it, otherwise it needs
            to be included in the extra attack details
        */
        end = ptr;
        while (x_Is_Alpha(*end))
            end++;
        backup = *end;
        *end = '\0';
        result = Output_Tag(node, "AttackVs", ptr, info, "defense");
        *end = backup;
        if (result == 0)
            ptr = end;

        /* The rest of the line is extra stuff
        */
        if (ptr[0] != '\0')
            Output_Field(node, "pwAtkExtra", ptr, info);
        }

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


T_Status    C_DDI_Powers::Post_Process_Entry(T_XML_Node root, T_Power_Info * info)
{
    x_Status_Return_Success();
}