/*  FILE:   XMLELEM.CPP

    Copyright (c) 2000-2012 by Lone Wolf Development.  All rights reserved.

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

    Implementation of the Element class for managing XML-based data streams.
*/


#include    "private.h"


/*  define constants used below
*/
#define BLANKS              "                                        "


/*  declare static member variables used below
*/
T_Glyph *       C_XML_Element::s_errors = NULL;
bool            l_is_unknown_trace = true;


/*
    name        --> name of the element (must be matched)
                    empty string ("") indicates that this is a synthetic element
                        intended to handle parenthetical sub-expressions, in
                        which case there is no name to be matched AND there can
                        be NO attributes specified
    is_any      --> whether all children must be matched, in sequence, or any
                        one of the children must be matched (true indicates a
                        syntax of "(a | b | c)")
    pcdata_type --> whether the element can be PCDATA
    valid_func  --> callback function to perform additional validation with
    child_count --> number of child elements (0=Empty)
    children    --> list of child elements
    attrib_count--> number of attributes
    attribs     --> list of attributes
*/
long        C_XML_Element::Construct(const T_Glyph * name,bool is_any,
                                    E_XML_PCDATA pcdata_type,
                                    T_Fn_XML_Validate valid_func,
                                    long child_count,T_XML_Child * children,
                                    long attrib_count,T_XML_Attribute * attribs,
                                    bool is_two_stage,bool is_omit)
{
    long        result;

    /* save the various fields
    */
    m_name = name;
    m_is_any = is_any;
    m_pcdata_type = pcdata_type;
    m_valid_func = valid_func;
    m_child_count = child_count;
    m_children = NULL;
    m_attrib_count = attrib_count;
    m_attrib_list = attribs;
    m_is_omit = is_omit;

    /* allocate storage for the list of child elements (if any)
    */
    if (m_child_count > 0) {
        m_children = new T_XML_Child[m_child_count];
        if (x_Trap_Opt(m_children == NULL))
            return(-900);
        }

    /* if we are not specifically operating in a two-stage process, initialize
        the children properly
    */
    if (!is_two_stage) {
        result = Initialize_Children(children);
        if (x_Trap_Opt(result != 0))
            return(-901);
        }
    return(0);
}


long        C_XML_Element::Initialize_Children(T_XML_Child * children)
{
    long        i;

    /* create each of the children appropriately; this process is a bit
        convoluted, since we have to handle the situation where the DTD is
        recursively defined; if we have recursive, we'll continue infinitely
        unless we recognize when we've already processed a child structure; we
        accomplish this by creating the element object within the actual
        structure providing the definition, as this structure only exists once;
        all we need to do is copy the pointer to the created object into our
        local object and we're set; the remaining problem is cleanup, since more
        than one object will point to the same child and all will attempt to
        delete the object; in addition, we need to cleanup the created object
        within the original structure, else we won't be able to be called more
        than once; to accomplish this, we save a pointer to where the newly
        created element object is stored in the original structure; during our
        destructor, this "reset_ptr" can be used to indicate whether we should
        delete the element and reset the original structure back properly
        NOTE!! This method precludes creating more than one instance of the same
            C_XML_Element hierarchy at any one time.
        NOTE!! The creation process MUST be done in TWO stages. In order for
            recursion to be handled properly, the child object must be created
            first and THEN have its children initialized. If it is done with one
            pass, the children will get processed BEFORE the current object is
            saved into the original structure, which undermines our entire
            effort to properly handle recursively defined DTDs.
    */
    for (i = 0; i < m_child_count; i++) {
        m_children[i].nature = children[i].nature;
        m_children[i].elem_info = children[i].elem_info;
        if (children[i].element == NULL) {
            children[i].element = new C_XML_Element(children[i].elem_info,true);
            children[i].element->Initialize_Children(children[i].elem_info->child_set);
            m_children[i].reset_ptr = &children[i].element;
            }
        else
            m_children[i].reset_ptr = NULL;
        m_children[i].element = children[i].element;
        }
    return(0);
}


C_XML_Element::C_XML_Element(const T_Glyph * name,bool is_any,
                                E_XML_PCDATA pcdata_type,
                                T_Fn_XML_Validate valid_func,
                                long child_count,T_XML_Child * children,
                                long attrib_count,T_XML_Attribute * attribs)
{
    Construct(name,is_any,pcdata_type,valid_func,
                child_count,children,attrib_count,attribs,false,false);
}


C_XML_Element::C_XML_Element(T_XML_Element * info)
{
    Construct(info->name,info->is_any,info->pcdata_type,info->valid_func,
                info->child_count,info->child_set,
                info->attrib_count,info->attrib_list,false,info->is_omit);
}


C_XML_Element::C_XML_Element(T_XML_Element * info,bool is_two_stage)
{
    Construct(info->name,info->is_any,info->pcdata_type,info->valid_func,
                info->child_count,info->child_set,
                info->attrib_count,info->attrib_list,is_two_stage,info->is_omit);
}


C_XML_Element::~C_XML_Element()
{
    long        i;

    /* cleanup all children appropriately; just because the child is non-NULL
        does NOT mean that we should clean it up; since the hierarchy can be
        recursive, it means that the same children can be re-used throughout; in
        order to know whether WE need to delete a child, we need to check the
        reset pointer, since it will be non-NULL only if it is our
        responsibility to perform the cleanup (if NULL, then we are a duplicate
        reference and some other object will perform the cleanup); when we
        delete the child, we also must reset its contents within the original
        definition structure, else we won't be able to be created again properly
    */
    for (i = 0; i < m_child_count; i++)
        if ((m_children[i].element != NULL) &&
            (m_children[i].reset_ptr != NULL) &&
            (*m_children[i].reset_ptr != NULL)) {
            delete m_children[i].element;
            *m_children[i].reset_ptr = NULL;
            }
    if (m_child_count > 0)
        delete [] m_children;
}


void    C_XML_Element::Parse(C_XML_Parser * parser,C_XML_Contents * contents)
{
    long            i,index,errval;
    bool            is_valid,is_token,old_is_ignore,is_done = false;
    const T_Glyph * name;
    T_Glyph *       ptr;
    T_Glyph         buffer[MAX_XML_BUFFER_SIZE * 2], temp[500];

    /* descend a level of depth in the parse
    */
    parser->Descend();
if (parser->Is_Debug()) Debug_Printf("%.*s%s\n",parser->Get_Depth()*2,BLANKS,x_String(m_name));

    /* initialize the contents properly
    */
    contents->Initialize(this);

    /* save the current "is_ignore" state; if we have the special case of both a
        child count and attribute count of -1, enable the is_ignore state
        NOTE! This is a special case to allow arbitrary, non-verified sub-trees
                in an otherwise strictly verified tree.
    */
    old_is_ignore = contents->Is_Ignore();
    if ((m_attrib_count == -1) && (m_child_count == -1))
        contents->Ignore_Unknowns(true);

    /* look for any attributes; if we get an attribute we don't expect, either
        bail out or ignore it, depending on our configuration
    */
    while (!parser->Is_End_Of_Text()) {

        /* grab the next token; if there's no token found, bail out
            NOTE! This test handles trailing whitespace at the end of document.
        */
        is_token = parser->Next_Token();
        if (!is_token)
            break;

        /* if this is the end of the tag, then we're done processing attributes
        */
        if (parser->Is_Tag_Close())
            break;

        /* we need to handle the special case of a tag that has no body
            contents; when this occurs, the tag is terminated within the opening
            tag; we still need to verify that required child tags weren't
            skipped, though
        */
        else if (parser->Is_Tag_Terminal()) {
            if (x_Trap_Opt(!parser->Next_Token() || !parser->Is_Tag_Close())) {
                strcpy(buffer,x_Internal_String(STRING_TAG_CLOSURE));
                errval = -104;
                goto error_exit;
                }
            is_done = true;
            break;
            }

        /* look for a match of this token with any of the attributes that are
            defined for this tag
            NOTE! If the attribute is marked as omitted, we just ignore it.
        */
        name = parser->Get_Token();
        for (index = 0; index < m_attrib_count; index++)
            if (!m_attrib_list[index].is_omit &&
                (strcmp(name,m_attrib_list[index].name) == 0)) {

                /* verify that the attribute is not duplicated and mark it found
                */
                if (x_Trap_Opt(contents->m_is_attrib[index])) {
                    String_Printf(buffer,x_Internal_String(STRING_ATTRIBUTE_REPEATED),name,m_name.c_str());
                    errval = -110;
                    goto error_exit;
                    }
                contents->m_is_attrib[index] = true;

                /* extract the new value and save it
                */
                parser->Parse_Assignment();
                contents->Set_Attribute(index,parser->Get_Token());
                break;
                }

        /* if we did not match anything, it's a problem (unless we are ignoring
            unrecognized tags and attributes)
        */
        if (index >= m_attrib_count)
            if (C_XML_Contents::Is_Ignore()) {
                strcpy(buffer,name);
                parser->Parse_Assignment();
if (l_is_unknown_trace)
Debug_Printf("%.*s*%s = %s (UNKNOWN)\n",(parser->Get_Depth()+1)*2,BLANKS,buffer,parser->Get_Token());
                continue;
                }
            else {
                x_Break_Opt();
                String_Printf(buffer,x_Internal_String(STRING_UNRECOGNIZED_ATTRIBUTE),name,m_name.c_str());
                errval = -105;
                goto error_exit;
                }

        /* if the attribute has a value set, verify that the attribute matches
            one of the valid values specified
        */
        if (m_attrib_list[index].nature == e_xml_set) {
            for (i = 0; i < m_attrib_list[index].set_count; i++)
                if (strcmp(contents->Get_Attribute(index),m_attrib_list[index].value_set[i]) == 0)
                    break;
            if (x_Trap_Opt(i >= m_attrib_list[index].set_count)) {
                String_Printf(buffer,x_Internal_String(STRING_VALUE_NOT_IN_SET),m_attrib_list[index].name);
                errval = -106;
                goto error_exit;
                }
            }

        /* if the attribute is fixed, make sure the value is correct
        */
        else if (m_attrib_list[index].nature == e_xml_fixed) {
            if (x_Trap_Opt(strcmp(contents->Get_Attribute(index),m_attrib_list[index].def_value) != 0)) {
                String_Printf(buffer,x_Internal_String(STRING_FIXED_INCORRECT),m_attrib_list[index].name);
                errval = -107;
                goto error_exit;
                }
            }

        /* dispatch any special validation that must be performed
        */
        if (m_valid_func != NULL) {
            is_valid = m_valid_func(contents->Get_Attribute(index));
            if (x_Trap_Opt(!is_valid)) {
                String_Printf(buffer,x_Internal_String(STRING_ATTRIBUTE_NONCOMPLIANT),m_attrib_list[index].name);
                errval = -108;
                goto error_exit;
                }
            }
        }

    /* verify that all required and fixed attributes were present
    */
    is_valid = true;
    temp[0] = '\0';
    ptr = temp;
    for (i = 0; i < m_attrib_count; i++)
        if (x_Trap_Opt(((m_attrib_list[i].nature == e_xml_required) || (m_attrib_list[i].nature == e_xml_fixed)) &&
                       !contents->m_is_attrib[i])) {
            sprintf(ptr, "%s'%s'", (is_valid ? "" : ", "), m_attrib_list[i].name);
            ptr += strlen(ptr);
            is_valid = false;
            }
    if (!is_valid) {
        String_Printf(buffer,x_Internal_String(STRING_REQUIRED_ATTRIBUTE),temp);
        errval = -109;
        goto error_exit;
        }

if (parser->Is_Debug())
for (i = 0; i < m_attrib_count; i++)
Debug_Printf("%.*s*%s = %s\n",(parser->Get_Depth()+1)*2,BLANKS,m_attrib_list[i].name,contents->Get_Attribute(i));

    /* if we're done (i.e. it's a tag with no contents), check for problems
    */
    if (is_done) {

        /* verify that there are no child tags that are required and missing
        */
        is_valid = true;
        temp[0] = '\0';
        ptr = temp;
        for (i = 0; i < m_child_count; i++)
            if (x_Trap_Opt((m_children[i].nature == e_xml_single) ||
                           (m_children[i].nature == e_xml_one_plus))) {
                sprintf(ptr, "%s'%s'", (is_valid ? "" : ", "), m_children[i].elem_info->name);
                ptr += strlen(ptr);
                is_valid = false;
                }
        if (!is_valid) {
            String_Printf(buffer,x_Internal_String(STRING_TAG_MISSING),temp);
            errval = -111;
            goto error_exit;
            }

        /* wrapup normally
        */
        goto finished;
        }

    /* parse the children of this element
    */
    Parse_Children(parser,contents);

    /* perform validation on this element (if specified)
    */
    if (m_valid_func != NULL) {
        is_valid = m_valid_func(contents->Get_PCDATA());
        if (x_Trap_Opt(!is_valid)) {
            String_Printf(buffer,x_Internal_String(STRING_PCDATA_NONCOMPLIANT),x_String(m_name));
            errval = -121;
            goto error_exit;
            }
        }

    /* ascend a level of depth in the parse and we're done
    */
finished:
    contents->Ignore_Unknowns(old_is_ignore);
if (parser->Is_Debug()) Debug_Printf("%.*s/%s\n",parser->Get_Depth()*2,BLANKS,x_String(m_name));
    parser->Ascend();
    return;

    /* trap an error, cleanup, and we're out of here
    */
error_exit:
    contents->Ignore_Unknowns(old_is_ignore);
    Set_Error(buffer);
    x_Exception(errval);
}


void    C_XML_Element::Parse_Children(C_XML_Parser * parser,
                                        C_XML_Contents * contents)
{
    long                i,next_child,actual[100];
    bool                is_token;
    C_XML_Contents *    child;
    T_Glyph             buffer[500];

    /* initialize the actual count parsed for each child
    */
    for (i = 0; i < m_child_count; i++)
        actual[i] = 0;

    /* look for all valid child tags
    */
    next_child = 0;
    while (!parser->Is_End_Of_Text()) {

        /* grab the next token; if no token is available, we've got a premature
            end of document situation
        */
        is_token = parser->Next_Token();
        if (x_Trap_Opt(!is_token)) {
            Set_Error(x_Internal_String(STRING_PREMATURE_END));
            x_Exception(-122);
            }

        /* if this isn't the start of a new tag, then it is probable the current
            tag contains PCDATA; make sure the tag CAN have PCDATA, then get it,
            after which our tag is complete (so resume the loop at the top)
        */
        if (!parser->Is_Tag_Open()) {
            if (x_Trap_Opt(m_pcdata_type == e_xml_no_pcdata)) {
                String_Printf(buffer,x_Internal_String(STRING_PCDATA_NOT_ALLOWED),x_String(m_name));
                Set_Error(buffer);
                x_Exception(-112);
                }
            parser->Unget_Token();
            parser->Reset_Token();
            parser->Parse_PCData((m_pcdata_type == e_xml_preserve));
            contents->Set_PCDATA(parser->Get_Token());
if (parser->Is_Debug()) Debug_Printf("%.*s PCDATA:%s\n",parser->Get_Depth()*2,BLANKS,contents->Get_PCDATA());
            continue;
            }

        /* grab the next token, which should now be the name of the new tag or
            the terminal tag indicator ("/")
        */
        parser->Next_Token();

        /* if this is a terminal tag, bail out of the loop and cleanup below
        */
        if (parser->Is_Tag_Terminal()) {

            /* close the context by matching the terminal token and close tag -
                if we find something we aren't expecting, raise an error
            */
            if (x_Trap_Opt(!parser->Next_Token() || !(m_name == parser->Get_Token()))) {
                String_Printf(buffer,x_Internal_String(STRING_EXPECTED_CLOSE_TAG),x_String(m_name));
                Set_Error(buffer);
                x_Exception(-113);
                }
            if (x_Trap_Opt(!parser->Next_Token() || !parser->Is_Tag_Close())) {
                String_Printf(buffer,x_Internal_String(STRING_EXPECTED_CLOSE_TAG),x_String(m_name));
                Set_Error(buffer);
                x_Exception(-114);
                }
            break;
        }

        /* it is possible that this is a special tag to start a literal CDATA
            block within PCDATA; if so, we have to recognize it and handle it
            appropriately here
        */
        if (strcmp(parser->Get_Token(),CDATA_START) == 0) {

            /* unget the entire CDATA start token that we retrieved
            */
            parser->Unget_Token();
            parser->Unget_Glyph('<');

            /* reset the token so that we start fresh with the PCDATA
            */
            parser->Reset_Token();

            /* verify that we can retrieve PCDATA for this element, then get it
            */
            if (x_Trap_Opt(m_pcdata_type == e_xml_no_pcdata)) {
                String_Printf(buffer,x_Internal_String(STRING_PCDATA_NOT_ALLOWED),x_String(m_name));
                Set_Error(buffer);
                x_Exception(-112);
                }
            parser->Parse_PCData((m_pcdata_type == e_xml_preserve));
            contents->Set_PCDATA(parser->Get_Token());
if (parser->Is_Debug()) Debug_Printf("%.*s PCDATA:%s\n",parser->Get_Depth()*2,BLANKS,contents->Get_PCDATA());
            continue;
            }

        /* If this is the start of a comments block, parse it and continue.
        */
        else if (strcmp(parser->Get_Token(),COMMENTS_START) == 0) {
            parser->Parse_Comments();
            continue;
            }

        /* if this element is of the "one of any" variety, handle it properly
            NOTE! It is illegal to have an implied sub-expression child for a
                    "one of any" element. This is only possible for an ordered
                    sequence element. Consequently, we don't worry about it.
        */
        if (m_is_any) {

            /* this is the name of a tag, so look for a match among the children
                NOTE! If the element is marked as omitted, we just ignore it.
            */
            for (i = 0; i < m_child_count; i++)
                if (!m_children[i].elem_info->is_omit &&
                    (strcmp(parser->Get_Token(),m_children[i].element->Get_Name()) == 0))
                    break;

            /* if we failed to match anything, handle it appropriately
            */
            if (i >= m_child_count) {
                if (C_XML_Contents::Is_Ignore())
                    Parse_Unknown_Tag(parser);
                else {
                    String_Printf(buffer,x_Internal_String(STRING_UNKNOWN_TAG),parser->Get_Token());
                    Set_Error(buffer);
                    x_Exception(-115);
                    }
                }

            /* otherwise, parse the new child
            */
            else {
                child = contents->Get_New_Child();
                m_children[i].element->Parse(parser,child);
                }
            }

        /* otherwise, retrieve and verify the tags in the proper order
        */
        else {

            /* iterate through the sequence of children until we find the child
                that matches our current tag; also, we have to watch out for
                implied sub-expression children, for which we need to descend a
                level and parse specially
                NOTE!! We must be careful to NOT modify next_child directly
                    here. If this is an unknown tag, then we must NOT increment
                    next_child, else we'll screw up the rest of the parse once
                    the unknown tag is completed. If we match something here,
                    then we'll set next_child equal to the match down below.
                NOTE! If the element is marked as omitted, we just ignore it.
            */
            for (i = next_child; i < m_child_count; i++) {
                if (!m_children[i].elem_info->is_omit &&
                    (strcmp(parser->Get_Token(),m_children[i].element->Get_Name()) == 0))
                    break;
//FIX - this is where we need to splice in handling for implied sub-expressions
                }

            /* if we failed to match anything, handle it appropriately
            */
            if (i >= m_child_count) {
                if (C_XML_Contents::Is_Ignore())
                    Parse_Unknown_Tag(parser);
                else {
                    String_Printf(buffer,x_Internal_String(STRING_UNKNOWN_TAG),parser->Get_Token());
                    Set_Error(buffer);
                    x_Exception(-117);
                    }
                }

            /* otherwise, verify the match and parse the new child
            */
            else {

                /* set the next_child to be the index of the match; this keeps
                    us in sync with where we are in the parse sequence
                */
                next_child = i;

                /* increment the count of this child and verify that it hasn't
                    gone over the limit
                */
                actual[next_child]++;
                switch (m_children[next_child].nature) {
                    case e_xml_single :
                    case e_xml_optional :
                        if (x_Trap_Opt(actual[next_child] > 1)) {
                            String_Printf(buffer,x_Internal_String(STRING_ELEMENT_LIMIT_ONE),
                                        m_children[next_child].elem_info->name,x_String(m_name));
                            Set_Error(buffer);
                            x_Exception(-116);
                            }
                        break;
                    default :
                        break;
                    }

                /* retrieve storage for the new child, then parse the child
                */
                child = contents->Get_New_Child();
                m_children[next_child].element->Parse(parser,child);
                }
            }
        }

    /* if this element is not an "is any", then verify that we have the correct
        number of instances for each child
    */
    if (!m_is_any) {
        for (i = 0; i < m_child_count; i++) {

            /* if this element has been marked as omitted, just skip it
            */
            if (m_children[i].elem_info->is_omit)
                continue;

            /* verify the instance count based on the nature
            */
            switch (m_children[i].nature) {
                case e_xml_single :
                    if (x_Trap_Opt(actual[i] != 1)) {
                        String_Printf(buffer,x_Internal_String(STRING_ELEMENT_EXACTLY_ONCE),
                                    m_children[i].elem_info->name,x_String(m_name));
                        Set_Error(buffer);
                        x_Exception(-118);
                        }
                    break;
                case e_xml_optional :
                    if (x_Trap_Opt(actual[i] > 1)) {
                        String_Printf(buffer,x_Internal_String(STRING_ELEMENT_LIMIT_ONE),
                                    m_children[i].elem_info->name,x_String(m_name));
                        Set_Error(buffer);
                        x_Exception(-119);
                        }
                    break;
                case e_xml_zero_plus :
                    break;
                case e_xml_one_plus :
                    if (x_Trap_Opt(actual[i] == 0)) {
                        String_Printf(buffer,x_Internal_String(STRING_ELEMENT_REQUIRED),
                                    m_children[i].elem_info->name,x_String(m_name));
                        Set_Error(buffer);
                        x_Exception(-120);
                        }
                    break;
                }
            }
        }
}


void    C_XML_Element::Parse_Unknown_Children(C_XML_Parser * parser, T_Glyph_Ptr name)
{
    T_Glyph             buffer[500];

    /* parse and throw away all child tags
    */
    while (!parser->Is_End_Of_Text()) {

        /* grab the next token
        */
        parser->Next_Token();

        /* if this isn't the start of a new tag, then assume this is PCDATA
        */
        if (!parser->Is_Tag_Open()) {
            parser->Unget_Token();
            parser->Reset_Token();
            parser->Parse_PCData(false);
if (parser->Is_Debug()) Debug_Printf("%.*s PCDATA:%s\n",parser->Get_Depth()*2,BLANKS,parser->Get_Token());
            continue;
            }

        /* grab the next token, which should now be the name of the new tag or
            the terminal tag indicator ("/")
        */
        parser->Next_Token();

        /* if this is a terminal tag, bail out of the loop and cleanup below
        */
        if (parser->Is_Tag_Terminal()) {

            /* close the context by matching the terminal token and close tag -
                if we find something we aren't expecting, raise an error
            */
            if (x_Trap_Opt(!parser->Next_Token() || (strcmp(name, parser->Get_Token()) != 0))) {
                String_Printf(buffer,x_Internal_String(STRING_EXPECTED_CLOSE_TAG),name);
                Set_Error(buffer);
                x_Exception(-162);
                }
            if (x_Trap_Opt(!parser->Next_Token() || !parser->Is_Tag_Close())) {
                String_Printf(buffer,x_Internal_String(STRING_EXPECTED_CLOSE_TAG),name);
                Set_Error(buffer);
                x_Exception(-163);
                }

            break;
            }

        /* it is possible that this is a special tag to start a literal CDATA
            block within PCDATA; if so, we have to recognize it and handle it
            appropriately here
        */
        if (strcmp(parser->Get_Token(),CDATA_START) == 0) {

            /* unget the entire CDATA start token that we retrieved
            */
            parser->Unget_Token();
            parser->Unget_Glyph('<');

            /* reset the token so that we start fresh, then get the PCDATA
            */
            parser->Reset_Token();
            parser->Parse_PCData(false);
if (parser->Is_Debug()) Debug_Printf("%.*s PCDATA:%s\n",parser->Get_Depth()*2,BLANKS,parser->Get_Token());
            continue;
            }

        /* If this is the start of a comments block, parse it and continue.
        */
        else if (strcmp(parser->Get_Token(),COMMENTS_START) == 0) {
            parser->Parse_Comments();
            continue;
            }

        /* parse the child tag
        */
        Parse_Unknown_Tag(parser);
        }

    /* if we got here because we ran out of text, the XML was ill-formed
    */
    if (x_Trap_Opt(parser->Is_End_Of_Text())) {
        Set_Error(x_Internal_String(STRING_UNEXPECTED_END));
        x_Exception(-160);
        }
}


void    C_XML_Element::Parse_Unknown_Tag(C_XML_Parser * parser)
{
    bool            is_done = false;
    C_String        attrib;
    C_String        name;
    T_Glyph         buffer[500];

    /* descend a level of depth in the parse
    */
    name = parser->Get_Token();
    parser->Descend();
if (parser->Is_Debug()) Debug_Printf("%.*s%s (UNKNOWN)\n",parser->Get_Depth()*2,BLANKS,x_String(name));

    /* ignore all attributes
    */
    while (!parser->Is_End_Of_Text()) {

        /* grab the next token; if this is the end of the tag, then we're done
            processing attributes
        */
        parser->Next_Token();
        if (parser->Is_Tag_Close())
            break;

        /* we need to handle the special case of a tag that has no body
            contents; when this occurs, the tag is terminated within the opening
            tag; we still need to verify that required child tags weren't
            skipped, though
        */
        else if (parser->Is_Tag_Terminal()) {
            if (x_Trap_Opt(!parser->Next_Token() || !parser->Is_Tag_Close())) {
                String_Printf(buffer,x_Internal_String(STRING_ELEMENT_TERMINATION),x_String(name));
                Set_Error(buffer);
                x_Exception(-161);
                }
            is_done = true;
            break;
            }

        /* parse out the assignment for the attribute
        */
        attrib = parser->Get_Token();
        parser->Parse_Assignment();
if (l_is_unknown_trace)
Debug_Printf("%.*s*%s = %s\n",(parser->Get_Depth()+1)*2,BLANKS,x_String(attrib),parser->Get_Token());
        }

    /* if we're done (i.e. it's a tag with no contents), get out
    */
    if (is_done) {
if (parser->Is_Debug()) Debug_Printf("%.*s/%s\n",parser->Get_Depth()*2,BLANKS,x_String(name));
        parser->Ascend();
        return;
        }

    /* parse the children of this element (make a copy of the name for safety).
    */
    strcpy(buffer, x_String(name));
    Parse_Unknown_Children(parser, buffer);

    /* ascend a level of depth in the parse and we're done
    */
if (parser->Is_Debug()) Debug_Printf("%.*s/%s\n",parser->Get_Depth()*2,BLANKS,x_String(name));
    parser->Ascend();
}


long        C_XML_Element::Get_Maximum_Attributes(long depth)
{
    long    i,count,child;

    /* if we've exceeded our maximum depth, assume recursion and bail out
    */
    if (depth >= 25)
        return(0);

    /* determine the maximum number of attributes for this and all children
    */
    count = m_attrib_count;
    for (i = 0; i < m_child_count; i++) {
        child = Get_Child_Element(i)->Get_Maximum_Attributes(depth + 1);
        if (child > count)
            count = child;
        }

    return(count);
}


void        XML_Enable_Unknown_Trace(bool is_enable)
{
    l_is_unknown_trace = is_enable;
}