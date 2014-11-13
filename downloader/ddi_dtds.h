/*  FILE:   DDIDTDS.H

    Copyright (c) 2001-2009 by Lone Wolf Development.  All rights reserved.

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

    This file includes:

    Private XML DTD definitions for D&DI XML files.
*/


static  T_XML_Element   l_id = { "ID", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_name = { "Name", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_level = { "Level", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_prerequisite = { "Prerequisite", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_action_type = { "ActionType", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_power_source = { "PowerSourceText", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_role = { "RoleName", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_key_abilities = { "KeyAbilities", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_cost = { "Cost", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_category = { "Category", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_enhancement = { "EnhancementValue", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_group_role = { "GroupRole", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_combat_role = { "CombatRole", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_size = { "Size", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_description = { "DescriptionAttribute", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_component_cost = { "ComponentCost", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_price = { "Price", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_key_skill = { "KeySkillDescription", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_is_new = { "IsNew", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_is_changed = { "IsChanged", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_is_mundane = { "IsMundane", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_sourcebook = { "SourceBook", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_teaser = { "Teaser", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_tier_name = { "TierName", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_tier_sort = { "TierSort", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_final_cost = { "FinalCost", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_class_name = { "ClassName", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_alignment = { "Alignment", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_type = { "Type", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_campaign = { "Campaign", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_skills = { "Skills", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_rarity = { "Rarity", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_levelsort = { "LevelSort", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_costsort = { "CostSort", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Child     l_deity_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_alignment, e_xml_single },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_deity = { "Deity", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_deity_children), l_deity_children,
                                0, NULL
                                };

static  T_XML_Child     l_glossary_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_category, e_xml_single },
                                { &l_type, e_xml_single },
                                { &l_is_new, e_xml_optional },
                                { &l_is_changed, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_glossary = { "Glossary", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_glossary_children), l_glossary_children,
                                0, NULL
                                };

static  T_XML_Child     l_power_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_level, e_xml_single },
                                { &l_action_type, e_xml_single },
                                { &l_is_new, e_xml_optional },
                                { &l_is_changed, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_class_name, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_power = { "Power", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_power_children), l_power_children,
                                0, NULL
                                };

static  T_XML_Child     l_class_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_power_source, e_xml_single },
                                { &l_role, e_xml_single },
                                { &l_key_abilities, e_xml_single },
                                { &l_is_new, e_xml_optional },
                                { &l_is_changed, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_class = { "Class", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_class_children), l_class_children,
                                0, NULL
                                };

static  T_XML_Child     l_epic_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_prerequisite, e_xml_single },
                                { &l_is_new, e_xml_optional },
                                { &l_is_changed, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_epic = { "EpicDestiny", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_epic_children), l_epic_children,
                                0, NULL
                                };

static  T_XML_Child     l_feat_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_is_new, e_xml_optional },
                                { &l_is_changed, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_tier_name, e_xml_single },
                                { &l_tier_sort, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_feat = { "Feat", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_feat_children), l_feat_children,
                                0, NULL
                                };

static  T_XML_Child     l_item_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_cost, e_xml_single },
                                { &l_level, e_xml_single },
                                { &l_rarity, e_xml_single },
                                { &l_category, e_xml_single },
                                { &l_enhancement, e_xml_optional },
                                { &l_is_mundane, e_xml_optional },
                                { &l_final_cost, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                { &l_levelsort, e_xml_single },
                                { &l_costsort, e_xml_single },
                                };
static  T_XML_Element   l_item = { "Item", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_item_children), l_item_children,
                                0, NULL
                                };

static  T_XML_Child     l_monster_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_level, e_xml_single },
                                { &l_group_role, e_xml_single },
                                { &l_combat_role, e_xml_single },
                                { &l_is_new, e_xml_optional },
                                { &l_is_changed, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_monster = { "Monster", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_monster_children), l_monster_children,
                                0, NULL
                                };

static  T_XML_Child     l_paragon_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_prerequisite, e_xml_single },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_paragon = { "ParagonPath", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_paragon_children), l_paragon_children,
                                0, NULL
                                };

static  T_XML_Child     l_race_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_size, e_xml_single },
                                { &l_description, e_xml_single },
                                { &l_is_new, e_xml_optional },
                                { &l_is_changed, e_xml_optional },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_race = { "Race", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_race_children), l_race_children,
                                0, NULL
                                };

static  T_XML_Child     l_ritual_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_level, e_xml_single },
                                { &l_component_cost, e_xml_single },
                                { &l_price, e_xml_single, },
                                { &l_key_skill, e_xml_single },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_ritual = { "Ritual", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_ritual_children), l_ritual_children,
                                0, NULL
                                };

static  T_XML_Child     l_background_children[] = {
                                { &l_id, e_xml_single },
                                { &l_name, e_xml_single },
                                { &l_type, e_xml_single },
                                { &l_campaign, e_xml_single },
                                { &l_skills, e_xml_single, },
                                { &l_sourcebook, e_xml_single },
                                { &l_teaser, e_xml_optional },
                                };
static  T_XML_Element   l_background = { "Background", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_background_children), l_background_children,
                                0, NULL
                                };

static  T_XML_Child     l_results_children[] = {
                                { &l_deity, e_xml_zero_plus },
                                { &l_glossary, e_xml_zero_plus },
                                { &l_power, e_xml_zero_plus },
                                { &l_class, e_xml_zero_plus },
                                { &l_epic, e_xml_zero_plus },
                                { &l_feat, e_xml_zero_plus },
                                { &l_item, e_xml_zero_plus },
                                { &l_monster, e_xml_zero_plus },
                                { &l_paragon, e_xml_zero_plus },
                                { &l_race, e_xml_zero_plus },
                                { &l_ritual, e_xml_zero_plus },
                                { &l_background, e_xml_zero_plus },
                                };
static  T_XML_Element   l_results = { "Results", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_results_children), l_results_children,
                                0, NULL
                                };

static  T_XML_Element   l_table = { "Table", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Element   l_total = { "Total", false, e_xml_preserve, NULL,
                                0, NULL,
                                0, NULL
                                };

static  T_XML_Child     l_tab_children[] = {
                                { &l_table, e_xml_single },
                                { &l_total, e_xml_single },
                                };
static  T_XML_Element   l_tab = { "Tab", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_tab_children), l_tab_children,
                                0, NULL
                                };

static  T_XML_Child     l_totals_children[] = {
                                { &l_tab, e_xml_single },
                                };
static  T_XML_Element   l_totals = { "Totals", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_totals_children), l_totals_children,
                                0, NULL
                                };

static  T_XML_Child     l_ddiindex_children[] = {
                                { &l_results, e_xml_single },
                                { &l_totals, e_xml_single },
                                };
static  T_XML_Element   l_ddiindex = { "Data", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_ddiindex_children), l_ddiindex_children,
                                0, NULL
                                };
