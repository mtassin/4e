/*  FILE:   OUTPUT_ITEMS.CPP

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


#define SUPERIOR_IMPL           "doneif (hero.isidentity[SuperImpl] = 0)\n" \
                                "perform assign[Helper.Proficient]\n" \
                                "perform assign[ImplemType.%s]"


static void         Output_Cost(T_Item_Info * info)
{
    T_Glyph         message[500];

    if ((info->cost == NULL) || (info->cost[0] == '\0')) {
        sprintf(message, "No cost found for item %s.\n", info->name);
        Log_Message(message);
        return;
        }

    Output_Field(info->node, "grCost", info->cost, info);
}


static void         Output_Weight(T_Item_Info * info)
{
    T_Double        weight;
    T_Glyph_Ptr     ptr, src;
    T_Glyph         buffer[5000];

    if ((info->weight == NULL) || (info->weight[0] == '\0'))
        return;

    /* make a backup copy of the weight without commas and get the weight from it
    */
    ptr = buffer;
    src = info->weight;
    while (*src != '\0') {
        if (*src != ',')
            *ptr++ = *src;
        src++;
        }
    *ptr = '\0';
    weight = atof(info->weight);

    Output_Field(info->node, "gearWeight", As_Float(weight), info);
}


static void         Output_Rarity(T_Item_Info * info)
{
    if ((info->rarity == NULL) || (info->rarity[0] == '\0'))
        return;

    Output_Tag(info->node, "Rarity", info->rarity, info);
}


static T_Glyph_Ptr Add_Property(T_Glyph_Ptr property_name)
{
    long                result;
    T_Glyph_Ptr         ptr;
    T_XML_Node          node;
    T_Glyph             property_id[UNIQUE_ID_LENGTH+1], message[1000];

    /* See if we have a 'standard' entry for this property - if so, just return
        its id
    */
    ptr = Mapping_Find("wepprop", property_name);
    if (ptr != NULL)
        return(ptr);

    /* Make sure the property name is capitalized
    */
    while (*property_name == ' ')
        property_name++;
    property_name[0] = toupper(property_name[0]);

    /* Search for a property with this name in our weapon properties group - if
        there's one already, we just need to get the id from it
    */
    result = XML_Get_First_Named_Child_With_Attr(Get_WepProp_Root(), "extgroup", "name",
                                                    property_name, &node);
    if (result == 0) {
        ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(node, "tag");
        return(ptr);
        }

    /* Otherwise, generate the unique id for this property's name
    */
    if (!Generate_Tag_Id(property_id, property_name, Get_WepProp_Root())) {
        sprintf(message, "No unique id could be generated for property '%s'\n", property_name);
        Log_Message(message);
        return(NULL);
        }

    /* Create a node for the new property and return its id
    */
    result = XML_Create_Child(Get_WepProp_Root(), "extgroup", &node);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not add weapon property tag node to group.\n");
        return(NULL);
        }
    XML_Write_Text_Attribute(node, "group", "WepProp");
    XML_Write_Text_Attribute(node, "tag", property_id);
    XML_Write_Text_Attribute(node, "name", property_name);
    ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(node, "tag");
    return(ptr);
}


static void         Output_Weapon_Properties(T_Item_Info * info, T_XML_Node target = NULL,
                                                bool is_assign_superior = false)
{
    T_Glyph_Ptr         ptr, end, prop_id;
    T_Glyph             backup;
    T_XML_Node          node;
    T_Glyph             message[1000];

    if (target == NULL)
        target = info->node;

    /* Each property name appears between a <br/> and a (. We can discard the
        rest of the text.
    */
    if ((info->properties == NULL) || (info->properties[0] == '\0'))
        return;
    ptr = strstr(info->properties, "<br/>");
    while (ptr != NULL) {
        ptr += 5;

        /* If this is just another html tag, ignore it
        */
        if (*ptr == '<')
            goto next_property;

        /* Find the end of the property name
        */
        end = strchr(ptr, '(');
        if (end == NULL) {
            sprintf(message, "Odd weapon property '%s' found for item %s.\n", ptr, info->name);
            Log_Message(message);
            goto next_property;
            }
        while (end[-1] == ' ')
            end--;

        /* If the weapon property is in our list of superior ones, we need to
            assign the "superior implement" tag if requested, then unset our
            flag to make sure we don't assign it multiple times
        */
        backup = *end;
        *end = '\0';
        if (is_assign_superior && Mapping_Find("superprop", ptr)) {
            Output_Tag(info->node, "Equipment", "SuperImpl", info);
            is_assign_superior = false;
            }

        /* We now have the property name at ptr. Add it as a thing, if it isn't
            there already, and output it if it isn't there already.
        */
        prop_id = Add_Property(ptr);
        *end = backup;
        if (prop_id != NULL) {
            if (XML_Get_First_Named_Child_With_Attr(target, "tag", "tag", prop_id, &node) != 0)
                Output_Tag(target, "WepProp", prop_id, info);
            }

        /* Go on to the next property
        */
next_property:
        ptr = strchr(ptr, ')');
        if (ptr != NULL)
            ptr = strstr(ptr, "<br/>");
        }
}


T_Status    C_DDI_Items::Output_Equipment(T_XML_Node root, T_Item_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph_Ptr     compset;
    T_Glyph         item_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* If this thing is from the monster manual, skip it - it's just junk like
        single arrows, single sling bullets, etc.
    */
    if ((info->source != NULL) && (strcmp(info->source, "Monster Manual") == 0))
        x_Status_Return_Success();

    /* Similarly, we don't care about lodgings, etc - that's not real gear.
    */
    if ((info->subcategory != NULL) && (strcmp(info->subcategory, "Lodging") == 0))
        x_Status_Return_Success();

    /* Generate a unique id for the equipment - if we can't, get out
    */
    if (!Generate_Thing_Id(item_id, "eq", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for item %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(item_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    compset = "Equipment";
    if ((info->subcategory != NULL) && (strcmp(info->subcategory, "Mount") == 0))
        compset = "Mount";
    result = Create_Thing_Node(root, m_term, compset, info->name, item_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, true);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    Output_Cost(info);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* Superior implements have properties, like weapons, and need an
        identifying tag
    */
    if (info->super_impl_type != NULL) {
        Output_Tag(info->node, "Equipment", "SuperImpl", info);
        sprintf(buffer, SUPERIOR_IMPL, info->super_impl_type);
        Output_Eval(info->node, buffer, "Setup", "75", info,
                    "Magic Weapon Copy", "Superior implement training");
        Output_Weapon_Properties(info);
        }

    /* Output our equipment type, unless we're a mount (in which case we don't
        need one).
    */
    if ((info->subcategory == NULL) || (strcmp(info->subcategory, "Mount") != 0))
        Output_Tag(info->node, "EquipType", info->subcategory, info, "equiptype");

    if ((info->subcategory != NULL) && (strcmp(info->subcategory, "Building") == 0))
        Output_Tag(info->node, "thing", "holder_top", info);

    Output_Weight(info);

    Output_Rarity(info);

    /* If we're a transportation item, add the top holder tag and make
        sure we never stack - otherwise, set our stackability to "merge" unless
        it's a mount (which doesn't stack).
    */
    if (info->subcategory != NULL) {
        if (stricmp(info->subcategory, "Transportation") == 0) {
            Output_Tag(info->node, "thing", "holder_top", info);
            XML_Write_Text_Attribute(info->node, "stacking", "never");
            }
        else if (stricmp(info->subcategory, "Mount") != 0)
            XML_Write_Text_Attribute(info->node, "stacking", "merge");
        }

cleanup_exit:
    x_Status_Return_Success();
}


static void         Output_Range(T_Item_Info * info)
{
    T_Int32U            chunk_count;
    T_Glyph_Ptr         chunks[1000];
    T_Glyph             buffer[100000], message[1000];

    /* If we have no range, or it's just "-", skip it
    */
    if ((info->range == NULL) || (info->range[0] == '\0') ||
        ((info->range[0] == '-') && (info->range[1] == '\0')))
        return;

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->range, "/");
    if (chunk_count < 2) {
        sprintf(message, "Odd weapon range (%s) found for item %s.\n", info->range, info->name);
        Log_Message(message);
        return;
        }

    /* Parse the first and second chunks as our short / long ranges
    */
    Output_Field(info->node, "wpShort", As_Int(atoi(chunks[0])), info);
    Output_Field(info->node, "wpLong", As_Int(atoi(chunks[1])), info);

    /* If we have more than two chunks, something is odd
    */
    if (chunk_count > 2) {
        sprintf(message, "Odd weapon range (%s) found for item %s.\n", info->range, info->name);
        Log_Message(message);
        }
}


static void         Output_Damage(T_Item_Info * info, T_XML_Node target = NULL)
{
    T_Int32U            chunk_count;
    T_Glyph_Ptr         chunks[1000];
    T_Glyph             buffer[100000], message[1000];

    if (target == NULL)
        target = info->node;

    /* If we have no damage
    */
    if ((info->damage == NULL) || (info->damage[0] == '\0'))
        return;

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->damage, "/");
    if (chunk_count == 0) {
        sprintf(message, "Odd weapon damage found for item %s.\n", info->name);
        Log_Message(message);
        return;
        }

    /* Parse the first and second chunks as our primary / secondary weapon
        damage - if we're hitting a different target, that means we're a
        secondary weapon too.
    */
    if (target != info->node)
        Output_Tag(target, "WepDamSec", chunks[0], info, "damage");
    else {
        Output_Tag(target, "WepDamage", chunks[0], info, "damage");
        if (chunk_count == 2)
            Output_Tag(target, "WepDamSec", chunks[1], info, "damage");
        }

    /* If we have more than two chunks, something is odd
    */
    if (chunk_count > 2) {
        sprintf(message, "Odd weapon damage found for item %s.\n", info->name);
        Log_Message(message);
        }
}


static void         Output_Weapon_Category(T_Item_Info * info)
{
    T_Int32U            chunk_count;
    T_Glyph_Ptr         chunks[1000];
    T_Glyph             buffer[100000], message[1000];

    /* If we have no damage
    */
    if ((info->subcategory == NULL) || (info->subcategory[0] == '\0')) {
        sprintf(buffer, "Error generating weapon category for item %s\n", info->name);
        Log_Message(buffer);
        return;
        }

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->subcategory, " -");
    if (chunk_count == 0) {
        sprintf(message, "Odd weapon category found for item %s.\n", info->name);
        Log_Message(message);
        return;
        }

    /* Parse the first chunk as our category - we need to combine that with
        whether the weapon is ranged or not
    */
    Output_Tag(info->node, "WepCat", chunks[0], info, "wepcat");
    if (chunk_count == 1) {
        sprintf(message, "Odd weapon category found for item %s.\n", info->name);
        Log_Message(message);
        return;
        }

    /* The second chunk is whether it's one handed or two
    */
    if (stricmp(chunks[1], "two") == 0)
        Output_Tag(info->node, "Equipment", "TwoHand", info);

    /* Ignore the rest, it's boring
    */
}


static void         Output_Weapon_Groups(T_Item_Info * info, T_XML_Node target = NULL)
{
    T_Glyph_Ptr         ptr, end;
    T_Glyph             backup;
    T_Glyph             message[1000];

    if (target == NULL)
        target = info->node;

    /* Each group name appears between a <br/> and a (. We can discard the
        rest of the text.
    */
    if ((info->groups == NULL) || (info->groups[0] == '\0'))
        return;
    ptr = strstr(info->groups, "<br/>");
    while (ptr != NULL) {
        ptr += 5;
        end = strchr(ptr, '(');
        if (end == NULL) {
            sprintf(message, "Odd weapon group found for item %s.\n", info->name);
            Log_Message(message);
            goto next_property;
            }
        while (end[-1] == ' ')
            end--;
        backup = *end;
        *end = '\0';

        /* We now have the property name at ptr. Add it as a thing, if it isn't
            there already, and output it.
        */
        Output_Tag(target, "WepGroup", ptr, info, "wepgroup");

        /* If we're in the bow, crossbow or sling group, change our compset to
            "Ranged"
        */
        if ((strcmp(ptr, "Bow") == 0) || (strcmp(ptr, "Crossbow") == 0) ||
            (strcmp(ptr, "Sling") == 0))
            XML_Write_Text_Attribute(target, "compset", "Ranged");

        /* Go on to the next property
        */
next_property:
        if (end != NULL)
            *end = backup;
        ptr = strstr(ptr, "<br/>");
        }
}


static T_XML_Node   Find_Weapon(T_XML_Node root, T_Glyph_Ptr name)
{
    T_Int32S            result;
    T_Glyph_CPtr        cptr;
    T_XML_Node          node;
    T_Glyph             buffer[500];

    /* Get the XML document for the powers list, and iterate through the powers
        in it.
    */
    result = XML_Get_First_Named_Child_With_Attr(root, "thing", "compset", "Melee", &node);
    while (result == 0) {
        cptr = XML_Get_Attribute_Pointer(node, "name");
        if (stricmp(name, cptr) == 0)
            return(node);

        result = XML_Get_Next_Named_Child_With_Attr(root, &node);
        }
    sprintf(buffer, "Error finding primary weapon for %s\n", name);
    Log_Message(buffer);
    return(NULL);
}


T_Status    C_DDI_Items::Output_Weapon(T_XML_Node root, T_Item_Info * info)
{
    T_Int32S        result;
    T_Glyph_Ptr     ptr, temp;
    T_XML_Node      node;
    T_Glyph         item_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Skip the 'unarmed attack' pick, since we already have one defined
    */
    if (stricmp(info->name, "Unarmed attack") == 0)
        x_Status_Return_Success();

    /* If this weapon has "- secondary end" on the end of its name, it's not
        a real weapon - it's the other end of a double weapon. This means we
        need to add its details to the original weapon with WepDamSec, WepGroup
        and WepProp tags.
    */
    temp = " - secondary end";
    if (stricmpright(info->name, temp) == 0) {

        /* Find the primary end of the weapon - we assume that it's already
            been output, since it will be alphabetically before this
        */
        strcpy(buffer, info->name);
        ptr = buffer + strlen(buffer) - strlen(temp);
        *ptr = '\0';
        node = Find_Weapon(root, buffer);
        if (node == NULL)
            x_Status_Return_Success();
        Output_Damage(info, node);
        Output_Weapon_Properties(info, node);
        Output_Weapon_Groups(info, node);
        Output_Tag(node, "WepProp", "Double", info);
        x_Status_Return_Success();
        }

    /* Generate a unique id for the weapon - if we can't, get out
    */
    if (!Generate_Thing_Id(item_id, "wp", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for item %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(item_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Melee", info->name, item_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, true);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* Add this weapon to our internal list of weapons
    */
    Mapping_Add("weapon", info->name, m_pool->Acquire(item_id));

    Output_Cost(info);

    Output_Weight(info);

    Output_Rarity(info);

    if ((info->proficient != NULL) && (info->proficient[0] != '\0'))
        Output_Field(info->node, "wpProf", As_Int(atoi(info->proficient)), info);

    /* If this is a dagger (and not the Parrying Dagger, which is its own
        thing), make sure it has the dagger implement type when proficient
    */
    if ((strstr(info->name, "dagger") != NULL) &&
        (stricmp(info->name, "Dagger") != 0) &&
        (stricmp(info->name, "Parrying Dagger") != 0)) {
        sprintf(buffer, SUPERIOR_IMPL, "wpDagger");
        Output_Eval(info->node, buffer, "Setup", "75", info,
                    "Magic Weapon Copy", "Superior implement training");

        /* For some reason we also need to specify the damage here
        */
        Output_Tag(info->node, "WepDamage", "1d4", info);
        }

    Output_Range(info);

    Output_Damage(info);

    Output_Weapon_Category(info);

    Output_Weapon_Properties(info, NULL, true);

    Output_Weapon_Groups(info);

cleanup_exit:
    x_Status_Return_Success();
}


static void Output_Special(T_XML_Node node, T_Glyph_Ptr special, T_Item_Info * info)
{
    T_Int32U        bonus;
    T_Glyph_Ptr     ptr, temp;
    bool            is_resist;

    ptr = special;

    /* Check for resistance
    */
    while (isspace(*ptr))
        ptr++;
    is_resist = strncmp(ptr, "Resist", 6) == 0;
    if (is_resist) {
        ptr += 6;
        while (isspace(*ptr))
            ptr++;
        }

    /* Parse out a number
    */
    while (*ptr == '+')
        ptr++;
    bonus = atoi(ptr);
    while ((*ptr != ' ') && (*ptr != '\0'))
        ptr++;
    if (*ptr == '\0')
        return;
    while (isspace(*ptr))
        ptr++;
    if (*ptr == '\0')
        return;

    /* Add the value to our field
    */
    temp = Mapping_Find("defarmspec", ptr);
    if (temp == NULL)
        return;
    Output_Field(info->node, temp, As_Int(bonus), info);
}


T_Status    C_DDI_Items::Output_Armor(T_XML_Node root, T_Item_Info * info)
{
    T_Int32S        result;
    T_Glyph         item_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* If this is cloth armor, fix up the name
    */
    if (strcmp(info->name, "Cloth Armor (Basic Clothing)") == 0)
        strcpy(info->name, "Cloth Armor");

    /* Generate a unique id for the armor - if we can't, get out
    */
    if (!Generate_Thing_Id(item_id, "ar", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for item %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(item_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Armor", info->name, item_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    Output_Cost(info);

    Output_Weight(info);

    Output_Rarity(info);

    if ((info->ac != NULL) && (info->ac[0] != '\0'))
        Output_Field(info->node, "arAC", As_Int(atoi(info->ac)), info);

    if ((info->armorcheck != NULL) && (info->armorcheck[0] != '\0'))
        Output_Field(info->node, "arCheck", As_Int(atoi(info->armorcheck)), info);

    if ((info->minenhance != NULL) && (info->minenhance[0] != '\0'))
        Output_Field(info->node, "arMinBonus", As_Int(atoi(info->minenhance)), info);

    if ((info->speed != NULL) && (info->speed[0] != '\0'))
        Output_Field(info->node, "arSpeed", As_Int(atoi(info->speed)), info);

    if ((info->special != NULL) && (info->special[0] != '\0'))
        Output_Special(info->node, info->special, info);

    /* If this is a shield, change our compset
    */
    if ((info->armortype != NULL) && ((stricmp(info->armortype, "Heavy Shields") == 0) ||
        (stricmp(info->armortype, "Light Shields") == 0)))
        XML_Write_Text_Attribute(info->node, "compset", "Shield");

    /* If we can confirm we're light armor by our name, add our light tag
    */
    if (Mapping_Find("lightarmor", info->name, true) != NULL)
        Output_Tag(info->node, "ArmorCat", "Light", info);

    /* Otherwise, check to see if we're heavy armor
    */
    else if (Mapping_Find("heavyarmor", info->name, true) != NULL)
        Output_Tag(info->node, "ArmorCat", "Heavy", info);

cleanup_exit:
    x_Status_Return_Success();
}


enum E_Pick_Type {
    e_pick_unknown,
    e_pick_skill,
    e_pick_defense,
    e_pick_resist,
    e_pick_surge_value,
    e_pick_trait,
};


struct T_Other_Info {
    T_Glyph_Ptr     pick;
    T_Int32U        value;
};


static void Parse_Other_Resists(T_Glyph_Ptr text, vector<T_Other_Info> * others)
{
    T_Glyph         backup;
    T_Glyph_Ptr     end, temp;
    T_Other_Info    other;
    bool            is_again = false;

    /* Find the end of the sentence - we don't want to match anything if the
        text is "you gain a +x bonus to y under some condition", only if it's a
        flat bonus.
    */
    end = strchr(text, '.');
    temp = strchr(text, '{');
    if ((end == NULL) || ((temp != NULL) && (temp < end)))
        end = temp;
    temp = strchr(text, ',');
    if ((end == NULL) || ((temp != NULL) && (temp < end))) {
        end = temp;
        is_again = true;
        }
    if (end != NULL) {
        backup = *end;
        *end = '\0';
        }
    while (*text == ' ')
        text++;

    /* Parse out the word "and"
    */
    if (strnicmp(text, "and", 3) == 0) {
        text += 3;
        while (*text == ' ')
            text++;
        }

    /* We must have the word 'resist'
    */
    if (strnicmp(text, "resist", 6) != 0)
        goto cleanup_exit;
    text += 6;
    while (*text == ' ')
        text++;

    /* Try to parse our value
    */
    other.value = atoi(text);
    if (other.value == 0)
        goto cleanup_exit;
    while ((*text != ' ') && (*text != '\0'))
        text++;
    while (*text == ' ')
        text++;

    /* Now parse out the resist value
    */
    temp = Mapping_Find("resist", text);
    if (temp != NULL) {
        other.pick = temp;
        others->push_back(other);
        }

cleanup_exit:
    if (end != NULL)
        *end = backup;

    /* If we found something like a comma, try to parse another resist after
        this one
    */
    if (is_again)
        Parse_Other_Resists(end+1, others);
}


static void Parse_Other_Skills(T_Glyph_Ptr text, vector<T_Other_Info> * others)
{
    T_Glyph         backup;
    T_Glyph_Ptr     end, temp, next;
    T_Other_Info    other;

    while (*text == ' ' || x_Is_Punct(*text))
        text++;

    /* Find the word 'checks' or a comma, which will probably terminate this
        skill, and skip the word 'and' if we find it
    */
    if (strnicmp(text, "and", 3) == 0) {
        text += 3;
        while (*text == ' ')
            text++;
        }
    end = strstr(text, "checks");
    if (end != NULL)
        next = end + 6;
    temp = strchr(text, ',');
    if ((end == NULL) || ((temp != NULL) && (temp < end))) {
        end = temp;
        next = end + 1;
        }
    temp = strchr(text, '.');
    if ((end == NULL) || ((temp != NULL) && (temp < end))) {
        end = temp;
        next = end + 1;
        }
    if (end == NULL)
        goto cleanup_exit;
    while (end[-1] == ' ')
        end--;
    if (end != NULL) {
        backup = *end;
        *end = '\0';
        }

    /* Parse out the next skill name
    */
    temp = Mapping_Find("skill", text);
    if (temp != NULL) {
        other.pick = temp;
        others->push_back(other);

        /* Try and parse another skill
        */
        Parse_Other_Skills(next, others);
        }

cleanup_exit:
    if (end != NULL)
        *end = backup;
}


static void Parse_Other_Defenses(T_Glyph_Ptr text, vector<T_Other_Info> * others)
{
    T_Glyph         backup;
    T_Glyph_Ptr     end, temp;
    T_Other_Info    other;

    while (*text == ' ')
        text++;

    /* Find the end of this defense, skipping an 'and' if we have it
    */
    if (strnicmp(text, "and", 3) == 0) {
        text += 3;
        while (*text == ' ')
            text++;
        }
    end = strchr(text, ',');
    temp = strchr(text, '.');
    if ((end == NULL) || ((temp != NULL) && (temp < end)))
        end = temp;
    if (end != NULL) {
        backup = *end;
        *end = '\0';
        }

    /* If we have 'defenses' at the end, skip it
    */
    if (stricmpright(text, "defenses") == 0) {
        temp = end - 8;
        *end = backup;
        end = temp;
        while (end[-1] == ' ')
            end--;
        backup = *end;
        *end = '\0';
        }

    /* Parse out the next defense name
    */
    temp = Mapping_Find("defense", text);
    if (temp != NULL) {
        other.pick = temp;
        others->push_back(other);

        /* Try and parse another defense
        */
        temp = end + 1;
        while (temp[1] == ' ')
            temp++;
        Parse_Other_Defenses(temp, others);
        }

    if (end != NULL)
        *end = backup;
}


static T_Glyph_Ptr  Parse_Pick(T_Glyph_Ptr text, T_Glyph_Ptr * pick, E_Pick_Type * type,
                                vector<T_Other_Info> * others, bool * is_bonus_enhance)
{
    T_Int32U        i, count;
    T_Glyph         backup, cutoff_backup, enhance_backup;
    T_Glyph_Ptr     end, next, temp, test, cutoff, enhance;
    T_Glyph_Ptr     cutoffs[] = { "checks", "defense", };
    T_Glyph_Ptr     enhancebonus[] = {  "equal to the item's enhancement bonus",
                                        "equal to the item's  enhancement bonus",
                                        "equal to this item's enhancement bonus",
                                        "equal to this armor's enhancement bonus",
                                        "equal to the cloak's enhancement bonus",
                                        "equal to the trinket's enhancement bonus",
                                        "equal to the locket's enhancement bonus",
                                        "equal to the ornament's enhancement bonus",
                                        "equal to the periapt's enhancement bonus",
                                        "equal to the orb's enhancement bonus",
                                        "equal to the armor's enhancement bonus",
                                        "equal to the weapon's enhancement bonus",
                                        "equal to the staff's enhancement bonus",
                                        "equal to the staff 's enhancement bonus",
                                        "equal to the enhancement bonus of this", };
    T_Other_Info    other;

    next = NULL;
    end = NULL;
    backup = '\0';
    cutoff = NULL;
    cutoff_backup = '\0';
    enhance = NULL;
    enhance_backup = '\0';

    /* First, search for our item bonus keywords
    */
    count = x_Array_Size(enhancebonus);
    for (i = 0; i < count; i++) {
        enhance = strstr(text, enhancebonus[i]);
        if (enhance != NULL) {
            *is_bonus_enhance = true;
            while (enhance[-1] == ' ')
                enhance--;
            enhance_backup = *enhance;
            *enhance = '\0';
            break;
            }
        }

    /* Find the end of the sentence - we don't want to match anything if the
        text is "you gain a +x bonus to y under some condition", only if it's a
        flat bonus.
    */
    end = strchr(text, '.');
    temp = strchr(text, '{');
    if ((end == NULL) || ((temp != NULL) && (temp < end)))
        end = temp;
    temp = strchr(text, ',');
    if ((end == NULL) || ((temp != NULL) && (temp < end))) {
        end = temp;
        if (end != NULL)
            next = end + 1;
        }
    temp = strstr(text, " and ");
    if ((end == NULL) || ((temp != NULL) && (temp < end))) {
        end = temp;
        if (end != NULL)
            next = end + 5;
        }
    if (end == NULL)
        end = text + strlen(text);
    else {
        backup = *end;
        *end = '\0';
        }

    /* If the last word is a known one, get rid of it
    */
    count = x_Array_Size(cutoffs);
    for (i = 0; i < count; i++) {
        if (stricmpright(text, cutoffs[i]) == 0) {
            cutoff = end - strlen(cutoffs[i]);
            while (cutoff[-1] == ' ')
                cutoff--;
            cutoff_backup = *cutoff;
            *cutoff = '\0';
            break;
            }
        }

    /* Test some fixed stuff
    */
    test = "healing surge value";
    if (stricmp(text, test) == 0) {
        end = text + strlen(test);
        *pick = "trSurgeVal";
        *type = e_pick_surge_value;
        goto cleanup_exit;
        }
    test = "defenses";
    if (stricmp(text, test) == 0) {
        end = text + strlen(test);
        *pick = "defAC";
        *type = e_pick_defense;
        other.pick = "defFort";
        others->push_back(other);
        other.pick = "defRef";
        others->push_back(other);
        other.pick = "defWill";
        others->push_back(other);
        goto cleanup_exit;
        }
    test = "hit points";
    if (stricmp(text, test) == 0) {
        end = text + strlen(test);
        *pick = "trSurgeVal";
        *type = e_pick_surge_value;
        goto cleanup_exit;
        }
    test = "saving throws";
    if (stricmp(text, test) == 0) {
        end = text + strlen(test);
        *pick = "trSave";
        *type = e_pick_trait;
        goto cleanup_exit;
        }
    test = "speed";
    if (stricmp(text, test) == 0) {
        end = text + strlen(test);
        *pick = "trSpeed";
        *type = e_pick_trait;
        goto cleanup_exit;
        }
    test = "initiative";
    if (stricmp(text, test) == 0) {
        end = text + strlen(test);
        *pick = "trInit";
        *type = e_pick_trait;
        goto cleanup_exit;
        }
    test = "initiative checks";
    if (stricmp(text, test) == 0) {
        end = text + strlen(test);
        *pick = "trInit";
        *type = e_pick_trait;
        goto cleanup_exit;
        }

    /* If it should be a resistance, try and match it
    */
    if (*type == e_pick_resist) {
        temp = Mapping_Find("resist", text);
        if (temp != NULL) {
            *pick = temp;
            text = (end == NULL) ? (T_Glyph_Ptr) "" : end+1;

            /* Try and parse out other resists, then get out
            */
            if (next != NULL)
                Parse_Other_Resists(next, others);
            goto cleanup_exit;
            }
        }

    /* Is it a skill?
    */
    temp = Mapping_Find("skill", text);
    if (temp != NULL) {
        *pick = temp;
        *type = e_pick_skill;
        text = (end == NULL) ? (T_Glyph_Ptr) "" : end+1;

        /* Try and parse out other skills, then get out
        */
        if (next != NULL)
            Parse_Other_Skills(next, others);
        goto cleanup_exit;
        }

    /* Is it a defense?
    */
    temp = Mapping_Find("defense", text);
    if (temp != NULL) {
        *pick = temp;
        *type = e_pick_defense;
        text = (end == NULL) ? (T_Glyph_Ptr) "" : end+1;

        /* Try and parse out other defenses, then get out
        */
        if (next != NULL)
            Parse_Other_Defenses(next, others);
        goto cleanup_exit;
        }

cleanup_exit:
    if (end != NULL)
        *end = backup;
    if (cutoff != NULL)
        *cutoff = cutoff_backup;
    if (enhance != NULL)
        *enhance = enhance_backup;
    return(text);
}


static void Output_Item_Properties(T_Item_Info * info, bool is_check_parent)
{
    T_Int32U                i;
    T_Int32S                value, temp;
    T_Glyph_Ptr             ptr, property, field, pick, parent;
    vector <T_Other_Info>   others;
    E_Pick_Type             type;
    bool                    is_bonus_enhance;
    T_Glyph                 buffer[10000], buffer2[10000];

    /* If we don't have anything to extract from our description, assign our
        "not interesting magic item" tag and get out
    */
    if ((info->description == NULL) || (info->description[0] == '\0')) {
        Output_Tag(info->node, "Hide", "Special", info);
        return;
        }

    /* If we have to check whether our parent is equipped, prepare something
        to insert into the script appropriately
    */
    parent = (T_Glyph_Ptr) (is_check_parent ? "parent." : "");

    /* If we don't have any properties (i.e. just powers), assign our "not
        interesting magic item" tag and get out
    */
    property = "Property:";
    ptr = strstr(info->description, property);
    if (ptr == NULL) {
        property = "{b}Property{/b}:";
        ptr = strstr(info->description, property);
        if (ptr == NULL) {
            property = "Properties:";
            ptr = strstr(info->description, property);
            if (ptr == NULL) {
                Output_Tag(info->node, "Hide", "Special", info);
                return;
                }
            }
        }
    while (ptr != NULL) {
        type = e_pick_unknown;
        field = NULL;
        pick = NULL;
        is_bonus_enhance = false;
        value = 0;
        others.empty();
        ptr += strlen(property);
        while (*ptr == ' ')
            ptr++;
        if (strnicmp(ptr, "You", 3) == 0)
            ptr += 3;
        while (*ptr == ' ')
            ptr++;
        if (strnicmp(ptr, "Gain an", 7) == 0)
            ptr += 7;
        else if (strnicmp(ptr, "Gain a", 6) == 0)
            ptr += 6;
        else if (strnicmp(ptr, "Gain one", 8) == 0) {
            ptr += 8;
            value = 1;
            }
        else if (strnicmp(ptr, "Gain", 4) == 0)
            ptr += 4;
        else if (strnicmp(ptr, "Increase your maximum hit points by", 35) == 0) {
            ptr += 35;
            type = e_pick_trait;
            pick = "trHealth";
            field = "trtBonus";
            }
        else if (strnicmp(ptr, "Resist", 6) == 0)
            /* do nothing - this is handled below */;
        else
            goto next_property;
        while (*ptr == ' ')
            ptr++;

        if (strnicmp(ptr, "resist", 6) == 0) {
            ptr += 6;
            type = e_pick_resist;
            }
        while (*ptr == ' ')
            ptr++;

        /* If we have a number here, parse it out - there might not be one (in
            which case there should be a reference to increasing stuff by our
            enhancement bonus later).
        */
        temp = atoi(ptr);
        if (temp != 0) {
            value = temp;
            while ((*ptr != ' ') && (*ptr != '\0'))
                ptr++;
            while (*ptr == ' ')
                ptr++;
            }

        /* Check for stuff that can come after the number
        */
        if (strnicmp(ptr, "healing surge", 13) == 0) {
            ptr += 13;
            type = e_pick_trait;
            pick = "trSurges";
            field = "trtItem";
            }

        /* Which field to use
        */
        if (type == e_pick_resist)
            field = "rsTotal";
        else if ((strnicmp(ptr, "item bonus to", 13) == 0) || (strnicmp(ptr, "item bonus on", 13) == 0)) {
            ptr += 13;
            field = "trtItem";
            }
        else if ((strnicmp(ptr, "power bonus to", 14) == 0) || (strnicmp(ptr, "power bonus on", 14) == 0)) {
            ptr += 14;
            field = "trtPower";
            }
        else if ((strnicmp(ptr, "shield bonus to", 15) == 0) || (strnicmp(ptr, "shield bonus on", 15) == 0)) {
            ptr += 15;
            field = "trtShield";
            }
        else if ((strnicmp(ptr, "bonus to", 8) == 0) || (strnicmp(ptr, "bonus on", 8) == 0)) {
            ptr += 8;
            field = "trtBonus";
            }
        else if (strnicmp(ptr, "extra", 5) == 0) {
            ptr += 5;
            field = "trtBonus";
            }
        else if (pick == NULL)
            goto next_property;
        while (*ptr == ' ')
            ptr++;

        /* Consume the words 'your' or 'all' if we have them
        */
        if (strnicmp(ptr, "all", 3) == 0)
            ptr += 3;
        if (strnicmp(ptr, "your", 4) == 0)
            ptr += 4;
        while (*ptr == ' ')
            ptr++;

        /* Parse a pick out of the stream if necessary
        */
        if (pick == NULL)
            ptr = Parse_Pick(ptr, &pick, &type, &others, &is_bonus_enhance);
        if (is_bonus_enhance)
            value = (info->enhbonus == NULL) ? 0 : atoi(info->enhbonus);
        if ((pick == NULL) || (type == e_pick_unknown) || (value == 0))
            goto next_property;

        /* Emit an appropriate eval script
        */
        switch (type) {
            case e_pick_skill :
            case e_pick_defense :
            case e_pick_trait :
                sprintf(buffer, "\ndoneif (%stagis[Equipped.Equipped] = 0)\n#traitmodify[%s,%s,%ld,\"\"]\n", parent, pick, field, value);
                for (i = 0; i < others.size(); i++) {
                    sprintf(buffer2, "#traitmodify[%s,%s,%ld,\"\"]\n", others[i].pick, field, value);
                    strcat(buffer, buffer2);
                    }
                Output_Eval(info->node, buffer, "Traits", "1000", info, "Derived trtFinal");
                break;
            case e_pick_resist :
                sprintf(buffer, "\ndoneif (%stagis[Equipped.Equipped] = 0)\n#traitmodify[%s,%s,%ld,\"\"]\n", parent, pick, field, value);
                for (i = 0; i < others.size(); i++) {
                    sprintf(buffer2, "#traitmodify[%s,%s,%lu,\"\"]\n", others[i].pick, field, others[i].value);
                    strcat(buffer, buffer2);
                    }
                Output_Eval(info->node, buffer, "Traits", "1000", info, "Derived trtFinal");
                break;
            case e_pick_surge_value :
                sprintf(buffer, "\ndoneif (%stagis[Equipped.Equipped] = 0)\n#traitmodify[%s,%s,%ld,\"\"]\n", parent, pick, field, value);
                Output_Eval(info->node, buffer, "Traits", "1000", info, "trSurgeVal final");
                break;
            default :
                x_Trap_Opt(1);
                break;
            }

next_property:
        ptr = strstr(ptr, property);
        }
}


static void     Output_Magic_General(T_Item_Info * info)
{
    bool            is_enhancement_bonus = false;
    T_Glyph         buffer[10000], tempflat[100], tempdice[100], temp10[100], temp10plus[100];

    /* Build the enhancement bonus string for this level of weapon
    */
    if (!info->is_partial) {
        if ((info->enhancement != NULL) && (info->enhancement[0] != '\0')) {
            if ((info->enhbonus == NULL) || (info->enhbonus[0] == '\0') ||
                (strcmp(info->enhbonus, "0") == 0) || (info->enhancement[0] == '+'))
                strcpy(buffer, info->enhancement);
            else {
                is_enhancement_bonus = true;
                sprintf(buffer, "+%s %s", info->enhbonus, info->enhancement);
                }
            Output_Field(info->node, "mgEnhance", buffer, info);
            }
        }

    /* If a level wasn't specified, just call us level 99 so the item sorts
        to the end of the choose list instead of the front
    */
    if ((info->level != NULL) && (info->level[0] != '\0'))
        Output_Field(info->node, "mgLevel", info->level, info);
    else
        Output_Field(info->node, "mgLevel", "99", info);

    Output_Rarity(info);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial)
        return;

    if ((info->flavor != NULL) && (info->flavor[0] != '\0'))
        Output_Field(info->node, "mgFlavor", info->flavor, info);

    if ((info->enhbonus != NULL) && (info->enhbonus[0] != '\0'))
        Output_Field(info->node, "mgBonus", info->enhbonus, info);

    /* If our crit bonus field has any "per plus"es in it, remove them and
        replace +1 with +x based on the enhancement bonus
    */
    if ((info->critical != NULL) && (info->critical[0] != '\0')) {
        if (!is_enhancement_bonus || (strstr(info->critical, " per plus") == NULL))
            Output_Field(info->node, "mgCrit", info->critical, info);
        else {
            sprintf(tempdice, "+%sd", info->enhbonus);
            sprintf(temp10, " %s0 damage", info->enhbonus);
            sprintf(temp10plus, "+%s0 damage", info->enhbonus);
            sprintf(tempflat, "+%s ", info->enhbonus);
            T_Glyph_Ptr     find[] = {  "+1d", " 10 damage per plus", "+10 damage per plus", " per plus", "+1 " };
            T_Glyph_Ptr     replace[] = { tempdice, temp10, temp10plus, "", tempflat };
            Text_Find_Replace(buffer,info->critical,strlen(info->critical),
                                x_Array_Size(find),find,replace);
            Output_Field(info->node, "mgCrit", buffer, info);
            }
        }
}


T_Status    C_DDI_Items::Output_Magic_Weapon(T_XML_Node root, T_Item_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph_Ptr     suffix;
    T_Glyph         item_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Get the suffix to use for this item - normally it's the enhancement
        bonus, but for items with no enhancement bonuses it's just the level.
        We append the level to all item ids since some may get other versions
        in the future, even if there's only one of them now.
    */
    if (info->is_artifact)
        suffix = "";
    else
        suffix = ((info->enhbonus == NULL) || (info->enhbonus[0] == '\0')) ? info->level : info->enhbonus;

    /* Generate a unique id for the magic weapon - if we can't, get out
    */
    if (!Generate_Thing_Id(item_id, "mw", info->name, &m_id_list, 3, suffix)) {
        sprintf(buffer, "Error generating unique id for item %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(item_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "MagicWep", info->name, item_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, true);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    Output_Magic_General(info);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* Output any item properties, such as resistances
    */
    Output_Item_Properties(info, true);

cleanup_exit:
    x_Status_Return_Success();
}


T_Status    C_DDI_Items::Output_Magic_Armor(T_XML_Node root, T_Item_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph_Ptr     suffix;
    T_Glyph         item_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Get the suffix to use for this item - normally it's the enhancement
        bonus, but for items with no enhancement bonuses it's just the level.
        We append the level to all item ids since some may get other versions
        in the future, even if there's only one of them now.
    */
    if (info->is_artifact)
        suffix = "";
    else
        suffix = ((info->enhbonus == NULL) || (info->enhbonus[0] == '\0')) ? info->level : info->enhbonus;

    /* Generate a unique id for the magic armor - if we can't, get out
    */
    if (!Generate_Thing_Id(item_id, "ma", info->name, &m_id_list, 3, suffix)) {
        sprintf(buffer, "Error generating unique id for item %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(item_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "MagicArmor", info->name, item_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, true);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    Output_Magic_General(info);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* Output any item properties, such as resistances
    */
    Output_Item_Properties(info, true);

cleanup_exit:
    x_Status_Return_Success();
}


T_Status    C_DDI_Items::Output_Magic_Item(T_XML_Node root, T_Item_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph_Ptr     suffix, compset;
    bool            is_staff = false;
    T_Glyph         item_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Get the suffix to use for this item - normally it's the enhancement
        bonus, but for items with no enhancement bonuses it's just the level.
        We append the level to all item ids since some may get other versions
        in the future, even if there's only one of them now.
    */
    if (info->is_artifact)
        suffix = "";
    else
        suffix = ((info->enhbonus == NULL) || (info->enhbonus[0] == '\0')) ? info->level : info->enhbonus;

    /* Generate a unique id for the magic item - if we can't, get out
    */
    if (!Generate_Thing_Id(item_id, "mg", info->name, &m_id_list, 3, suffix)) {
        sprintf(buffer, "Error generating unique id for item %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(item_id);

    /* Everything is a MagicItem, except for staffs, which are special
    */
    compset = "MagicItem";
    if ((info->implement != NULL) && (strcmp(info->implement, "Staff") == 0)) {
        compset = "MagicStaff";
        is_staff = true;
        }

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, compset, info->name, item_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, true);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    Output_Magic_General(info);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* Output the item slot or implement type unless this is a staff (in which
        case it's pretty obvious).
    */
    if (!is_staff) {
        if ((info->implement != NULL) && (info->implement[0] != '\0'))
            Output_Tag(info->node, "ImplemType", info->implement, info, "impltype");
        else if ((info->itemcategory != NULL) && (info->itemcategory[0] != '\0')) {
            Output_Tag(info->node, "ItemSlot", info->itemcategory, info, "itemslot");

            /* Consumables etc default to merge stacking
            */
            if ((stricmp(info->itemcategory, "Potion") == 0) ||
                (stricmp(info->itemcategory, "Alchemical") == 0) ||
                (stricmp(info->itemcategory, "Ammunition") == 0))
                XML_Write_Text_Attribute(info->node, "stacking", "merge");
            }
        }

    Output_Item_Properties(info, false);

cleanup_exit:
    x_Status_Return_Success();
}


T_Status    C_DDI_Items::Output_Entry(T_XML_Node root, T_Item_Info * info)
{
    /* Skip the "Item Set" category for now, as it needs special handling
    */
    if (stricmp(info->itemcategory, "item set") == 0)
        x_Status_Return_Success();

    if (info->filename != NULL) {
        if ((strcmp(info->filename, FILE_EQUIPMENT) == 0) || (strcmp(info->filename, FILE_MOUNTS) == 0))
            x_Status_Return(Output_Equipment(root, info));
        if (strcmp(info->filename, FILE_WEAPONS) == 0)
            x_Status_Return(Output_Weapon(root, info));
        if (strcmp(info->filename, FILE_ARMOR) == 0)
            x_Status_Return(Output_Armor(root, info));
        if (strcmp(info->filename, FILE_WEAPONS_MAGIC) == 0)
            x_Status_Return(Output_Magic_Weapon(root, info));
        if (strcmp(info->filename, FILE_ARMOR_MAGIC) == 0)
            x_Status_Return(Output_Magic_Armor(root, info));
        }
    x_Status_Return(Output_Magic_Item(root, info));
}


static void Find_Armor_Type(T_XML_Node root, T_Glyph_Ptr name, T_Glyph_Ptr * type_id,
                            T_Glyph_Ptr * type_name)
{
    long            result;
    T_Glyph_Ptr     ptr;
    T_XML_Node      node;
    T_Glyph         findtype[1000];

    *type_id = NULL;
    *type_name = NULL;

    /* Chainmail is stupid
    */
    if (stricmp(name, "Chain") == 0) {
        *type_id = "arChainmai";
        *type_name = "Chainmail";
        return;
        }

    strcpy(findtype, name);
    result = XML_Get_First_Named_Child_With_Attr(root, "thing", "name", findtype, &node);
    if (result == 0)
        goto cleanup_exit;

    /* We didn't find it, so try taking 'Armor' off the name if it has it
    */
    if (stricmpright(findtype, "Armor") == 0) {
        ptr = findtype + strlen(findtype) - strlen("Armor");
        while (ptr[-1] == ' ')
            ptr--;
        *ptr = '\0';
        result = XML_Get_First_Named_Child_With_Attr(root, "thing", "name", findtype, &node);
        if (result == 0)
            goto cleanup_exit;
        }

    /* Otherwise, put "Armor" on the end of the name and try that
    */
    else {
        strcat(findtype, " Armor");
        result = XML_Get_First_Named_Child_With_Attr(root, "thing", "name", findtype, &node);
        if (result == 0)
            goto cleanup_exit;
        }

    /* Finally, try taking any "s" off the end
    */
    strcpy(findtype, name);
    if (stricmpright(findtype, "s") == 0) {
        ptr = findtype + strlen(findtype) - 1;
        while (ptr[-1] == ' ')
            ptr--;
        *ptr = '\0';
        result = XML_Get_First_Named_Child_With_Attr(root, "thing", "name", findtype, &node);
        if (result == 0)
            goto cleanup_exit;
        }

cleanup_exit:
    if (result == 0) {
        *type_id = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(node, "id");
        *type_name = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(node, "name");
        }
}


T_Status    C_DDI_Items::Post_Process_Armor(T_XML_Node root, T_Item_Info * info)
{
    T_Glyph_Ptr         type_id, type_name, our_id;
    T_Glyph             buffer[1000];

    /* If we have no type, we're done
    */
    if ((info->armortype == NULL) || (info->armortype[0] == '\0'))
        x_Status_Return_Success();

    /* Otherwise, we're a type of another sort of armor. Try to find that kind
        of armor in our XML document.
    */
    Find_Armor_Type(root, info->armortype, &type_id, &type_name);

    /* If we still didn't find a match, report an error
    */
    if (type_id == NULL) {
        sprintf(buffer, "Couldn't match armor type '%s' for armor %s\n", info->armortype, info->name);
        Log_Message(buffer);
        x_Status_Return_Success();
        }

    /* We found the right armor type, so add a tag for it - unless it's got the
        same unique id as we have, in which case, skip it.
    */
    our_id = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(info->node, "id");
    if (strcmp(type_id, our_id) != 0) {
        Output_Tag(info->node, "ArmorType", type_id, info);

        /* See if we match light armor - if so, add a light armor tag
        */
        if (Mapping_Find("lightarmor", type_name, true) != NULL)
            Output_Tag(info->node, "ArmorCat", "Light", info);

        /* See if we match heavy armor - if so, add a heavy armor tag
        */
        else if (Mapping_Find("heavyarmor", type_name, true) != NULL)
            Output_Tag(info->node, "ArmorCat", "Heavy", info);
        }
    x_Status_Return_Success();
}


T_Status    C_DDI_Items::Post_Process_Magic_Weapon(T_XML_Node root, T_Item_Info * info)
{
    T_Glyph_Ptr         splitwords[] = { " or " };

    /* If we have no weapon requirement, there's nothing to do - we're suitable
        for all weapons
    */
    if ((info->weaponreq == NULL) || (stricmp(info->weaponreq, "Any") == 0))
        x_Status_Return_Success();
    if (stricmp(info->weaponreq, "Any melee") == 0) {
        Output_Tag(info->node, "ReqWep", "AnyMelee", info);
        x_Status_Return_Success();
        }
    if (stricmp(info->weaponreq, "Any ranged") == 0) {
        Output_Tag(info->node, "ReqWep", "AnyRange", info);
        x_Status_Return_Success();
        }
    if (stricmp(info->weaponreq, "Any thrown") == 0) {
        Output_Tag(info->node, "ReqWep", "AnyThrow", info);
        x_Status_Return_Success();
        }

    /* Otherwise, we're limited to a subset of weapons.
    */
    Split_To_Tags(info->node, info, info->weaponreq, ",.", "ReqWep", "wepgroup", "weapon",
                    false, NULL, splitwords, x_Array_Size(splitwords));

    x_Status_Return_Success();
}


T_Status    C_DDI_Items::Post_Process_Magic_Armor(T_XML_Node root, T_Item_Info * info)
{
    T_Int32U            i, chunk_count;
    T_Glyph_Ptr         type_id, type_name, chunks[1000];
    T_XML_Node          armor_root;
    T_Glyph_Ptr         splitwords[] = { " or " };
    T_Glyph             buffer[100000], message[1000];

    /* If we have no weapon requirement, there's nothing to do - we're suitable
        for all weapons
    */
    if ((info->armorreq == NULL) || (stricmp(info->armorreq, "Any") == 0))
        x_Status_Return_Success();

    /* Find the armor root in our list of root nodes
    */
    armor_root = NULL;
    for (i = 0; i < MAX_XML_CONTAINERS; i++) {
        if (stricmp(m_docs[i].filename, FILE_ARMOR) == 0) {
            armor_root = m_docs[i].root;
            break;
            }
        }
    if (x_Trap_Opt(armor_root == NULL)) {
        Log_Message("Couldn't find armor document root node!\n");
        x_Status_Return_Success();
        }

    /* Chunk our text
    */
    chunk_count = x_Array_Size(info->armorreq);
    Split_To_Array(buffer, chunks, &chunk_count, info->armorreq, ",.",
                    NULL, splitwords, x_Array_Size(splitwords));

    /* Find an appropriate tag for each chunk
    */
    for (i = 0; i < chunk_count; i++) {
        chunks[i][0] = toupper(chunks[i][0]);
        Find_Armor_Type(armor_root, chunks[i], &type_id, &type_name);
        if (type_id == NULL) {
            sprintf(message, "Couldn't match armor type '%s' for magic armor %s\n", chunks[i], info->name);
            Log_Message(message);
            continue;
            }
        Output_Tag(info->node, "ReqArmor", type_id, info);
        }

    x_Status_Return_Success();
}


T_Status    C_DDI_Items::Post_Process_Entry(T_XML_Node root, T_Item_Info * info)
{
    T_Int32U        i;
    T_Power_Info *  power_info;

    /* Add bootstraps for all linked powers, now that they've been output and
        have ids
    */
    for (i = 0; i < info->power_count; i++) {
        power_info = Get_Power(info->powers[i]);
        if (x_Trap_Opt(power_info == NULL))
            continue;
        Output_Bootstrap(info->node, power_info->id, info);
        }

    /* Do any other post-processing
    */
    if (info->filename != NULL) {
        if (strcmp(info->filename, FILE_ARMOR) == 0)
            x_Status_Return(Post_Process_Armor(root, info));
        if (strcmp(info->filename, FILE_WEAPONS_MAGIC) == 0)
            x_Status_Return(Post_Process_Magic_Weapon(root, info));
        if (strcmp(info->filename, FILE_ARMOR_MAGIC) == 0)
            x_Status_Return(Post_Process_Magic_Armor(root, info));
        }

   x_Status_Return_Success();
}