/*  FILE:   PARSE_ITEMS.CPP

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


/*  Include our template class definition - see the comment at the top of the
    crawler.h file for details. Summary: C++ templates are terrible.
*/
#include "crawler.h"


template<class T> C_DDI_Crawler<T> *    C_DDI_Crawler<T>::s_crawler = NULL;


C_DDI_Items::C_DDI_Items(C_Pool * pool) : C_DDI_Crawler("items", "item", "Item",
                                                        5, 4, false, pool)
{
}


C_DDI_Items::~C_DDI_Items()
{
}


static bool Is_Name_Armor(T_Glyph_Ptr name)
{
    return (strstr(name, "Armor") != NULL);
}


/*  For shields, make sure the name has "Shield", but not "Shielding" (as in
    brooch of shielding) or "of shield" (as inwand of shield) in it.
*/
static bool Is_Name_Shield(T_Glyph_Ptr name)
{
    if (strstr(name, "Shield") == NULL)
        return(false);
    return ((strstr(name, "Shielding") == NULL) && (strstr(name, "of Shield") == NULL));
}


static void     Set_Item_File(T_Item_Info * info)
{
    T_Int32U        length, temp_length;
    T_Glyph_Ptr     temp1;

    /* Weapons are either normal or magic
    */
    if (strcmp(info->itemcategory, "Weapon") == 0) {
        info->filename = (T_Glyph_Ptr) ((info->level[0] == '\0') ? FILE_WEAPONS : FILE_WEAPONS_MAGIC);
        return;
        }

    /* Armor is either normal or magic
    */
    if (strcmp(info->itemcategory, "Armor") == 0) {
        info->filename = (T_Glyph_Ptr) ((info->level[0] == '\0') ? FILE_ARMOR : FILE_ARMOR_MAGIC);
        return;
        }

    /* For equipment, if the name of this thing ends with "(empty)", trim
        that off
    */
    if ((strcmp(info->itemcategory, "Gear") == 0) || (strcmp(info->itemcategory, "Equipment") == 0)) {
        info->filename = FILE_EQUIPMENT;

        length = strlen(info->name);
        temp1 = "(empty)";
        temp_length = strlen(temp1);
        if ((temp_length < length) && (strcmp(info->name + length - temp_length, temp1) == 0)) {
            temp1 = info->name + length - temp_length;
            *temp1 = '\0';
            while (temp1[-1] == ' ')
                *--temp1 = '\0';
            }
        return;
        }

    /* Otherwise, we're a magic item and our category is "Neck" or "Arms" or
        something, so we're good
    */
}


T_Status        C_DDI_Items::Parse_Index_Cell(T_XML_Node node, T_Item_Info * info,
                                                T_Int32U subtype)
{
    T_Status        status;
    T_Glyph_Ptr     ptr;
    T_Glyph         temp[1000];

    status = Get_Required_Child_PCDATA(&info->itemcategory, node, "Category");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->level, node, "Level");
    if (!x_Is_Success(status))
        x_Status_Return(status);
    status = Get_Required_Child_PCDATA(&info->rarity, node, "Rarity");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    /* If our level is '0', that's obviously nonsense so get
        rid of it
    */
    if ((info->level[0] == '0') && (info->level[1] == '\0'))
        info->level = "";

    /* We only care about the number in the level - get rid of everything else
        so that we don't have to worry about parsing "4+" or whatever
    */
    ptr = info->level;
    while (*ptr != '\0') {
        if (!x_Is_Digit(*ptr))
            *ptr = '\0';
        *ptr++;
        }

    status = Get_Required_Child_PCDATA(&info->cost, node, "Cost");
    if (!x_Is_Success(status))
        x_Status_Return(status);

    /* Strip "gp" off the end of the cost
    */
    if (stricmpright(info->cost, "gp") == 0) {
        ptr = info->cost + strlen(info->cost);
        ptr -= 2;
        while (x_Is_Space(ptr[-1]))
            ptr--;
        *ptr = '\0';
        }

    /* Zap any commas in the cost - how many ways can this poor cost field be
        molested?
    */
    Text_Find_Replace(temp, info->cost, ",", "");
    strcpy(info->cost, temp);

    /* Strip off any trailing 0s and the decimal point from the cost
    */
    if ((info->cost[0] != '\0') && (strchr(info->cost, '.') != NULL)) {
        ptr = info->cost + strlen(info->cost) - 1;
        while (ptr[-1] == '0')
            ptr--;
        if (ptr[-1] == '.')
            ptr--;
        *ptr = '\0';
        }

    /* Now that we have all the data we need, work out what kind of item this is
    */
    Set_Item_File(info);

    x_Status_Return_Success();
}


static T_Status     Parse_Equipment(T_Glyph_Ptr contents, T_Item_Info * info, T_Glyph_Ptr ptr,
                                    T_Glyph_Ptr checkpoint, C_Pool * pool)
{
    T_Glyph_Ptr     temp, end, monster, published;

    /* If we haven't had our subcategory set already, grab it
    */
    if (info->subcategory == NULL)
        ptr = Extract_Text(&info->subcategory, pool, ptr, checkpoint, "<b>Category</b>:", "<br");

    T_Glyph_Ptr     find_options[] = { "<b>Weight</b>:" };
    T_Glyph_Ptr     end_options[] = { "<br", "\n" };
    ptr = Extract_Text(&info->weight, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                        end_options, x_Array_Size(end_options));

    monster = strstr(ptr, "<h1 class=\"monster\">");

    /* If appropriate, classify this item as a mount instead of equipment
    */
    if ((info->subcategory != NULL) && (stricmp(info->subcategory, "Mount") == 0)) {
        info->filename = FILE_MOUNTS;
        info->subcategory = "Mount";
        }

    /* Trim us at the "Published in" section, since we should already have that
        information
    */
    published = strstr(ptr, "<p>Published in");
    temp = strstr(ptr, "<p class=\"publishedIn\">Published in");
    if ((published == NULL) || ((temp != NULL) && (temp < published)))
        published = temp;
    if (published != NULL)
        *published = '\0';

    ptr = strstr(ptr, "<b>Description</b>:");
    if ((ptr != NULL) && ((monster == NULL) || (ptr < monster))) {
        ptr += 19;
        end = monster;
        if (published != NULL)
            end = published;
        if (end == NULL)
            end = strstr(ptr, "</div");
        if (end != NULL) {
            *end = '\0';
            Parse_Description_Text(&info->description, ptr, pool);
            *end = '<';
            }
        }

    x_Status_Return_Success();
}


static T_Status     Parse_Weapon(T_Glyph_Ptr contents, T_Item_Info * info, T_Glyph_Ptr ptr,
                                    T_Glyph_Ptr checkpoint, C_Pool * pool)
{
    T_Glyph_Ptr     temp, end;
    T_Glyph         backup;

    /* Weapon info comes after the <h1> tag
    */
    ptr = Extract_Text(&info->subcategory, pool, ptr, checkpoint, "</h1><br/>", "<br");

    ptr = Extract_Text(&info->damage, pool, ptr, checkpoint, "<b>Damage</b>:", "<br");

    ptr = Extract_Text(&info->proficient, pool, ptr, checkpoint, "Proficient:", "<br");

    /* Range is done terribly for some reason
    */
    ptr = Extract_Text(&info->range, pool, ptr, checkpoint, "<br/>Range:", "<br");

    ptr = Extract_Text(&info->weight, pool, ptr, checkpoint, "Weight:", "<br");

    T_Glyph_Ptr     find_options[] = { "<b>Properties</b>:" };
    T_Glyph_Ptr     end_options[] = { "<b>", "</div" };
    ptr = Extract_Text(&info->properties, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                        end_options, x_Array_Size(end_options));

    ptr = Extract_Text(&info->groups, pool, ptr, checkpoint, "<b>Group</b>:", "</div");

    /* Some stuff, like "accurate orbs", are incorrectly categorized as
        weapons, when they should really be equipment - basically anything
        whose group is an implement type.
        NOTE: Quarterstaffs are an exception, since they are an actual weapon.
            If we don't skip them, they get shoved incorrectly into the "Staff"
            group.
    */
    if ((info->groups != NULL) && (info->groups[0] != '\0') &&
        (strcmp(info->name, "Quarterstaff") != 0)) {
        temp = info->groups;
        while (strncmp(temp, "<br/>", 5) == 0)
            temp += 5;
        end = strchr(temp, '(');
        if (end != NULL) {
            while (end[-1] == ' ')
                end--;
            backup = *end;
            *end = '\0';
            }
        temp = Mapping_Find("impltype", temp);
        if (temp != NULL) {
            info->filename = FILE_EQUIPMENT;
            info->super_impl_type = temp;
            }
        if (end != NULL)
            *end = backup;
        }

    x_Status_Return_Success();
}


static T_Status     Parse_Armor(T_Glyph_Ptr contents, T_Item_Info * info, T_Glyph_Ptr ptr,
                                    T_Glyph_Ptr checkpoint, C_Pool * pool)
{
    T_Glyph_Ptr         starts[] = { "???", "???", "???", };
    T_Glyph_Ptr         ends[] = { "<br", "<BR" };

    /* Weapon info comes after the <h1> tag
    */
    ptr = Extract_Text(&info->description, pool, ptr, checkpoint, "<b>Description</b>:", "<br");
    Strip_Bad_Characters(info->description);

    ptr = Extract_Text(&info->ac, pool, ptr, checkpoint, "<b>AC Bonus</b>:", "<br");

    ptr = Extract_Text(&info->minenhance, pool, ptr, checkpoint, "<b>Minimum Enhancement Value</b>:", "<br");

    ptr = Extract_Text(&info->armorcheck, pool, ptr, checkpoint, "<b>Check</b>:", "<br");

    ptr = Extract_Text(&info->speed, pool, ptr, checkpoint, "<b>Speed</b>:", "<br");

    starts[0] = "<b>Weight</b>:";
    ptr = Extract_Text(&info->weight, pool, ptr, checkpoint, starts, 1,
                        ends, x_Array_Size(ends));

    starts[0] = "<b>Special</b>:";
    starts[1] = "<B>Special</B>:";
    ptr = Extract_Text(&info->special, pool, ptr, checkpoint, starts, 2,
                        ends, x_Array_Size(ends));
    Strip_Bad_Characters(info->special);

    ptr = Extract_Text(&info->armortype, pool, ptr, checkpoint, "<b>Type</b>:", "<br");

    /* If we didn't find any description earlier, get the italicized
        description at the end
    */
    if ((info->description == NULL) || (info->description[0] == '\0')) {
        ptr = Extract_Text(&info->description, pool, ptr, checkpoint, "<i>", "</i>");
        Strip_Bad_Characters(info->description);
        }

    x_Status_Return_Success();
}


static void         Parse_Power_Keywords(T_Glyph_Ptr ptr, T_Power_Info * info, C_Pool * pool)
{
    T_Glyph_Ptr         end;

    while (*ptr != '\0') {
        while (*ptr == ' ')
            ptr++;
        end = strchr(ptr, ',');
        if (end != NULL)
            *end = '\0';
        info->keywords[info->keyword_count++] = pool->Acquire(ptr);
        if (end == NULL)
            break;
        *end = ',';
        ptr = end + 1;
        }
}


static void         Parse_Power_Action(T_Power_Info * info)
{
    T_Glyph_Ptr         end;

    /* If the first sentence of this power description is "<something> Action",
        parse it out into the ActionType field.
    */
    end = strchr(info->description, '.');
    if (end == NULL)
        return;
    *end = '\0';
    if (stricmpright(info->description, "Action") != 0) {
        *end = '.';
        return;
        }
    info->action = info->description;
    info->description = end + 1;
    while (*info->description == ' ')
        info->description++;
}


static T_Int32U     Extract_Bonuses(T_Glyph_Ptr bonustable, T_Int32U levels[], T_Int32U bonuses[])
{
    T_Int32U            i;
    T_Glyph_Ptr         ptr;

    if ((bonustable == NULL) || (bonustable[0] == '\0'))
        return(0);

    /* Extract the level and bonus from text like this:
        <tr><td class="mic1">Lvl 1</td><td class="mic2">+1</td>
    */
    i = 0;
    ptr = bonustable;
    while (true) {

        /* Get the level
        */
        ptr = strstr(ptr, "Lvl ");
        if (ptr == NULL)
            goto done;
        ptr += 4;
        levels[i] = atoi(ptr);

        /* Get the bonus
        */
        ptr = strstr(ptr, "mic2");
        if (ptr == NULL)
            goto done;
        ptr = strchr(ptr, '>');
        if (ptr == NULL)
            goto done;
        ptr++;
        if (*ptr == '+')
            ptr++;
        bonuses[i] = atoi(ptr);

        /* Go on to the next level / bonus
        */
        i++;
        }

done:
    return(i);
}


static bool Sort_Items(const T_Item_Info & item1, const T_Item_Info & item2)
{
    if (atoi(item1.level) < atoi(item2.level))
        return(true);
    if ((item1.enhbonus != NULL) && (item2.enhbonus != NULL) &&
        (atoi(item1.enhbonus) < atoi(item2.enhbonus)))
        return(true);
    return(false);
}


static T_Status     Parse_Magic_Item(T_Glyph_Ptr contents, T_Item_Info * info, T_Glyph_Ptr ptr,
                                        T_Glyph_Ptr checkpoint, C_Pool * pool,
                                        C_DDI_Items * crawler, vector<T_Item_Info> * extras)
{
    T_Int32U        i, bonus, bonus_count;
    T_Glyph_Ptr     end, temp1, temp2, temp3, powerstart;
    T_Power_Info    power_info;
    T_Item_Info     dupe;
    T_Power_Info *  power_ptr;
    T_Int32U        levels[10];
    T_Int32U        bonuses[10];
    T_Glyph         buffer[1000];

    /* Search for a certain string to see if this is an artifact
    */
    if (strstr(ptr, "Epic Level</h1>") != NULL)
        info->is_artifact = true;
    else if (strstr(ptr, "Paragon Level</h1>") != NULL)
        info->is_artifact = true;

    /* Extract any flavor text and skip past the </i> at the end
    */
    {
        T_Glyph_Ptr     find_options[] = { "<p class=\"miflavor\">", "<p class=\"flavor\"><i>" };
        T_Glyph_Ptr     end_options[] = { "</p>", "</i>" };
        ptr = Extract_Text(&info->flavor, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                            end_options, x_Array_Size(end_options));
        }
    Strip_Bad_Characters(info->flavor);
    if (strncmp(ptr, "</p>", 4) == 0)
        ptr += 4;

    /* Extract the table of levels for us to use later
    */
    temp1 = NULL;
    ptr = Extract_Text(&temp1, pool, ptr, checkpoint, "<table class=\"magicitem\">", "</table>");
    bonus_count = Extract_Bonuses(temp1, levels, bonuses);

    /* Skip over the level - we already know it, but we need to parse it out so
        it doesn't interfere with stuff
    */
    ptr = Extract_Text(&temp1, pool, ptr, checkpoint, "<b>Level</b>:", "<br");

    {
        T_Glyph_Ptr     find_options[] = { "<b>Weapon: </b>", "<b>Weapon</b>:" };
        T_Glyph_Ptr     end_options[] = { "</p>", "<br", "&nbsp;" };
        ptr = Extract_Text(&info->weaponreq, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                            end_options, x_Array_Size(end_options));
        }

    {
        T_Glyph_Ptr     find_options[] = { "<b>Armor: </b>", "<b>Armor</b>:" };
        T_Glyph_Ptr     end_options[] = { "</p>", "<br", "&nbsp;" };
        ptr = Extract_Text(&info->armorreq, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                            end_options, x_Array_Size(end_options));
        }

    {
        T_Glyph_Ptr     find_options[] = { "<b>Implement (", "<b>Implement: </b>" };
        T_Glyph_Ptr     end_options[] = { ")", "</p>", "<br", "&nbsp;" };
        ptr = Extract_Text(&info->implement, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                            end_options, x_Array_Size(end_options));
        if (strncmp(ptr, ")</b>", 5) == 0)
            ptr += 5;
        }

	/* Parse out our item slot, but don't actually do anything with it - we get
		better data from the index file
	*/
    ptr = Extract_Text(&temp1, pool, ptr, checkpoint, "<b>Item Slot: </b>", "</p>");

    {
        T_Glyph_Ptr     find_options[] = { "<b>Enhancement Bonus:</b>", "<b>Enhancement</b>:" };
        T_Glyph_Ptr     end_options[] = { "<br", "</p" };
        ptr = Extract_Text(&info->enhancement, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                            end_options, x_Array_Size(end_options));
        }


    /* Set this item up with the default level and bonus if we have them
    */
    if (bonus_count > 0) {
        info->level = pool->Acquire(As_Int(levels[0]));
        if (bonuses[0] != 0)
            info->enhbonus = pool->Acquire(As_Int(bonuses[0]));
        }

    /* If not, and it's a weapon, armor, or neckpiece, we don't know what the
        enhancement bonus is because it's not listed anywhere! Have a guess.
    */
    else if (((info->weaponreq != NULL) && (info->weaponreq[0] != '\0')) ||
        ((info->armorreq != NULL) && (info->armorreq[0] != '\0')) ||
        (strcmp(info->itemcategory, "Neck") == 0)) {
        bonus = atoi(info->level);
        if (bonus != 0)
            bonus = (bonus - 1) / 5 + 1;

        /* If we don't have a bonus, can we infer it from the enhancement bonus
            text?
        */
        else if (info->enhancement != NULL) {
            temp1 = info->enhancement;
            if (*temp1 == '+')
                temp1++;
            bonus = atoi(temp1);
            }

        if (bonus != 0)
            info->enhbonus = pool->Acquire(As_Int(bonus));
        }
    {
        T_Glyph_Ptr     find_options[] = { "<b>Critical:</b>", "<b>Critical</b>:" };
        T_Glyph_Ptr     end_options[] = { "<br", "</p" };
        ptr = Extract_Text(&info->critical, pool, ptr, checkpoint, find_options, x_Array_Size(find_options),
                            end_options, x_Array_Size(end_options));
        Strip_Bad_Characters(info->critical);
        }

    /* Parse anything else as description text
    */
    end = strstr(ptr, "<i>First");
    temp1 = strstr(ptr, "</div>");
    if ((end == NULL) || ((temp1 != NULL) && (temp1 < end)))
        end = temp1;
    temp1 = strstr(ptr, "<p>Published in");
    if ((end == NULL) || ((temp1 != NULL) && (temp1 < end)))
        end = temp1;
    temp1 = strstr(ptr, "<p class=\"publishedIn\">Published in");
    if ((end == NULL) || ((temp1 != NULL) && (temp1 < end)))
        end = temp1;
    if (end != NULL)
        *end = '\0';
    Parse_Description_Text(&info->description, ptr, pool, true);
    if (end != NULL)
        *end = '<';

    /* Description text longer than ~10k characters causes Hero Lab to crash,
        so fix that - we won't actually display more than 5000 characters
        anyway
    */
    if ((info->description != NULL) && (strlen(info->description) > 5000))
        info->description[5000] = '\0';

    /* If we have "Shield" in our name, set our armor requirement appropriately
    */
    if (Is_Name_Shield(info->name))
        info->armorreq = pool->Acquire("Light Shield, Heavy Shield");

    /* Otherwise, this is the first instance of this item - extract any
        powers that happen to be here
    */
    powerstart = "<h2 class=\"mihead\">Power ";
    temp2 = ptr;
    temp1 = powerstart;
    ptr = strstr(temp2, temp1);
    if (ptr == NULL) {
        temp1 = "</p>Power (";
        ptr = strstr(temp2, temp1);
        }
    while (ptr != NULL) {
        ptr += strlen(temp1);
        memset(&power_info, 0, sizeof(power_info));

        /* Get the new checkpoint, which is the first <br> or </p> we find
        */
        end = strstr(ptr, "<br");
        temp2 = strstr(ptr, "</p");
        if ((end == NULL) || ((temp2 != NULL) && (temp2 < end)))
            end = temp2;
        if (end == NULL) {
            sprintf(buffer, "No power checkpoint found for magic item %s\n", info->name);
            Log_Message(buffer);
            break;
            }
        *end = '\0';

        /* Copy the name from the name of the item, and set an appropriate
            class
        */
        power_info.name = info->name;
        power_info.forclass = "Magic Item";
        power_info.type = "Magic Item";

        /* Keywords come next if we have parentheses
        */
        if (*ptr == '(') {
            ptr++;
            temp2 = strchr(ptr, ')');
            if (temp2 != NULL) {
                *temp2 = '\0';
                Parse_Power_Keywords(temp2, &power_info, pool);
                *temp2 = ')';
                ptr = temp2 + 1;
                }
            }

        /* Skip past any image separator tag
        */
        while (x_Is_Space(*ptr))
            ptr++;
        if (strncmp(ptr, "<img", 4) == 0) {
            temp2 = strchr(ptr, '>');
            if (temp2 != NULL)
                ptr = temp2 + 1;
            }
        while (x_Is_Space(*ptr))
            ptr++;

        /* Copy in the power use
        */
        if (strnicmp(ptr, "At-Will", 7) == 0) {
            power_info.use = POWER_ATWILL;
            ptr += 7;
            }
        else if (strnicmp(ptr, "Encounter", 9) == 0) {
            power_info.use = POWER_ENCOUNTER;
            ptr += 9;
            }
        else if (strnicmp(ptr, "Daily", 5) == 0) {
            power_info.use = POWER_DAILY;
            power_info.limit = "Magic Item Daily";
            ptr += 5;
            }
        else if (strnicmp(ptr, "Healing Surge", 13) == 0) {
            power_info.use = POWER_DAILY_SURGE;
            ptr += 13;
            }
        else if (strnicmp(ptr, "Consumable", 10) == 0) {
            power_info.use = POWER_CONSUMABLE;
            ptr += 10;
            }

        /* These days, no power use seems to indicate "at-will power"
        */
        else
            power_info.use = POWER_ATWILL;

        /* Action is sometimes in parentheses after the power use
        */
        while (x_Is_Space(*ptr))
            ptr++;
        if (*ptr == '(') {
            ptr++;
            temp2 = strchr(ptr, ')');
            if (temp2 != NULL) {
                *temp2 = '\0';
                power_info.action = pool->Acquire(ptr);
                *temp2 = ')';
                }
            }
        while (x_Is_Space(*ptr))
            ptr++;


        /* Finally, copy the description
        */
        temp3 = ptr;
        ptr = strstr(temp3, "<p class=\"mistat");
        if (ptr != NULL) {
            ptr = strchr(ptr, '>');
            if (ptr == NULL) {
                sprintf(buffer, "No power description found for magic item %s\n", info->name);
                Log_Message(buffer);
                break;
                }
            ptr++;
            }

        /* If we don't have a standard description paragraph, look for the next
            sentence to start the power
        */
        else {
            ptr = strstr(temp3, ". ");
            if (ptr == NULL) {
                sprintf(buffer, "No power description found for magic item %s\n", info->name);
                Log_Message(buffer);
                break;
                }
            ptr++;
            while (x_Is_Space(*ptr))
                ptr++;
            }

        Parse_Description_Text(&power_info.description, ptr, pool, true);
        Parse_Power_Action(&power_info);
        *end = '<';

        /* Add a record to the race, so we can bootstrap the power once it gets
            processed
        */
        info->powers[info->power_count++] = C_DDI_Powers::Get_Crawler()->Append_Item(&power_info);;

        /* Try to find following powers!
        */
        temp1 = powerstart;
        temp2 = strstr(ptr, temp1);
        if (temp2 == NULL) {
            temp1 = "<br/>Power (";
            temp2 = strstr(ptr, temp1);
            }
        ptr = temp2;
        }

    /* If we have more than one power to attach, give them an id number
    */
    if (info->power_count > 1) {
        for (i = 0; i < info->power_count; i++) {
            power_ptr = C_DDI_Powers::Get_Crawler()->Get_Item(info->powers[i]);
            if (x_Trap_Opt(power_ptr == NULL))
                continue;
            power_ptr->idnumber = pool->Acquire(As_Int(i + 1));
            }
        }

    /* Based on our improved data, work out which file the item should be in
    */
    if ((info->weaponreq != NULL) && (info->weaponreq[0] != '\0'))
        info->filename = FILE_WEAPONS_MAGIC;
    else if ((info->armorreq != NULL) && (info->armorreq[0] != '\0'))
        info->filename = FILE_ARMOR_MAGIC;
    else
        info->filename = NULL;

    /* If we have more than one level of this item, duplicate it and add it to
        our extras list
    */
    for (i = 1; i < bonus_count; i++) {
        memcpy(&dupe, info, sizeof(T_Item_Info));
        dupe.level = pool->Acquire(As_Int(levels[i]));
        if (bonuses[i] != 0)
            dupe.enhbonus = pool->Acquire(As_Int(bonuses[i]));
        extras->push_back(dupe);
        }
    sort(extras->begin(), extras->end(), Sort_Items);

    x_Status_Return_Success();
}


/* Note - this function should be resilient to stuff in the HTML changing, so
    if we don't find what we expect, we should just return successfully and
    not complain about it.
*/
T_Status        C_DDI_Items::Parse_Entry_Details(T_Glyph_Ptr contents, T_Item_Info * info,
                                                    T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                                    vector<T_Item_Info> * extras)
{
    if (info->filename != NULL) {
        if ((strcmp(info->filename, FILE_EQUIPMENT) == 0) || (strcmp(info->filename, FILE_MOUNTS) == 0))
            x_Status_Return(Parse_Equipment(contents, info, ptr, checkpoint, m_pool));
        if (strcmp(info->filename, FILE_WEAPONS) == 0)
            x_Status_Return(Parse_Weapon(contents, info, ptr, checkpoint, m_pool));
        if (strcmp(info->filename, FILE_ARMOR) == 0)
            x_Status_Return(Parse_Armor(contents, info, ptr, checkpoint, m_pool));
        }
    x_Status_Return(Parse_Magic_Item(contents, info, ptr, checkpoint, m_pool, this, extras));
}
