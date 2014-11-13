/*  FILE:   DTDS.H

    Copyright (c) 2001-2012 by Lone Wolf Development.  All rights reserved.

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

    Private XML DTD definitions for the DDI Crawler.
*/


/* On the mac, XCode raises a lot more errors than the windows compilers do.
    Define this to make xcode ignore unused variables in this file, since most of
    it is just copied & pasted from the standard Hero Lab codebase and we want
    it to remain the same for easy comparison / reuse.
*/
#ifdef _OSX
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif


/*  define the various signatures used in the XML files
*/
#define DATA_SIGNATURE          "Hero Lab Data"
#define AUGMENTATION_SIGNATURE  "Hero Lab Structure"
#define MAPPING_SIGNATURE       "DDI Downloader Mapping"


/*  define a structure to bind together a list of strings, integer mappings
    into that list, and the array size - use templates so it's fully general
    and can be used for other things later, then define a typedef for what
    we're actually using here.
    NOTE! Be careful how you do comparisons here - if either of the types
        cannot be compared using the == operator, it's your responsibility to
        handle the comparison properly. Perhaps a better way of doing this
        would be to use a class with a virtual Compare function.
*/
template<typename Type1, typename Type2>
struct  T_Table_Map {
    Type1 *         key1;
    Type2 *         key2;
    T_Int32U        count;
    };

typedef T_Table_Map<T_Glyph_Ptr, T_Int32U> T_Basis_Lookup;


static  T_Glyph_Ptr     l_yes_no[] = { "yes", "no" };
static  T_Int32U        l_map_yes_no[] = { 1, 0 };
#define SIZE_YES_NO     x_Array_Size(l_yes_no)
static  T_Basis_Lookup  l_lookup_yes_no = { l_yes_no, l_map_yes_no, SIZE_YES_NO };


/*  define the valid set of default stacking behaviors that are supported
*/
#define STACKING_INDIVIDUAL     0   //add each new item individually
#define STACKING_NEW_GROUP      1   //add all new items as a single new group
#define STACKING_COMBINE        2   //add all new items to an existing item/group
#define STACKING_NEVER          3   //never allow stacking for this thing
static  T_Glyph_Ptr     l_stacking[] = { "solo", "new", "merge", "never" };
static  T_Int32U        l_map_stacking[] = { STACKING_INDIVIDUAL, STACKING_NEW_GROUP, STACKING_COMBINE, STACKING_NEVER };
#define SIZE_STACKING   x_Array_Size(l_stacking)
static  T_Basis_Lookup  l_lookup_stacking = { l_stacking, l_map_stacking, SIZE_STACKING };


/*  define the valid set of prereq contexts that are supported
*/
#define ALT_TARGET_NONE         0
#define ALT_TARGET_HERO         1
#define ALT_TARGET_PARENT       2
static  T_Glyph_Ptr     l_prereq_target[] = { "container", "hero", "parent" };
static  T_Int32U        l_map_prereq_target[] = { ALT_TARGET_NONE, ALT_TARGET_HERO, ALT_TARGET_PARENT };
#define SIZE_PREREQ_TARGET  x_Array_Size(l_prereq_target)
static  T_Basis_Lookup  l_lookup_prereq_target = { l_prereq_target, l_map_prereq_target, SIZE_PREREQ_TARGET };


/*  define error severities
*/
static  T_Glyph_Ptr     l_severity[] = { "error", "warning" };
static  T_Int32U        l_map_severity[] = { 1, 0 };
#define SIZE_SEVERITY   x_Array_Size(l_severity)
static  T_Basis_Lookup  l_lookup_severity = { l_severity, l_map_severity, SIZE_SEVERITY };


/*  define the valid set of value assignment behaviors that are supported
*/
#define BOOTASSIGN_ASSIGN       0   //new value is assigned
#define BOOTASSIGN_MINIMUM      1   //minimum of existing and new value is assigned
#define BOOTASSIGN_MAXIMUM      2   //maximum of existing and new value is assigned
static  T_Glyph_Ptr     l_boot_assign[] = { "assign", "minimum", "maximum" };
static  T_Int32U        l_map_boot_assign[] = { BOOTASSIGN_ASSIGN, BOOTASSIGN_MINIMUM, BOOTASSIGN_MAXIMUM };
#define SIZE_BOOT_ASSIGN    x_Array_Size(l_boot_assign)
static  T_Basis_Lookup  l_lookup_boot_assign = { l_boot_assign, l_map_boot_assign, SIZE_BOOT_ASSIGN };


/*  define the valid set of uniqueness behaviors for things
*/
#define UNIQUENESS_NONE         0
#define UNIQUENESS_UNIQUE       1
#define UNIQUENESS_USER_ONCE    2
static  T_Glyph_Ptr     l_uniqueness[] = { "none", "unique", "useronce" };
static  T_Int32U        l_map_uniqueness[] = { UNIQUENESS_NONE, UNIQUENESS_UNIQUE, UNIQUENESS_USER_ONCE };
#define SIZE_UNIQUENESS     x_Array_Size(l_uniqueness)
static  T_Basis_Lookup  l_lookup_uniqueness = { l_uniqueness, l_map_uniqueness, SIZE_UNIQUENESS };


/*  define the XML structure for a "match" tagexpr
*/
static  T_XML_Element   l_match = { "match", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };


/*  define the comments element
*/
static  T_XML_Element   l_comments = { "comment", false, e_xml_preserve, NULL,
                                        0, NULL, 0, NULL
                                        };


/*  define the XML structure for before and after references
*/
static  T_XML_Attribute l_before_attributes[] = {
                                { "name", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Element   l_before = { "before", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_before_attributes), l_before_attributes
                                };

static  T_XML_Attribute l_after_attributes[] = {
                                { "name", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Element   l_after = { "after", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_after_attributes), l_after_attributes
                                };


/*  define the XML structure for a condition tag expression
*/
static  T_XML_Child     l_condtest_children[] = {
                                { &l_match, e_xml_optional },
                                { &l_before, e_xml_zero_plus },
                                { &l_after, e_xml_zero_plus }
                                };
static  T_XML_Attribute l_condtest_attributes[] = {
                                { "phase", e_xml_required, NULL, NULL, 0, NULL },
                                { "priority", e_xml_required, NULL, NULL, 0, NULL },
                                { "name", e_xml_implied, "", NULL, 0, NULL },
                                { "isprimary", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Element   l_condtest = { "condtest", false, e_xml_preserve, NULL,
                                x_Array_Size(l_condtest_children), l_condtest_children,
                                x_Array_Size(l_condtest_attributes), l_condtest_attributes
                                };


/*  define the XML structure for a containerreq tag expression
*/
static  T_XML_Child     l_containerreq_children[] = {
                                { &l_match, e_xml_optional },
                                { &l_before, e_xml_zero_plus },
                                { &l_after, e_xml_zero_plus }
                                };
static  T_XML_Attribute l_containerreq_attributes[] = {
                                { "phase", e_xml_required, NULL, NULL, 0, NULL },
                                { "priority", e_xml_required, NULL, NULL, 0, NULL },
                                { "name", e_xml_implied, "", NULL, 0, NULL },
                                { "isprimary", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Element   l_containerreq = { "containerreq", false, e_xml_preserve, NULL,
                                x_Array_Size(l_containerreq_children), l_containerreq_children,
                                x_Array_Size(l_containerreq_attributes), l_containerreq_attributes
                                };


/*  define a generic autotag element for managing a list of auto-assigned tags
*/
static  T_XML_Attribute l_autotag_attributes[] = {
                                { "group", e_xml_required, NULL, NULL, 0, NULL },
                                { "tag", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Element   l_autotag = { "autotag", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_autotag_attributes), l_autotag_attributes
                                };


/*  define the XML structure for a field value override for bootstraps
*/
static  T_XML_Attribute l_assignval_attributes[] = {
                                { "field", e_xml_required, NULL, NULL, 0, NULL },
                                { "value", e_xml_required, NULL, NULL, 0, NULL },
                                { "behavior", e_xml_set, l_boot_assign[0], NULL, SIZE_BOOT_ASSIGN, l_boot_assign }
                                };
static  T_XML_Element   l_assignval = { "assignval", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_assignval_attributes), l_assignval_attributes
                                };


/*  define the XML structure for a thing reference (bootstrap)
*/
static  T_XML_Attribute l_bootstrap_attributes[] = {
                                { "thing", e_xml_required, NULL, NULL, 0, NULL },
                                { "index", e_xml_implied, "1", NULL, 0, NULL },
                                { "phase", e_xml_implied, "*", NULL, 0, NULL },
                                { "priority", e_xml_implied, "0", NULL, 0, NULL }
                                };
static  T_XML_Child     l_bootstrap_children[] = {
                                { &l_match, e_xml_optional },
                                { &l_containerreq, e_xml_optional },
                                { &l_autotag, e_xml_zero_plus },
                                { &l_assignval, e_xml_zero_plus }
                                };
static  T_XML_Element   l_bootstrap = { "bootstrap", false, e_xml_preserve, NULL,
                                x_Array_Size(l_bootstrap_children), l_bootstrap_children,
                                x_Array_Size(l_bootstrap_attributes), l_bootstrap_attributes
                                };


/*  define shared XML structure for eval scripts and rules
*/
#define DTD_EVAL \
            { "phase", e_xml_required, NULL, NULL, 0, NULL }, \
            { "priority", e_xml_implied, "100", NULL, 0, NULL }, \
            { "value", e_xml_implied, "1", NULL, 0, NULL }, \
            { "index", e_xml_implied, "1", NULL, 0, NULL }, \
            { "runlimit", e_xml_implied, "0", NULL, 0, NULL }, \
            { "sortas", e_xml_implied, "", NULL, 0, NULL }, \
            { "name", e_xml_implied, "", NULL, 0, NULL }, \
            { "isprimary", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no }, \
            { "iseach", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no }


/*  define a generic condition element for use with condition tag expressions
*/
static  T_XML_Element   l_condition = { "condition", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };


/*  define the XML structure for an eval script
*/
static  T_XML_Attribute l_eval_attributes[] = {
                                DTD_EVAL
                                };
static  T_XML_Child     l_eval_children[] = {
                                { &l_condition, e_xml_optional },
                                { &l_match, e_xml_optional },
                                { &l_before, e_xml_zero_plus },
                                { &l_after, e_xml_zero_plus }
                                };
static  T_XML_Element   l_eval = { "eval", false, e_xml_preserve, NULL,
                                x_Array_Size(l_eval_children), l_eval_children,
                                x_Array_Size(l_eval_attributes), l_eval_attributes
                                };


/*  define the XML structure for an eval rule
*/
static  T_XML_Attribute l_evalrule_attributes[] = {
                                DTD_EVAL,
                                { "message", e_xml_required, NULL, NULL, 0, NULL },
                                { "summary", e_xml_implied, "", NULL, 0, NULL },
                                { "severity", e_xml_set, l_severity[0], NULL, SIZE_SEVERITY, l_severity },
                                { "reportlimit", e_xml_implied, "0", NULL, 0, NULL },
                                { "issilent", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Child     l_evalrule_children[] = {
                                { &l_condition, e_xml_optional },
                                { &l_match, e_xml_optional },
                                { &l_before, e_xml_zero_plus },
                                { &l_after, e_xml_zero_plus }
                                };
static  T_XML_Element   l_evalrule = { "evalrule", false, e_xml_preserve, NULL,
                                x_Array_Size(l_evalrule_children), l_evalrule_children,
                                x_Array_Size(l_evalrule_attributes), l_evalrule_attributes
                                };


/*  define the XML structure for a "live" tagexpr
*/
static  T_XML_Element   l_live = { "live", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };


/*  define the XML structure for a "setup" script
*/
static  T_XML_Element   l_setup = { "setup", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };


/*  define the XML structure for a "position" and "header" script
*/
static  T_XML_Element   l_position = { "position", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };
static  T_XML_Element   l_header = { "header", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };


/*  define the XML structure for a "gear" script
*/
static  T_XML_Element   l_gear = { "gear", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };


/*  define the XML structure for a tag group reference (non-dynamic)
*/
static  T_XML_Attribute l_groupref_attributes[] = {
                                { "group", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Element   l_groupref = { "groupref", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_groupref_attributes), l_groupref_attributes
                                };


/*  define the XML structure for a channel broadcast entry
*/
static  T_XML_Attribute l_broadcast_attributes[] = {
                                { "channel", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Element   l_broadcast = { "broadcast", false, e_xml_preserve, NULL,
                                0, NULL,
                                x_Array_Size(l_broadcast_attributes), l_broadcast_attributes
                                };


/*  define the XML structure for a tag entry
*/
static  T_XML_Attribute l_tag_attributes[] = {
                                { "group", e_xml_required, NULL, NULL, 0, NULL },
                                { "tag", e_xml_required, NULL, NULL, 0, NULL },
                                { "name", e_xml_implied, "", NULL, 0, NULL },
                                { "abbrev", e_xml_implied, "", NULL, 0, NULL }
                                };
static  T_XML_Element   l_tag = { "tag", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_tag_attributes), l_tag_attributes
                                };


/*  define the XML structure for a field value entry used by a thing
*/
static  T_XML_Attribute l_fieldval_attributes[] = {
                                { "field", e_xml_required, NULL, NULL, 0, NULL },
                                { "value", e_xml_required, NULL, NULL, 0, NULL }
//                                { "tag", e_xml_implied, "", NULL, 0, NULL }
                                };
static  T_XML_Element   l_fieldval = { "fieldval", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_fieldval_attributes), l_fieldval_attributes
                                };


/*  define the XML structure for an array value entry used by a thing
*/
static  T_XML_Attribute l_arrayval_attributes[] = {
                                { "field", e_xml_required, NULL, NULL, 0, NULL },
                                { "index", e_xml_required, NULL, NULL, 0, NULL },
                                { "column", e_xml_implied, "0", NULL, 0, NULL },
                                { "value", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Element   l_arrayval = { "arrayval", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_arrayval_attributes), l_arrayval_attributes
                                };


/*  define a generic test element for use with test tag expressions
*/
static  T_XML_Element   l_test = { "test", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };

/*  define a generic validation element for use with validation scripts
*/
static  T_XML_Element   l_valid = { "evalrule", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };

/*  define the XML structure for a "validate" script
*/
static  T_XML_Element   l_validate = { "validate", false, e_xml_preserve, NULL,
                                0, NULL, 0, NULL
                                };


/*  define the XML structure of a pre-requisite entry used by a thing
*/
static  T_XML_Attribute l_prereq_attributes[] = {
                                { "message", e_xml_required, NULL, NULL, 0, NULL },
                                { "iserror", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no },
                                { "onlyonce", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no },
                                { "issilent", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Child     l_prereq_children[] = {
                                { &l_match, e_xml_optional },
                                { &l_test, e_xml_optional },
                                { &l_valid, e_xml_optional },
                                { &l_validate, e_xml_optional }
                                };
static  T_XML_Element   l_prereq = { "prereq", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_prereq_children), l_prereq_children,
                                x_Array_Size(l_prereq_attributes), l_prereq_attributes
                                };


/*  define the XML structure of a link reference entry used by a thing
*/
static  T_XML_Attribute l_link_attributes[] = {
                                { "linkage", e_xml_required, NULL, NULL, 0, NULL },
                                { "thing", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Element   l_link = { "link", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_link_attributes), l_link_attributes
                                };


/*  define the XML structure of a child entity reference entry used by a thing
*/
static  T_XML_Attribute l_child_attributes[] = {
                                { "entity", e_xml_required, NULL, NULL, 0, NULL }
                                };
static  T_XML_Child     l_child_children[] = {
                                { &l_tag, e_xml_zero_plus },
                                { &l_bootstrap, e_xml_zero_plus }
                                };
static  T_XML_Element   l_child = { "child", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_child_children), l_child_children,
                                x_Array_Size(l_child_attributes), l_child_attributes
                                };


/*  define the XML structure of a child actor entry used by a thing (i.e. minion)
*/
static  T_XML_Attribute l_minion_attributes[] = {
                                { "id", e_xml_required, NULL, NULL, 0, NULL },
                                { "ownmode", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no },
                                { "isinherit", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Child     l_minion_children[] = {
                                { &l_tag, e_xml_zero_plus },
                                { &l_bootstrap, e_xml_zero_plus }
                                };
static  T_XML_Element   l_minion = { "minion", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_minion_children), l_minion_children,
                                x_Array_Size(l_minion_attributes), l_minion_attributes
                                };


/*  define the XML structure of a source reference used by a thing
*/
static  T_XML_Attribute l_usesource_attributes[] = {
                                { "source", e_xml_implied, "", NULL, 0, NULL },
                                { "id", e_xml_implied, "", NULL, 0, NULL },
                                { "parent", e_xml_implied, "", NULL, 0, NULL },
                                { "name", e_xml_implied, "", NULL, 0, NULL }
                                };
static  T_XML_Element   l_usesource = { "usesource", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_usesource_attributes), l_usesource_attributes
                                };


/*  define the XML structure of a "pickreq" reference used by a thing
*/
static  T_XML_Attribute l_pickreq_attributes[] = {
                                { "thing", e_xml_required, NULL, NULL, 0, NULL },
                                { "iserror", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no },
                                { "ispreclude", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Element   l_pickreq = { "pickreq", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_pickreq_attributes), l_pickreq_attributes
                                };


/*  define the XML structure of an "exprreq" reference used by a thing
*/
static  T_XML_Attribute l_exprreq_attributes[] = {
                                { "message", e_xml_required, NULL, NULL, 0, NULL },
                                { "iserror", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Element   l_exprreq = { "exprreq", false, e_xml_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_exprreq_attributes), l_exprreq_attributes
                                };


/*  define the XML structure of a thing
*/
static  T_XML_Attribute l_thing_attributes[] = {
                                { "id", e_xml_required, NULL, NULL, 0, NULL },
                                { "name", e_xml_required, NULL, NULL, 0, NULL },
                                { "description", e_xml_implied, "", NULL, 0, NULL },
                                { "compset", e_xml_required, NULL, NULL, 0, NULL },
                                { "summary", e_xml_implied, "", NULL, 0, NULL },
                                { "condphase", e_xml_implied, "*", NULL, 0, NULL },
                                { "condprior", e_xml_implied, "10000", NULL, 0, NULL },
                                { "maxlimit", e_xml_implied, "0", NULL, 0, NULL },
                                { "buytemplate", e_xml_implied, "", NULL, 0, NULL },
                                { "xactspecial", e_xml_implied, "0", NULL, 0, NULL },
                                { "panellink", e_xml_implied, "", NULL, 0, NULL },
                                { "replaces", e_xml_implied, "", NULL, 0, NULL },
                                { "lotsize", e_xml_implied, "1", NULL, 0, NULL },
                                { "stacking", e_xml_set, l_stacking[0], NULL, SIZE_STACKING, l_stacking },
                                { "uniqueness", e_xml_set, l_uniqueness[0], NULL, SIZE_UNIQUENESS, l_uniqueness },
//NOTE! This attribute is deprecated and replaced by "uniqueness" above but required for backwards compatability
                                { "isunique", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no },
                                { "holdable", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no },
                                { "isprivate", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no },
//NOTE! This attribute is no longer used, but it's required for backwards compatability with user-created material
                                { "sortorder", e_xml_implied, "", NULL, 0, NULL }
                                };
static  T_XML_Child     l_thing_children[] = {
                                { &l_comments, e_xml_optional },
                                { &l_fieldval, e_xml_zero_plus },
                                { &l_arrayval, e_xml_zero_plus },
                                { &l_usesource, e_xml_zero_plus },
                                { &l_tag, e_xml_zero_plus },
                                { &l_bootstrap, e_xml_zero_plus },
                                { &l_condition, e_xml_optional },
                                { &l_condtest, e_xml_optional },
                                { &l_containerreq, e_xml_optional },
                                { &l_gear, e_xml_optional },
                                { &l_broadcast, e_xml_zero_plus },
                                { &l_link, e_xml_zero_plus },
                                { &l_eval, e_xml_zero_plus },
                                { &l_evalrule, e_xml_zero_plus },
                                { &l_pickreq, e_xml_zero_plus },
                                { &l_exprreq, e_xml_zero_plus },
                                { &l_prereq, e_xml_zero_plus },
                                { &l_child, e_xml_optional },
                                { &l_minion, e_xml_optional }
                                };
static  T_XML_Element   l_thing = { "thing", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_thing_children), l_thing_children,
                                x_Array_Size(l_thing_attributes), l_thing_attributes
                                };


static  T_XML_Attribute l_data_attributes[] = {
                                { "signature", e_xml_fixed, DATA_SIGNATURE, NULL, 0, NULL },
                                { "ispartial", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no },
                                };
static  T_XML_Child     l_data_children[] = {
                                { &l_thing, e_xml_zero_plus },
                                };
static  T_XML_Element   l_data = { "document", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_data_children), l_data_children,
                                x_Array_Size(l_data_attributes), l_data_attributes
                                };





/*  define the data structures to reflect the DTD for an augmentation file
*/
static  T_XML_Child     l_extgroup_children[] = {
                                { &l_comments, e_xml_optional }
                                };
static  T_XML_Attribute l_extgroup_attributes[] = {
                                { "group", e_xml_required, NULL, NULL, 0, NULL },
                                { "tag", e_xml_required, NULL, NULL, 0, NULL },
                                { "name", e_xml_implied, "", NULL, 0, NULL },
                                { "heading", e_xml_implied, "", NULL, 0, NULL },
                                { "abbrev", e_xml_implied, "", NULL, 0, NULL },
                                { "order", e_xml_implied, "0", NULL, 0, NULL },
                                { "distinct", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no },
                                { "special", e_xml_implied, "0", NULL, 0, NULL },
                                { "obsolete", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Element   l_extgroup = { "extgroup", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_extgroup_children), l_extgroup_children,
                                x_Array_Size(l_extgroup_attributes), l_extgroup_attributes
                                };


static  T_XML_Child     l_source_children[] = {
                                { &l_comments, e_xml_optional }
                                };
static  T_XML_Attribute l_source_attributes[] = {
                                { "id", e_xml_required, NULL, NULL, 0, NULL },
                                { "name", e_xml_required, NULL, NULL, 0, NULL },
                                { "abbrev", e_xml_implied, "", NULL, 0, NULL },
                                { "description", e_xml_implied, "", NULL, 0, NULL },
                                { "parent", e_xml_implied, "", NULL, 0, NULL },
                                { "sortorder", e_xml_implied, "99999999", NULL, 0, NULL },
                                { "minchoices", e_xml_implied, "0", NULL, 0, NULL },
                                { "maxchoices", e_xml_implied, "0", NULL, 0, NULL },
                                { "selectable", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no },
                                { "reportable", e_xml_set, l_yes_no[0], NULL, SIZE_YES_NO, l_yes_no },
                                { "reportname", e_xml_implied, "", NULL, 0, NULL },
                                { "default", e_xml_set, l_yes_no[1], NULL, SIZE_YES_NO, l_yes_no }
                                };
static  T_XML_Element   l_source = { "source", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_source_children), l_source_children,
                                x_Array_Size(l_source_attributes), l_source_attributes
                                };

static  T_XML_Attribute l_augmentation_attributes[] = {
                                { "signature", e_xml_fixed, AUGMENTATION_SIGNATURE, NULL, 0, NULL }
                                };
static  T_XML_Child     l_augmentation_children[] = {
                                { &l_extgroup, e_xml_zero_plus },
                                { &l_source, e_xml_zero_plus },
                                };
static  T_XML_Element   l_augmentation = { "document", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_augmentation_children), l_augmentation_children,
                                x_Array_Size(l_augmentation_attributes), l_augmentation_attributes
                                };


/*  define the data structures to reflect the DTD for a mapping file
*/
static  T_XML_Attribute l_tuple_attributes[] = {
                                { "a", e_xml_required, NULL, NULL, 0, NULL },
                                { "b", e_xml_implied, "", NULL, 0, NULL },
                                { "c", e_xml_implied, "", NULL, 0, NULL },
                                };
static  T_XML_Element   l_tuple = { "tuple", false, e_xml_no_pcdata, NULL,
                                0, NULL,
                                x_Array_Size(l_tuple_attributes), l_tuple_attributes
                                };

static  T_XML_Child     l_map_children[] = {
                                { &l_tuple, e_xml_one_plus }
                                };
static  T_XML_Attribute l_map_attributes[] = {
                                { "name", e_xml_required, NULL, NULL, 0, NULL },
                                };
static  T_XML_Element   l_map = { "map", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_map_children), l_map_children,
                                x_Array_Size(l_map_attributes), l_map_attributes
                                };

static  T_XML_Attribute l_mapping_attributes[] = {
                                { "signature", e_xml_fixed, MAPPING_SIGNATURE, NULL, 0, NULL }
                                };
static  T_XML_Child     l_mapping_children[] = {
                                { &l_map, e_xml_zero_plus },
                                };
static  T_XML_Element   l_mapping = { "document", false, e_xml_no_pcdata, NULL,
                                x_Array_Size(l_mapping_children), l_mapping_children,
                                x_Array_Size(l_mapping_attributes), l_mapping_attributes
                                };
