/*  FILE:   OUTPUT_CLASSES.CPP

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

#include "crawler.h"


#define CLASS_FEATURE_REPLACES  "This class feature replaces"
#define FEATURE_DISABLE_SCRIPT  "perform hero.child[%s].setfocus\n" \
                                "call FeatureDis"
#define CHANNEL_DIVINITY_SCRIPT "doneif (activated = 0)\n\n" \
                                "perform hero.assign[Hero.ChannelDiv]"


/* Things for building a custom mechanics pick for Essentials classes
    NOTE: There are really only 30 levels, but we count from 1 for ease of
            thinking about it, hence defining the array with 31 slots
*/
#define MAX_LEVELS              31
#define MAX_POWERS_PER_LEVEL    20

enum E_Power_Type {
    e_type_at_will = 0,
    e_type_encounter = 1,
    e_type_utility = 2,
    e_type_daily = 3
    };
#define POWER_TYPE_COUNT        (e_type_daily + 1)

struct T_Powers_Available {
    T_Int32U        slot[MAX_POWERS_PER_LEVEL];
    };


static T_Glyph_Ptr Get_Class_Abbrev(T_Glyph_Ptr class_id)
{
    T_Glyph_Ptr     ptr;

    ptr = Mapping_Find("classabbr", class_id);
    x_Trap_Opt(ptr == NULL);
    return(ptr);
}


static void Output_Defense_Bonuses(T_Class_Info * info)
{
    T_Int32U            i, chunk_count;
    T_Int32S            bonus;
    T_Glyph             sign;
    T_Glyph_Ptr         ptr, field, chunks[1000];
    T_Glyph             buffer[100000], message[1000];

    if ((info->defbonuses == NULL) || (info->defbonuses == '\0')) {
        sprintf(buffer, "No defense bonuses found for class %s\n", info->name);
        Log_Message(buffer);
        return;
        }

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->defbonuses, ",.");

    for (i = 0; i < chunk_count; i++) {
        ptr = chunks[i];
        sign = '\0';
        bonus = 0;
        field = NULL;

        /* Get the sign for whether we're adding or subtracting to a defense
        */
        if ((*ptr == '+') || (*ptr == '-'))
            sign = *ptr++;

        /* Get the size of the bonus, then skip past all digits and spaces
        */
        bonus = atoi(ptr);
        if (sign == '-')
            bonus *= -1;
        while (x_Is_Digit(*ptr) || (*ptr == ' '))
            ptr++;

        /* The rest of the chunk should be the defense name
        */
        if (stricmp(ptr, "Fortitude") == 0)
            field = "clsFort";
        else if (stricmp(ptr, "Reflex") == 0)
            field = "clsRef";
        else if (stricmp(ptr, "Will") == 0)
            field = "clsWill";
        else if (stricmp(ptr, "AC") == 0)
            field = "clsAC";
        else if (stricmp(ptr, "Armor Class") == 0)
            field = "clsAC";
        else {
            sprintf(message, "Couldn't determine defense bonus '%s' for class %s\n", chunks[i], info->name);
            Log_Message(message);
            continue;
            }

        /* Output the field
        */
        Output_Field(info->node, field, As_Int(bonus), info);
        }
}


static void Output_Trained_Skills(T_Class_Info * info, bool is_hybrid)
{
    T_Int32U            skill_choices;
    T_Glyph_Ptr         ptr, end, temp;
    T_Glyph             backup, buffer[5000], buffer2[5000];

    if (!is_hybrid && ((info->skl_trained == NULL) || (info->skl_trained == '\0'))) {
        sprintf(buffer, "No trained skills found for class %s\n", info->name);
        Log_Message(buffer);
        return;
        }

    /* Do stuff with our trained skills - make a copy so we can do a
        destructive parse
    */
    strcpy(buffer, (info->skl_trained == NULL) ? "" : info->skl_trained);

    /* First, find the part that tells us how many class skills we can pick.
    */
    ptr = buffer;
    ptr = strstr(buffer, "From the class skills list below");
    if (ptr != NULL) {

        /* Firstly, terminate the buffer before this point, at the end of the
            list of compulsory skills
        */
        end = ptr++;
        while ((end > buffer) && (end[-1] == ' '))
            end--;
        *end = '\0';

        /* Parse the first number out of the string as our skill count
        */
        while (!x_Is_Digit(*ptr) && (*ptr != '\0'))
            ptr++;
        if (*ptr != '\0') {
            end = ptr;
            while (x_Is_Digit(*end) && (*end != '\0'))
                end++;
            *end = '\0';
            Output_Field(info->node, "clsSkills", ptr, info);
            }
        }

    /* The remaining characters before the terminator in buffer are the list of
        trained skills the class automatically gets. The string will be empty,
        or something like:
            Religion.
            Stealth, Thievery.
            Dungeoneering or Nature (your choice).
    */
    skill_choices = 0;
    ptr = buffer;
    while ((*ptr == ' ') && (*ptr != '\0'))
        ptr++;
    while (*ptr != '\0') {

        /* Parse up to the next comma, full stop, or opening paren - that's the
            end of this item.
        */
        while (*ptr == ' ')
            ptr++;
        end = ptr + strlen(ptr);
        temp = strchr(ptr, '(');
        if (temp != NULL)
            end = min(end, temp);
        temp = strchr(ptr, ',');
        if (temp != NULL)
            end = min(end, temp);
        temp = strchr(ptr, '.');
        if (temp != NULL)
            end = min(end, temp);

        /* Skip backwards until we consume all whitespace, then null-terminate
            so we can search within this entry
        */
        while ((end > ptr) && (end[-1] == ' '))
            end--;
        backup = *end;
        *end = '\0';

        /* Search for the word " or " (with spaces for disambiguation) in the
            list - this lets us know if it's a choice the user has to make.
        */
        temp = strstr(ptr, " or ");
        if (temp != NULL) {
            *temp = '\0';
            sprintf(buffer2, "Skill%lu", ++skill_choices);
            Output_Tag(info->node, buffer2, ptr, info, "skill");
            Output_Tag(info->node, buffer2, temp + 4, info, "skill");
            *temp = ' ';
            }

        /* Otherwise, output a tag for this trained skill
        */
        else
            Output_Tag(info->node, "TrainSkill", ptr, info, "skill");

        /* Restore the character we changed to a null terminator
        */
        *end = backup;

        /* Now move on until we find a comma (the end of this skill) or the end
            of the string. Move past the comma, then we'll parse the next
            skill.
        */
        ptr = end;
        while ((*ptr != ',') && (*ptr != '\0'))
            ptr++;
        if (*ptr == ',')
            ptr++;
        }
}


static void Output_Tag_And_Field(T_Class_Info * info, T_Glyph_Ptr what, T_Glyph_Ptr text,
                                    T_Glyph_Ptr splitchars, T_Glyph_Ptr group,
                                    T_Glyph_Ptr mapping, T_Glyph_Ptr field)
{
    T_Int32U            chunk_count;
    T_Glyph_Ptr         ptr, chunks[1000];
    T_Glyph             buffer[100000];

    if ((text == NULL) || (text[0] == '\0')) {
        sprintf(buffer, "No %s found for class %s\n", what, info->name);
        Log_Message(buffer);
        return;
        }

    /* Chunk our text - we only care about the first chunk, because that's
        the tag we're looking for.
    */
    chunk_count = 1;
    Split_To_Array(buffer, chunks, &chunk_count, text, splitchars);
    if (chunk_count == 0) {
        sprintf(buffer, "Couldn't extract %s for class %s\n", what, info->name);
        Log_Message(buffer);
        return;
        }
    Output_Tag(info->node, group, chunks[0], info, mapping);

    /* Everything after that, put in our field
    */
    ptr = text + strlen(chunks[0]);
    while ((*ptr != '\0') && (x_Is_Space(*ptr) || (strchr(splitchars, *ptr) != NULL)))
        ptr++;
    if (*ptr == '\0')
        return;
    Output_Field(info->node, field, ptr, info);
}


static void Output_Weapon_Proficiencies(T_Class_Info * info)
{
    T_Int32U            i, chunk_count;
    T_Tuple *           category;
    T_Tuple *           group;
    T_Glyph_Ptr         ptr, chunks[1000];
    T_Glyph             buffer[100000],message[1000];

    if ((info->weaponprofs == NULL) || (info->weaponprofs[0] == '\0')) {
        sprintf(buffer, "No weapon proficiencies found for class %s\n", info->name);
        Log_Message(buffer);
        return;
        }

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->weaponprofs, ",.");
    if (chunk_count == 0) {
        sprintf(buffer, "Couldn't extract weapon proficiencies for class %s\n", info->name);
        Log_Message(buffer);
        return;
        }
    for (i = 0; i < chunk_count; i++) {

        /* Match the first word to a weapon category - e.g. "Simple" or
            "Military"
        */
        category = Mapping_Find_Tuple("wepcat", chunks[i], true);

        /* If we didn't find one, it's probably a weapon, so output a tag
            directly for it
        */
        if (category == NULL) {
            Output_Tag(info->node, "WeaponProf", chunks[i], info, "weapon");
            continue;
            }

        /* Now match the remainder to "ranged", "melee", or a weapon group - if
            it's just "ranged" or "melee", we can output a simple tag
        */
        ptr = chunks[i] + strlen(category->a);
        while (*ptr == ' ')
            ptr++;
        if (stricmp(ptr, "melee") == 0) {
            Output_Tag(info->node, "WpCatRqMel", category->b, info);
            continue;
            }
        if (stricmp(ptr, "ranged") == 0) {
            Output_Tag(info->node, "WpCatRqRng", category->b, info);
            continue;
            }

        /* Otherwise, see which weapon group we match
        */
        group = Mapping_Find_Tuple("wepgroup", ptr);
        if (group == NULL) {
            sprintf(message, "Unknown weapon group %s for class %s\n", ptr, info->name);
            Log_Message(message);
            continue;
            }

        /* Output a tag in the appropriate tag group
        */
        Output_Tag(info->node, category->c, group->b, info);
        }
}


T_Status    C_DDI_Classes::Output_Entry(T_XML_Node root, T_Class_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_XML_Node      node;
    T_Glyph_Ptr     abbrev, ptr, equivalent, hybrid_for;
    bool            is_hybrid = false;
    T_Glyph         class_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Generate a unique id for the class - if we can't, get out
    */
    if (!Generate_Thing_Id(class_id, "cls", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for class %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(class_id);

    /* Get our abbreviation based on our class name - this isn't something we
        can generate, we have to do it by hand. If we can't find one, something
        odd is going on - get out.
    */
    abbrev = Get_Class_Abbrev(class_id);
    if (x_Trap_Opt(abbrev == NULL)) {
        sprintf(buffer, "Couldn't get abbreviation for class %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Class", info->name, class_id, info->description,
                                UNIQUENESS_UNIQUE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;
    node = info->node;

    /* Add this class to our internal list of classes
    */
    Mapping_Add("class", info->name, m_pool->Acquire(class_id), abbrev);

    /* If this class starts with "Hybrid", add a special tag to it and a tag
        for the normal class it 'counts as'
    */
    if (strncmp(info->name, "Hybrid ", 7) == 0) {
        is_hybrid = true;
        Output_Tag(info->node, "User", "Hybrid", info);
        hybrid_for = info->name + 7;

        /* If there's an essentials mapping involved, handle it
        */
        equivalent = Mapping_Find("fakeclass", hybrid_for);
        if (equivalent != NULL)
            Output_Tag(info->node, "Class", equivalent, info);

        /* Otherwise use the vanilla class name
        */
        else {
            ptr = Mapping_Find("class", hybrid_for);
            if ((ptr != NULL) && (ptr[0] != '\0'))
                Output_Tag(info->node, "Class", ptr, info);
            }
        }

    /* Output our class role. The first word of these lines is the relevant tag,
        and should be added to a tag group; the rest is the flavor text and
        should be stuck in a field.
    */
    Output_Tag_And_Field(info, "role", info->role, ". ", "ClassRole", "role", "clsRoleTxt");
    Split_To_Tags(node, info, info->keyabilities, ",.;", "KeyAbility", "attr");

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* Add various easy fields
    */
    Output_Field(node, "clsStartHP", info->hp_level1, info);
    Output_Field(node, "clsHPLev", info->hp_perlevel, info);
    Output_Field(node, "clsSurges", info->surges, info);
    Output_Field(node, "clsFlavor", info->flavor, info);

    /* Output our power source. The first word of these lines is
        the relevant tag, and should be added to a tag group; the rest is the
        flavor text and should be stuck in a field.
    */
    Output_Tag_And_Field(info, "power source", info->powersource, ". ", "PowerSrc",
                            "powersrc", "clsSrcTxt");

    /* Output our list of armor proficiencies
    */
    Split_To_Tags(node, info, info->armorprofs, ",.;", "ArmorProf", "armorprof");

    /* Output our list of weapon proficiencies
    */
    Output_Weapon_Proficiencies(info);

    /* Output our list of defense bonuses
    */
    Output_Defense_Bonuses(info);

    /* Output our list of implement types if we have them
    */
    if (info->implement != NULL)
        Split_To_Tags(node, info, info->implement, ",.", "ImplemType", "impltype");

    /* Output our trained skills
    */
    Output_Trained_Skills(info, is_hybrid);

    /* Output our class skills. We need to strip the "(Wis)" etc bits off the
        end before we turn them into tags.
    */
    Split_To_Tags(node, info, info->skl_class, ",.", "Skill", "skill", NULL, 0, "(");

    /* Output our original name if we have one
    */
    if ((info->original_name != NULL) && (info->original_name[0] != '\0'))
        Output_Tag(node, "ClassName", info->original_name, info);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


static void Find_Feature_Powers(T_XML_Node classnode, T_Glyph_Ptr class_id,
                                vector<T_XML_Node> * list, T_Class_Info * info)
{
    T_Int32S            result;
    T_Glyph_CPtr        cptr;
    T_XML_Document      document;
    T_XML_Node          root, node, child;

    /* Get the XML document for the powers list, and iterate through the powers
        in it.
    */
    document = C_DDI_Powers::Get_Output_Document();
    result = XML_Get_Document_Node(document, &root);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not get XML document root.");
        return;
        }
    result = XML_Get_First_Named_Child(root, "thing", &node);
    while (result == 0) {

        /* If this power isn't a feature, skip it.
        */
        result = XML_Get_First_Named_Child_With_Attr(node, "tag", "group", "PowerType", &child);
        if (result != 0)
            goto next_power;
        cptr = XML_Get_Attribute_Pointer(child, "tag");
        if (strcmp(cptr, "Feature") != 0)
            goto next_power;

        /* if this power isn't for the right class, skip it.
        */
        result = XML_Get_First_Named_Child_With_Attr(node, "tag", "group", "PowerClass", &child);
        if (result != 0)
            goto next_power;
        cptr = XML_Get_Attribute_Pointer(child, "tag");
        if (strcmp(cptr, class_id) != 0)
            goto next_power;

        /* Otherwise, add the node to our list
        */
        list->push_back(node);

next_power:
        result = XML_Get_Next_Named_Child(root, &node);
        }
}


static T_Glyph_Ptr  Pull_Power_Id(T_Glyph_Ptr name, vector<T_XML_Node> * list)
{
    T_Int32U        i, count;
    T_Glyph_CPtr    cptr;
    T_XML_Node      node;

    /* See if this feature appears in the list of 'feature' powers for the
        class - if so, return its unique id.
    */
    count = list->size();
    for (i = 0; i < count; i++) {
        node = (*list)[i];
        if (x_Trap_Opt(node == NULL))
            continue;
        cptr = XML_Get_Attribute_Pointer(node, "name");

        /* If we found the power, remove it from the list and return its id.
            That way, we can just bootstrap any powers that aren't features at
            the end of processing.
        */
        if (stricmp(name, cptr) == 0) {
            list->erase(list->begin() + i);
            return((T_Glyph_Ptr) XML_Get_Attribute_Pointer(node, "id"));
            }
        }

    return(NULL);
}


static T_Glyph_Ptr  Get_Feature_Description(T_Glyph_Ptr * final, T_Glyph_Ptr name, T_Glyph_Ptr details,
                                    C_Pool * pool)
{
    T_Status            status;
    T_Glyph_Ptr         ptr, end, temp, resume;
    T_Glyph             buffer1[500],buffer2[500];

    *final = "";

    /* Search for the name in the details. If the name is "Channel Divinity",
        we want to search for "<b>CHANNEL DIVINITY</b>", for example.
    */
    strcpy(buffer1, name);
    strupr(buffer1);
    sprintf(buffer2, "<b>%s</b>{br}", buffer1);
    ptr = strstr(details, buffer2);
    if (ptr == NULL) {
        sprintf(buffer1, "No description found for class feature %s", name);
        Log_Message(buffer1);
        return(NULL);
        }

    /* Now search for the ending marker, which looks like "{br}{br}<b>",
        "{br}{br}<i>" or "{br}{br}<p>" - if we don't find either of those,
        fall back to "{br}{br}". We need to parse out any bold or italic tag
        later, so don't skip past it.
    */
    end = strstr(ptr, "{br}{br}<b>");
    temp = strstr(ptr, "{br}{br}<i>");
    if ((end == NULL) || ((temp != NULL) && (temp < end)))
        end = temp;
    temp = strstr(ptr, "{br}{br}<p>");
    if ((end == NULL) || ((temp != NULL) && (temp < end)))
        end = temp;
    if (end == NULL)
        end = strstr(ptr, "{br}{br}");
    if (end == NULL) {
        sprintf(buffer1, "No end to description found for class feature %s", name);
        Log_Message(buffer1);
        return(NULL);
        }
    resume = end + strlen("{br}{br}");

    /* Extract the feature description
    */
    *end = '\0';
    status = Parse_Description_Text(final, ptr + strlen(buffer2), pool);
    x_Trap_Opt(!x_Is_Success(status));
    *end = '{';

    /* Return the point directly after the end of the feature, so we can try
        and parse build options out of it
    */
    return(resume);
}


static void Find_Nonhybrid_Version(T_XML_Node root, T_XML_Node node, T_Glyph_Ptr name,
                                    T_Class_Info * info)
{
    T_Int32S        result;
    T_XML_Node      child;
    T_Glyph_Ptr     ptr;
    T_Glyph         buffer[500];

    /* If we don't end with (Hybrid), we're not a hybrid version of some normal
        feature
    */
    if (stricmpright(name, "(Hybrid)") != 0)
        return;

    /* Otherwise work out the name we need to look for - strip off the (Hybrid)
        and all spaces
    */
    strcpy(buffer, name);
    ptr = buffer + strlen(buffer);
    ptr -= 8;
    while (x_Is_Space(ptr[-1]))
        ptr--;
    *ptr = '\0';

    /* Find the node - Hybrid classes are processed last, so the node should
        already have been created - and assign us a tag with its id, so we
        "count as" the feature with the regular name
    */
    result = XML_Get_First_Named_Child_With_Attr(root, "thing", "name", buffer, &child);
    if (x_Trap_Opt(result != 0))
        return;
    ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(child, "id");
    Output_Tag(node, "HasFeature", ptr, info);
}


static bool Parse_Build_Options(T_XML_Node root, T_XML_Node classnode, T_Glyph_Ptr abbrev,
                                T_Glyph_Ptr feature_id, T_Glyph_Ptr ptr, T_List_Ids * id_list,
                                C_Pool * pool, T_Class_Info * info)
{
    T_Int32U            i, count;
    T_Int32S            result;
    T_Glyph_Ptr         end, name, temp, descript, powerstart, resume;
    T_Glyph_Ptr         namestart, nameend, classid, optend, lookahead, temp2;
    T_Power_Info *      power_info;
    T_XML_Node          node, tag;
    T_Glyph             backup;
    bool                is_lower, is_source = false;
    T_Glyph             idbuffer[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Some classes like the shaman's "Companion Spirit" feature have a power
        here - if so, we need to skip it so we can parse out the build options.
    */
    if (strncmp(ptr, "<p><h1 class", 12) == 0) {
        temp = strstr(ptr, "</p><b>");
        if (temp != NULL)
            ptr = temp + 4; // leave our <b> to be parsed out

        /* Sometimes that isn't present, so here's our backup string to look
            for
        */
        else {
            temp = strstr(ptr, "</p></p>");
            if (x_Trap_Opt(temp == NULL))
                return(false);
            ptr = temp + 8;
            }
        }

    /* If we don't start with a section in bold, there are no options for
        this feature
    */
    if (strncmp(ptr, "<b>", 3) != 0)
        return(false);

    /* Otherwise, we're going to find something like the fighter's weapon
        talents, so we need to parse them out. Find the name of this build
        option, which is enclosed in the <b></b> tags.
    */
    ptr += 3;
    temp = "</b>{br}";
    end = strstr(ptr, temp);
    if (x_Trap_Opt(end == NULL))
        return(false);
    *end = '\0';
    name = pool->Acquire(ptr);
    *end = '<';
    ptr = end + strlen(temp);

    /* If the name is all in upper case, it's a feature, not an option
    */
    temp = name;
    is_lower = false;
    while (*temp != '\0') {
        if (islower(*temp++))
            is_lower = true;
        }
    if (!is_lower)
        return(false);

    /* The end of the option and the start of the next one is "{br}{br}<b>" (or
        "</p>{br}<b>", or if that fails just "{br}{br}"), some text, and then
        "</b>{br}". We need to search for both bits, or we get fooled by
        "{br}{br}<b>" separating different parts of the same build option.
    */
    temp2 = ptr;
    while (1) {
        optend = strstr(temp2, "{br}{br}<b>");
        resume = optend + 8;
        temp = strstr(temp2, "</p>{br}<b>");
        if ((optend == NULL) || ((temp != NULL) && (temp < optend))) {
            optend = temp;
            resume = optend + 8;
            }

        /* Also check for the end of an option and the start of a
            "<h3>Level 3:</h3>" section, as that ends this set of build
            options, which means we don't have to look ahead for the
            "real" end later if we actually found something.
        */
        temp = strstr(temp2, "{br}{br}<h3>");
        if ((optend == NULL) || ((temp != NULL) && (temp < optend))) {
            optend = temp;
            resume = optend + 8;
            if (temp != NULL)
                break;
            }
        if (optend == NULL) {
            optend = strstr(temp2, "{br}{br}");
            if (optend == NULL)
                optend = strstr(temp2, "{br}");
            resume = optend;
            break;
            }
        lookahead = optend + 13;
        while ((*lookahead != '<') && (*lookahead != '\0'))
            lookahead++;
        if (*lookahead == '\0')
            return(false);
        if (strncmp(lookahead, "</b>{br}", 8) == 0)
            break;
        temp2 = lookahead;
        }

    /* By default, our end pointer is the end of the option. However, this may
        change if we find powers before then - we want to stop before the powers.
        Set our end pointer, but terminate at the end of the option anyway.
    */
    backup = *optend;
    end = optend;
    *optend = '\0';

    /* If we find an "<h1 class=" section before that, that's a power at the
        end of the section. Set our new 'end' position appropriately.
    */
    powerstart = strstr(ptr, "<h1 class=");
    if (powerstart != NULL) {
        if (powerstart < end)
            end = powerstart;
        else
            powerstart = NULL;
        }

    /* Parse out the description and fix it up
    */
    *end = '\0';
    Parse_Description_Text(&descript, ptr, pool);
    *end = '{';

    /* If it's just "hey you gain a power" don't worry about it here
    */
    if (Is_Regex_Match("^You gain the (.+) power\\.$", descript, buffer))
        goto cleanup_exit;

    /* Generate a unique id for the feature
        NOTE: We don't want it to try make up clever ids for class features
            because that messes things up for some reason, so specify false
            as the last parameter
    */
    sprintf(buffer, "f%s", abbrev);
    if (!Generate_Thing_Id(idbuffer, buffer, name, id_list, 3, NULL, false)) {
        sprintf(buffer, "Error generating unique id for build option %s\n", name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* For some reason this shows up a lot, and we need to skip it
    */
    if (stricmp(name, "Hybrid talent Options") == 0)
        goto cleanup_exit;

    /* Output a new thing for this feature - note that it can't be unique,
        because some scripts on it need to be able to find its source.
    */
    result = Create_Thing_Node(root, "build option", "BuildOpt", name, idbuffer, descript,
                            UNIQUENESS_NONE, NULL, &node, info->is_partial);
    if (result != 0)
        goto cleanup_exit;
    Output_Tag(node, "BuildOpt", feature_id, info);

    Find_Nonhybrid_Version(root, node, name, info);

    /* Add a bootstrap node for this feature to the class
    */
    result = Output_Bootstrap(classnode, idbuffer, info);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* If we have powers to parse, we can do it now, now that the node has been
        created.
    */
    while (powerstart != NULL) {

        /* Find the name between a </span> tag and an </h1> tag
        */
        namestart = strstr(powerstart, "</span>");
        if (x_Trap_Opt(namestart == NULL))
            break;
        namestart += 7;
        nameend = strstr(namestart, "</h1>");
        if (x_Trap_Opt(nameend == NULL))
            break;
        *nameend = '\0';
        strcpy(buffer, namestart);
        *nameend = '<';

        /* Now that we have a copy of the name, try to find a power with this
            name for this class in the powers document
        */
        classid = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(classnode, "id");
        count = C_DDI_Powers::Get_Crawler()->Get_Item_Count();
        for (i = 0; i < count; i++) {
            power_info = C_DDI_Powers::Get_Crawler()->Get_Item(i);
            if (x_Trap_Opt(power_info == NULL))
                continue;
            if (stricmp(power_info->name, buffer) != 0)
                continue;

            /* If this power wasn't output for whatever reason, that's an error
            */
            if (power_info->node == NULL) {
                sprintf(buffer, "Build option power node for '%s' was not output.\n", power_info->name);
                Log_Message(buffer);
                break;
                }

            /* We know the power has the right name, so get the XML node and
                check the class just in case
            */
            result = XML_Get_First_Named_Child_With_Attr(power_info->node, "tag",
                        "group", "PowerClass", &tag);
            while (result == 0) {
                if (strcmp(XML_Get_Attribute_Pointer(tag, "tag"), classid) == 0)
                    break;
                result = XML_Get_Next_Named_Child_With_Attr(power_info->node, &tag);
                }
            if (x_Trap_Opt(result != 0))
                continue;

            /* Add a bootstrap for this power, contingent on the option being
                active
            */
            result = Output_Bootstrap(node, XML_Get_Attribute_Pointer(power_info->node, "id"),
                                        info, "fieldval:boActive <> 0", "Setup", "500", NULL,
                                        "Build option final");
            if (x_Trap_Opt(result != 0))
                goto cleanup_exit;
            break;
            }

        /* Now find the end of the power section. This not appear any more,
            since they changed stuff...
        */
        temp = strstr(nameend, "<i>First published in");
        temp2 = strstr(nameend, "<p>Published in");
        if ((temp == NULL) || ((temp2 != NULL) && (temp2 < temp)))
            temp = temp2;
        if (temp == NULL) {
            temp = powerstart+1;
            goto next_power;
            }
        temp += 3;
        while ((*temp != '>') && (*temp != '\0'))
            temp++;
        if (*temp == '\0')
            break;

        /* Skip past the > that indicates the start of the source
        */
        temp++;

        /* temp now points to the start of the source, so find the end
        */
        temp2 = temp;
        while ((*temp2 != '<') && (*temp2 != '\0'))
            temp2++;
        if (*temp2 == '\0')
            break;

        /* Extract and set the source
        */
        *temp2 = '\0';
        if (!is_source)
            Output_Source(node, temp);
        is_source = true;
        *temp2 = '<';

        /* Now find the end of the power section
        */
        temp2 = strstr(temp, "</i></p></p>");
        if (temp2 != NULL)
            temp = temp2 + 12;
        else {
            temp2 = strstr(temp, "</p>");
            if (temp2 == NULL)
                break;
            temp = temp2 + 4;
            }

        /* Consume a '{br}' if it's there - if it is, we're done with powers
        */
        if (strncmp(temp, "{br}", 4) == 0) {
            resume = temp + 4;
            break;
            }

        /* Otherwise, see if we can find another power
        */
next_power:
        powerstart = strstr(temp, "<h1 class=");
        }

    /* Un-terminate the rest of the build option
    */
    *optend = backup;

    /* See if there's another option after this one
    */
    ptr = resume;
    Parse_Build_Options(root, classnode, abbrev, feature_id, ptr, id_list, pool, info);

    /* We parsed an option, so return true - this feature has options
    */
    return(true);

    /* Something went wrong!
    */
cleanup_exit:
    *optend = backup;
    return(false);
}


static void Check_Replacement_Features(T_XML_Node root, T_XML_Node node, T_Class_Info * info)
{
    T_Int32U            length;
    T_Int32S            result;
    T_Glyph_Ptr         ptr;
    T_Glyph_CPtr        cptr, temp, desc, id_text;
    T_XML_Node          child;
    T_Glyph             name[500], buffer[500];

    /* Get the description of our feature and test for the magic phrase:
        This class feature replaces (the|your) .* class feature
    */
    desc = XML_Get_Attribute_Pointer(node, "description");
    if (desc[0] == '\0')
        return;
    ptr = CLASS_FEATURE_REPLACES;
    cptr = strstr(desc, ptr);
    if (cptr == NULL)
        return;
    cptr += strlen(ptr);

    /* Parse out the word "the" or "your", and any spaces around them
    */
    while (x_Is_Space(*cptr))
        cptr++;
    if (strnicmp(cptr, "the", 3) == 0)
        cptr += 3;
    else if (strnicmp(cptr, "your", 4) == 0)
        cptr += 4;
    else
        return;
    while (x_Is_Space(*cptr))
        cptr++;

    /* Parse out the name (i.e. any text before " class feature")
    */
    temp = strstr(cptr, " class feature");
    if (temp == NULL)
        return;
    length = temp - cptr;
    strncpy(name, cptr, length);
    name[length] = '\0';

    /* Find the appropriate class feature
    */
    result = XML_Get_First_Named_Child_With_Attr(root, "thing", "name", name, &child);
    if (result != 0)
        return;
    id_text = XML_Get_Attribute_Pointer(child, "id");

    /* Generate a script to disable it, and assign that to us along with a
        "needs checkbox" tag
    */
    Output_Tag(node, "User", "FeatureChk", info);
    sprintf(buffer, FEATURE_DISABLE_SCRIPT, id_text);
    Output_Eval_Rule(node, buffer, "Setup", "98", "Class feature error", info, "Disable class feature");
}


static void Find_Recommended_Powers(T_Class_Info * info)
{
    T_Int32S        result;
    T_Void_Ptr      rx;
    string          match;
    T_Power_Info *  power_info;
    T_XML_Node      node;
    bool            is_recommend;

    /* Build our regexp
    */
    rx = Regex_Create("<h1 class=\"[^\"]+\"><span class=\"level\">[^<]+</span>([^<]+)</h1>");

    /* Prepare a vector with the indicies of the capture groups we want to
        iterate through
    */
    vector<int> v;
    v.push_back(1);

    /* Go through our details text and find everything that looks like a
        power
    */
    is_recommend = false;
    match = Regex_Find_Match_Captures(rx, info->details, v);
    while (match.size() != 0) {

        /* Find the details of this power
        */
        power_info = C_DDI_Powers::Get_Crawler()->Find_Item(match.c_str());

        /* Incrementer the iterator once for each group we matched, now that
            we've used it
        */
        match = Regex_Next_Match_Capture(rx);

        /* If the power is already bootstrapped by the class, we don't need to
            recommend it
        */
        if (x_Trap_Opt(power_info == NULL))
            continue;
        if (x_Trap_Opt(power_info->id == NULL))
            continue;
        result = XML_Get_First_Named_Child_With_Attr(info->node, "bootstrap", "thing", power_info->id, &node);
        if (result == 0)
            continue;

        /* Otherwise add a tag for this class unless it's there already
        */
        result = XML_Get_First_Named_Child_With_Attr(power_info->node, "tag", "tag", info->id, &node);
        if (result == 0)
            continue;
        Output_Tag(power_info->node, "RecomClass", info->id, info);
        is_recommend = true;
        }

    Regex_Destroy(rx);

    /* Add a tag indicating this class uses recommended powers
    */
    if (is_recommend)
        Output_Tag(info->node, "User", "RecomPower", info);
}


static bool     Is_Conditional_Power(T_Glyph_Ptr description)
{
    return(
        Is_Regex_Match("^You gain (an at-will attack|a utility|a) power associated with your .+\\.$", description)
        );
}


static bool     Is_Add_Power(T_Glyph_Ptr name, T_Glyph_Ptr description, T_Glyph_Ptr class_name,
                                T_Glyph_Ptr atwill_equiv, T_Int32U * level, T_Int32U * level_second,
                                T_Int32U * power_level, T_Int32U * delete_level,
                                E_Power_Type * type)
{
    T_Glyph         regexp[500], level_buf[500], delete_buf[500], powertype[500], buffer[1000];

    *level = 0;
    *power_level = 0;
    *level_second = 0;
    *delete_level = 0;
    *type = e_type_at_will;

    /* Check our name for the type of power to add
    */
    sprintf(regexp, "^Level ([0-9]+) %s (Daily|Utility|Encounter|At-Will) Powers?$", class_name);
    if (!Is_Regex_Match(regexp, name, level_buf, powertype)) {

        /* If we have at-will equivalent powers, look for those instead - if
            we find it, that's an extra level 1 power at this level
        */
        if (atwill_equiv != NULL) {
            sprintf(regexp, "^Level ([0-9]+) (.+ )?%s", atwill_equiv);
            if (Is_Regex_Match(regexp, name, level_buf, powertype)) {
                *level = atoi(level_buf);
                *power_level = 1;
                *type = e_type_at_will;
                return(true);
                }
            }
        return(false);
        }

    /* Check our description for a power level to be deleted.
    */
    strcpy(regexp, "This new power replaces the (daily attack|utility|encounter attack|at-will attack) power you gained at ([0-9]+)");
    if (Is_Regex_Match(regexp, description, NULL, delete_buf))
        *delete_level = atoi(delete_buf);

    *level = atoi(level_buf);
    *power_level = *level;
    if (strcmp(powertype, "Daily") == 0)
        *type = e_type_daily;
    else if (strcmp(powertype, "Utility") == 0)
        *type = e_type_utility;
    else if (strcmp(powertype, "Encounter") == 0)
        *type = e_type_encounter;
    else if (strcmp(powertype, "At-Will") == 0)
        *type = e_type_at_will;
    else {
        sprintf(buffer, "Unknown power type found in %s - %s\n", name, class_name);
        Log_Message(buffer);
        }

    /* Check for two levels to be added, which is how the mage does it
    */
    sprintf(regexp, "You add two of the following powers to your spellbook\\.");
    if (Is_Regex_Match(regexp, description))
        *level_second = *level;

    return(true);
}


static void     Add_Power(T_Powers_Available * powers, T_Int32U add_level,
                            T_Int32U add_level_second, T_Int32U delete_level)
{
    T_Int32U        i, j;
    T_Glyph         buffer[500];

    /* If we need to delete a level, find it in the array and get rid of it
    */
    if (delete_level != 0) {
        for (i = 0; i < MAX_POWERS_PER_LEVEL; i++) {
            if (powers->slot[i] == delete_level) {

                /* Copy the remaining slots down over this one to erase it
                */
                for (j = i + 1; j < MAX_POWERS_PER_LEVEL; j++)
                    powers->slot[j - 1] = powers->slot[j];
                powers->slot[MAX_POWERS_PER_LEVEL - 1] = 0;
                break;
                }
            }
        if (x_Trap_Opt(i >= MAX_POWERS_PER_LEVEL)) {
            sprintf(buffer, "Couldn't delete level %lu power\n", delete_level);
            Log_Message(buffer);
            }
        }

    /* Now append this one into the first empty slot
    */
    for (i = 0; i < MAX_POWERS_PER_LEVEL; i++) {
        if (powers->slot[i] == 0) {
            powers->slot[i] = add_level;
            break;
            }
        }
    if (x_Trap_Opt(i >= MAX_POWERS_PER_LEVEL)) {
        sprintf(buffer, "No space to add level %lu power\n", add_level);
        Log_Message(buffer);
        }

    /* If we have a second level to add, do it
    */
    if (add_level_second != 0) {
        for (i = 0; i < MAX_POWERS_PER_LEVEL; i++) {
            if (powers->slot[i] == 0) {
                powers->slot[i] = add_level_second;
                break;
                }
            }
        if (x_Trap_Opt(i >= MAX_POWERS_PER_LEVEL)) {
            sprintf(buffer, "No space to add level %lu power\n", add_level_second);
            Log_Message(buffer);
            }
        }
}


static bool     Is_Level(T_Glyph_Ptr description, T_Int32U * level, T_Glyph_Ptr name)
{
    T_Glyph         level_buf[500];

    *level = 0;
    name[0] = '\0';

    if (!Is_Regex_Match("^Level ([0-9]+) (.+)$", description, level_buf, name))
        return(false);

    *level = atoi(level_buf);
    return(true);
}


static void Backfill_Powers(T_Powers_Available powers[MAX_LEVELS], T_Int32U last_level,
                                T_Int32U new_level)
{
    T_Int32U        i, j;

    if (x_Trap_Opt(new_level <= 1))
        return;

    /* Copy the slots for the last level that was set up to this level,
        including to this level, so they can be re-used / modified based on the
        new power distribution for this level
    */
    for (i = last_level + 1; i <= new_level; i++) {
        for (j = 0; j < MAX_POWERS_PER_LEVEL; j++) {
            powers[i].slot[j] = powers[last_level].slot[j];
            }
        }
}


static void Output_Power_Mechanics(T_XML_Node root, T_Class_Info * info, E_Power_Type type,
                                        T_Powers_Available powers[MAX_LEVELS])
{
    T_Int32U        i, j;
    T_Int32S        result;
    T_XML_Node      mechanics, arrayval;
    T_Glyph_Ptr     replace, hide;
    bool            is_powers;
    T_Glyph_Ptr     names[POWER_TYPE_COUNT] = { "%s At-Will Mechanics",
                                                "%s Encounter Mechanics",
                                                "%s Utility Mechanics",
                                                "%s Daily Mechanics",
                                                };
    T_Glyph_Ptr     ids[POWER_TYPE_COUNT] = { "%sPwrAtW",
                                                "%sPwrEnc",
                                                "%sPwrUti",
                                                "%sPwrDai",
                                                };
    T_Glyph_Ptr     replaces[POWER_TYPE_COUNT] = {  "PwrAtWill",
                                                    "PwrEncount",
                                                    "PwrUtility",
                                                    "PwrDaily",
                                                    };
    T_Glyph_Ptr     hides[POWER_TYPE_COUNT] = { "HidePwrAtW",
                                                "HidePwrEnc",
                                                "HidePwrUti",
                                                "HidePwrDai",
                                                 };
    T_Glyph         name[100], id[100];

    sprintf(name, names[type], info->name);
    sprintf(id, ids[type], Get_Class_Abbrev(info->id));
    replace = replaces[type];
    hide = hides[type];

    /* Create a new mechanics pick for the class / power type combo and
        bootstrap it to the class
    */
    result = Create_Thing_Node(root, "mechanics", "Mechanics", name, id, "",
                                UNIQUENESS_UNIQUE, NULL, &mechanics,
                                true);
    x_Trap_Opt(result != 0);
    Output_Bootstrap(info->node, id, info);

    /* Add important tags to the mechanics pick
    */
    Output_Tag(mechanics, "Mechanics", replace, info);
    Output_Tag(mechanics, "User", "NoBootMech", info);

    /* Set the appropriate fields on the mechanics pick
    */
    is_powers = false;
    for (i = 0; i < MAX_LEVELS; i++) {

//HACK - ignore Cavalier level 30 for daily powers, as they're done specially
//think of a better way to do this...
if ((type == e_type_daily) && (i == 30) && (strcmp(info->name, "Cavalier") == 0))
continue;

        for (j = 0; j < MAX_POWERS_PER_LEVEL; j++) {
            if (powers[i].slot[j] == 0)
                continue;
            result = XML_Create_Child(mechanics, "arrayval", &arrayval);
            x_Trap_Opt(result != 0);
            XML_Write_Text_Attribute(arrayval, "field", "ClPwrTable");
            XML_Write_Integer_Attribute(arrayval, "index", i);
            XML_Write_Integer_Attribute(arrayval, "column", j);
            XML_Write_Integer_Attribute(arrayval, "value", powers[i].slot[j]);
            is_powers = true;
            }
        }


    /* If we don't have any powers for this level, just hide the table
    */
    if (!is_powers)
        Output_Tag(info->node, "Hero", hide, info);
}


static void Fix_Feature_Name(string & text)
{
    T_Int32U        i, count;

    /* The first character should always be capitalized, so leave it so and
        start with #2
    */
    count = text.length();
    for (i = 1; i < count; i++) {

        /* This character should be lower case unless it's the first letter of
            a word
        */
        if (x_Is_Space(text[i-1]))
            text[i] = toupper(text[i]);
        else
            text[i] = tolower(text[i]);
        }
}


static void Get_Feature_Names(vector<string> * vec, T_Glyph_Ptr description)
{
    T_Void_Ptr      rx;
    string          match;
    bool            is_skip;

    /* Build our regexp
     */
    rx = Regex_Create("<b>([^a-z<]+)</b>");

    /* Prepare a vector with the indicies of the capture groups we want to
        iterate through
    */
    vector<int> v;
    v.push_back(1);

    /* Find all the matches in our text, adding them to the vector
    */
    match = Regex_Find_Match_Captures(rx, description, v);
    while (match.size() != 0) {

        /* Make sure we include at least one letter, otherwise we pick up
            things like <b>5-6</b> by accident
        */
        is_skip = true;
        for (auto str_iter = match.begin(); str_iter != match.end(); str_iter++)
            if (x_Is_Alpha(*str_iter)) {
                is_skip = false;
                break;
                }

        /* Convert the name to Title Case first, since that's what the unique
            id gets generated from
        */
        if (!is_skip) {
            Fix_Feature_Name(match);
            vec->push_back(match);
            }

        /* Incrementer the iterator once for each group we matched, now that
            we've used it
         */
        match = Regex_Next_Match_Capture(rx);
        }
}


static bool Is_Talent(T_Glyph_Ptr feature_name, T_Glyph_Ptr talent_names[],
                        T_Int32U talent_count)
{
    T_Int32U    i;

    for (i = 0; i < talent_count; i++) {
        if (stricmp(talent_names[i], feature_name) == 0)
            return(true);
        }
    return(false);
}


static void Output_Remaining_Features(T_Class_Info * info, T_XML_Node root, T_Glyph_Ptr class_id,
                                        T_Glyph_Ptr abbrev, T_Glyph_Ptr talent_names[],
                                        T_Int32U talent_count, vector<T_XML_Node> * list,
                                        T_List_Ids * id_list, C_Pool * pool)
{
    T_Int32U            i, count, level, delete_level, power_level, level_second;
    T_Int32U            last_level[POWER_TYPE_COUNT];
    T_Int32S            result;
    T_Glyph_Ptr         start, desc, power_id, equivalent_base, source, end, atwill_equiv, feature_name;
    T_XML_Node          feature;
    T_Power_Info *      power_info;
    E_Power_Type        power_type;
    T_Tuple *           mapping;
    bool                is_options, is_other_class, is_mechanics, is_essentials;
    vector<T_XML_Node>  feature_nodes;
    vector<string>      feature_names;
    T_Glyph             buffer[100000], buffer2[5000], buffer3[5000], name_buffer[1000], suffix[100];
    T_Powers_Available  powers[POWER_TYPE_COUNT][MAX_LEVELS];

    if ((info->features == NULL) || (info->features[0] == '\0')) {
        sprintf(buffer, "No class features found for class %s\n", info->name);
        Log_Message(buffer);
        return;
        }

    /* Skip past any puncutation and white space at the start
    */
    start = info->features;
    while (x_Is_Space(*start) || x_Is_Punct(*start))
        start++;

    /* We need to generate special mechanics picks if this is an essentials
        class, since they do weird things
    */
    equivalent_base = Mapping_Find("essbase", info->name);
    is_essentials = (equivalent_base != NULL);
    is_mechanics = is_essentials;

    /* Reset our powers arrays
    */
    memset(last_level, 0, sizeof(last_level));
    memset(powers, 0, sizeof(powers));

    /* If we have at-will equivalent powers, we start with 2 level 1 powers, so
        just get that worked out now
    */
    atwill_equiv = NULL;
    mapping = Mapping_Find_Tuple("atwilleqv", info->name);
    if (mapping != NULL) {
        atwill_equiv = mapping->b;
        powers[e_type_at_will][1].slot[0] = 1;
        powers[e_type_at_will][1].slot[1] = 1;
        last_level[e_type_at_will] = 1;
        Output_Field(info->node, "clsTrmAtW", atwill_equiv, info);
        Output_Field(info->node, "clsTrmAtWs", mapping->c, info);
        }

    /* For essentials classes, a lot of required feature names aren't included,
        so we need to try parsing them out of the description text - they look
        like <b>SLAYER'S DEFIANCE</b> etc
    */
    Get_Feature_Names(&feature_names, info->details);

    /* Now output all our features
    */
    count = feature_names.size();
    for (i = 0; i < count; i++) {
        feature_name = (T_Glyph_Ptr) feature_names[i].c_str();

        end = Get_Feature_Description(&desc, feature_name, info->details, pool);

        /* If this is a hybrid talent name, skip it
        */
        if (Is_Talent(feature_name, talent_names, talent_count))
            continue;

        /* Find the power id, if we have one, and pull it out of our power list
            so that we know this power has been taken care of.
        */
        power_id = Pull_Power_Id(feature_name, list);
        if (power_id != NULL) {

            /* If this feature replaces another feature, we need to make the
                power optional, so it still needs to be a feature. Otherwise
                we can just bootstrap the power directly.
            */
            if (strstr(desc, CLASS_FEATURE_REPLACES) == NULL) {
                result = Output_Bootstrap(info->node, power_id, info);
                x_Trap_Opt(result != 0);
                continue;
                }
            }

        /* If not, check that we don't explicitly reference a power
        */
        if (Is_Regex_Match("^You gain the (.+) power\\.", desc, buffer2)) {
            power_info = C_DDI_Powers::Get_Crawler()->Find_Item(buffer2);
            if (x_Trap_Opt(power_info == NULL)) {
                sprintf(buffer2, "Couldn't find power for class feature %s\n", feature_name);
                Log_Message(buffer2);
                continue;
                }

            /* Figure out if the power is for another class (essentials class
                equivalence doesn't count)
                NOTE: We need to get the 'base' class for the class, e.g.
                    Fighter, instead of the renamed Essentials version, e.g.
                    Weaponmaster, because powers are still "Fighter" powers.
            */
            is_other_class = stricmp(power_info->forclass, (equivalent_base == NULL) ? info->name : equivalent_base) != 0;

            /* Skip prereqs if we're for another class
            */
            result = Output_Bootstrap(info->node, power_info->id, info, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        is_other_class ? (T_Glyph_Ptr) "User" : NULL,
                                        is_other_class ? (T_Glyph_Ptr) "ClassOK" : NULL);
            x_Trap_Opt(result != 0);
            continue;
            }

        /* Check for powers that are only added if you take other selections,
            e.g. get power X if you take virtue Y
        */
        if (Is_Conditional_Power(desc))
            continue;

        /* If the feature is named something like "Level 11 Classname Daily/
            Utility Power", we need to track it as a general "power choice"
            entry and calculate it separately
        */
        if (Is_Add_Power(feature_name, desc, info->name, atwill_equiv, &level, &level_second,
                            &power_level, &delete_level, &power_type)) {
            if ((((T_Int32U) power_type) >= POWER_TYPE_COUNT) || (level >= MAX_LEVELS)) {
                sprintf(buffer2, "Bad power level or type for class feature %s\n", feature_name);
                Log_Message(buffer2);
                continue;
                }

            /* Fill in any gaps in the array
            */
            if (last_level[power_type] >= 1)
                Backfill_Powers(powers[power_type], last_level[power_type], level);
            last_level[power_type] = level;

            /* Set the details for this level
            */
            Add_Power(&powers[power_type][level], power_level, level_second, delete_level);
            continue;
            }

        /* Check for the level changing on other abilities - if it does,
            replace the name we use with the rest of the text in the name
        */
        suffix[0] = '\0';
        if (Is_Level(feature_name, &level, name_buffer)) {
            feature_name = name_buffer;
            sprintf(suffix, "%lu", level);
            }

        /* Now we have the name of the feature, we can try to generate a unique
            id for it.
            NOTE: We don't want it to try make up clever ids for class features
                because that messes things up for some reason, so specify false
                as the last parameter
        */
        sprintf(buffer2, "f%s", abbrev);
        if (!Generate_Thing_Id(buffer3, buffer2, feature_name, id_list, 3, suffix, false)) {
            sprintf(buffer2, "Error generating unique id for class feature %s\n", feature_name);
            Log_Message(buffer2);
            continue;
            }

        /* Add a bootstrap node for this feature
        */
        result = Output_Bootstrap(info->node, buffer3, info);
        if (x_Trap_Opt(result != 0))
            return;

        /* Output a new thing for this feature - note that it can't be unique,
            because some scripts on it need to be able to find its source.
        */
        Create_Thing_Node(root, "class feature", "ClassFeat",
                                feature_name, buffer3, desc,
                                UNIQUENESS_NONE, NULL, &feature, info->is_partial);
        feature_nodes.push_back(feature);

        Find_Nonhybrid_Version(root, feature, feature_name, info);

        /* Add a level activation tag if any
        */
        if (level != 0)
            Output_Tag(feature, "AtLevel", As_Int(level), info);

        /* Check for any 'appears in' source
        */
        source = NULL;
        Extract_Text(&source, pool, desc, NULL, "{i}Appears in ", "{/i}");
        if ((source != NULL) && (source[0] != '\0'))
            Output_Source(feature, source);

        /* Parse out any build options if we have them
        */
        is_options = false;
        if (end != NULL)
            is_options = Parse_Build_Options(root, info->node, abbrev, buffer3, end,
                                                id_list, pool, info);

        /* If we have options, add an appropriate field to this node
        */
        if (is_options) {
            sprintf(buffer2, "(component.BuildOpt) & (BuildOpt.%s)", buffer3);
            Output_Field(feature, "usrCandid1", buffer2, info);
            }

        /* If this feature is named "Ritual Casting", bootstrap the "Ritual
            Caster" feat and make sure we don't appear in the special ability
            list
        */
        if (Is_Regex_Match("Ritual Casting|Ritual Caster", feature_name)) {
            Output_Bootstrap(feature, "ftRituaCas", info);
            Output_Tag(feature, "Hide", "Special", info);
            }

        /* If this feature is named "Channel Divinity", add a script to add the
            appropriate tag to the hero
        */
        else if (stricmp(feature_name, "Channel Divinity") == 0)
            Output_Eval(feature, CHANNEL_DIVINITY_SCRIPT, "Setup", "10000", info);

        /* If we have a power id, that means this class feature bootstraps a
            power that only activates if the feature is turned on, so attach
            the bootstrap with an appropriate condition expression.
        */
        if (power_id != NULL) {
            result = Output_Bootstrap(feature, power_id, info, "fieldval:usrIsCheck <> 0",
                                        "Setup", "200");
            x_Trap_Opt(result != 0);
            }
        }

    count = feature_nodes.size();
    for (i = 0; i < count; i++)
        Check_Replacement_Features(root, feature_nodes[i], info);

    /* For essentials classes, build a list of all "recommended" powers so we
        can distinguish them from regular powers
    */
    if (is_essentials)
        Find_Recommended_Powers(info);

    /* Finish the mechanics array if necessary, backfilling up any last powers
        that we need to
    */
    if (is_mechanics)
        for (i = 0; i < POWER_TYPE_COUNT; i++) {
            if ((last_level[i] >= 1) && (last_level[i] < 30))
                Backfill_Powers(powers[i], last_level[i], 30);
            Output_Power_Mechanics(root, info, (E_Power_Type) i, powers[i]);
            }
}


static void Output_Hybrid_Talents(T_Class_Info * info, T_XML_Node root, T_Glyph_Ptr class_id,
                                        T_Glyph_Ptr abbrev, T_Glyph_Ptr chunks[],
                                        T_Int32U chunk_count, vector<T_XML_Node> * list,
                                        T_List_Ids * id_list, C_Pool * pool)
{
    T_Int32U            i;
    T_Int32S            result;
    T_Glyph_Ptr         temp, end;
    T_XML_Node          feature;
    T_Glyph             buffer2[5000], buffer3[5000];

    /* Now output all our features
    */
    for (i = 0; i < chunk_count; i++) {

        /* Find the power id, if we have one, and pull it out of our power list
            so that we know this power has been taken care of, then add a
            bootstrap for it.
        */
        temp = Pull_Power_Id(chunks[i], list);
        if (temp != NULL) {
            result = Output_Bootstrap(info->node, temp, info);
            x_Trap_Opt(result != 0);
            continue;
            }

        /* Now we have the name of the feature, we can try to generate a unique
            id for it.
        */
        sprintf(buffer2, "t%s", abbrev);
        if (!Generate_Thing_Id(buffer3, buffer2, chunks[i], id_list)) {
            sprintf(buffer2, "Error generating unique id for hybrid talent %s\n", chunks[i]);
            Log_Message(buffer2);
            continue;
            }

        /* Add a bootstrap node for this feature
        */
        result = Output_Bootstrap(info->node, buffer3, info);
        if (x_Trap_Opt(result != 0))
            return;

        /* Output a new thing for this feature - note that it can't be unique,
            because some scripts on it need to be able to find its source.
        */
        end = Get_Feature_Description(&temp, chunks[i], info->details, pool);
        Create_Thing_Node(root, "hybrid talent", "HybridTal",
                                chunks[i], buffer3, temp,
                                UNIQUENESS_NONE, NULL, &feature, info->is_partial);
        }
}


T_Status    C_DDI_Classes::Post_Process_Entry(T_XML_Node root, T_Class_Info * info)
{
    T_Status            status = LWD_ERROR;
    T_Int32U            i, chunk_count;
    T_Glyph_Ptr         start, class_id, abbrev, equivalent, equivalent_id, chunks[1000];
    vector<T_XML_Node>  feature_list;
    T_Glyph             talent_buffer[100000], buffer[1000], talents[500];

    /* Get our class id - we'll need it later
    */
    class_id = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(info->node, "id");
    if (x_Trap_Opt(class_id == NULL)) {
        sprintf(buffer, "Error retrieving unique id for class %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Get our abbreviation based on our class name
    */
    abbrev = Get_Class_Abbrev(class_id);
    if (x_Trap_Opt(abbrev == NULL)) {
        sprintf(buffer, "Couldn't get abbreviation for class %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* If we have an equivalent class, add a tag for it
    */
    equivalent = Mapping_Find("essclasses", info->name);
    if (equivalent != NULL) {
        equivalent_id = Mapping_Find("class", equivalent);
        if (x_Trap_Opt(equivalent_id == NULL)) {
            sprintf(buffer, "Couldn't get equivalent class id for class %s\n", info->name);
            Log_Message(buffer);
            }
        else
            Output_Tag(info->node, "Class", equivalent_id, info);
        }

    /* First, we want to find all the "feature" powers for this class. We can't
        bootstrap them yet, since some of them might be taken care of by the
        call to Output_Remaining_Features, below.
    */
    Find_Feature_Powers(info->node, class_id, &feature_list, info);

    /* Make a copy of our hybrid talents text, replacing " or " with ", " so we
        can easily chunk up our talents.
    */
    chunk_count = 0;
    if ((info->hybrid_talents != NULL) && (info->hybrid_talents[0] != '\0')) {
        Text_Find_Replace(talents, info->hybrid_talents, " or ", ", ");

        /* Skip past any puncutation and white space at the start
        */
        start = talents;
        while (x_Is_Space(*start) || x_Is_Punct(*start))
            start++;

        /* Build an array of our hybrid talent names so we can check it while
            outputting features
        */
        chunk_count = x_Array_Size(chunks);
        Split_To_Array(talent_buffer, chunks, &chunk_count, start, ",.");

        /* Get rid of any bad characters from the names first, to convert
            e.g. smart quotes into proper characters
        */
        for (i = 0; i < chunk_count; i++)
            Strip_Bad_Characters(chunks[i]);
        }

    /* Output any remaining features and hybrid talent stuff
    */
    Output_Remaining_Features(info, root, class_id, abbrev, chunks, chunk_count, &feature_list, &m_id_list, m_pool);
    Output_Hybrid_Talents(info, root, class_id, abbrev, chunks, chunk_count, &feature_list, &m_id_list, m_pool);

    /* Any powers left in the list weren't added by features, so bootstrap them
    */
    for (i = 0; i < feature_list.size(); i++)
        Output_Bootstrap(info->node, XML_Get_Attribute_Pointer(feature_list[i], "id"), info);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}