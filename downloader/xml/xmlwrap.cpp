/*  FILE:   XMLWRAP.CPP

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

    Implementation of public wrapper API for XML-based data streams. This file
    is little more than a thin layer around the underlying C++ classes. If you
    are comfortable with C++, then you can use the class methods directly. This
    file is intended as a simpler API for people who are familiar with C and not
    with C++, plus is provides a API that can be readily turned into a DLL that
    can be called from Visual Basic.
*/


#include    "private.h"


/*  define the private structure of an XML document
*/
typedef struct T_XML_Document_ {
    C_XML_Root *        contents;
    C_XML_Element *     root;
    }   T_XML_Document_Body, * T_XML_Document;


/*  declare a buffer for reporting errors into
*/
static  T_Glyph     l_error_buffer[256];
static  T_Glyph *   l_errors = l_error_buffer;
static  long        l_line = 0;


/*  read the contents of a text file into memory as one large string
*/
static  C_String Read_File(const T_Glyph * filename)
{
    C_String        contents;
    std::ifstream   input(filename);
    T_Glyph         buffer[100000];

    if (!input.good())
        x_Exception(-999);

    while (!input.eof()) {
        input.read(buffer,sizeof(buffer));
        contents.append(buffer,(T_Int32U) input.gcount());
        }

    return(contents);
}


/*  Parse the XML data within "text" into the provided document, using the XML
    structure defined within "root".
*/
static  long        Load_XML(T_XML_Document * document,T_XML_Element * root,
                                C_String& text,bool is_dynamic,bool is_warnings)
{
    C_XML_Element * element;
    C_XML_Parser *  parser;
    C_XML_Root *    contents;
    long            retval = 0;

    /* allocate a new parser object
    */
    parser = new C_XML_Parser;
    if (x_Trap_Opt(parser == NULL)) {
        strcpy(l_errors,x_Internal_String(STRING_UNEXPECTED_ERROR));
        return(-2);
        }

    /* create the element hierarchy for the DTD, starting with the given root
    */
    element = new C_XML_Element(root);
    if (x_Trap_Opt(element == NULL)) {
        strcpy(l_errors,x_Internal_String(STRING_UNEXPECTED_ERROR));
        return(-3);
        }

    /* parse the XML document appropriately
        NOTE! To turn on debug tracing, set the last parameter to "true"
    */
    try {
        contents = parser->Parse_Document(element,text,is_dynamic,false);
        }
    catch (C_Exception& exception) {
        if (*l_errors == '\0')
            strcpy(l_errors,x_Internal_String(STRING_UNSPECIFIED_ERROR));
        l_line = C_XML_Parser::Get_Line();
        retval = exception.Get_Error();
        delete parser;
        delete element;
        return(retval);
        }

    /* Check to see if the document root has an ok encoding - if not, we need
        to return a positive status to indicate a warning
    */
    retval = 0;
    if (!contents->Is_Encoding_OK()) {
        strcpy(l_errors, x_Internal_String(STRING_BAD_CHARACTER_ENCODING));
        retval = 100;
        }

    /* If warnings aren't allowed, but we have one, treat it as an error - free
        our memory and return it.
    */
    if (!is_warnings && (retval > 0)) {
        delete parser;
        delete contents;
        delete element;
        return(retval);
        }

    /* create and setup the document object for return
    */
    *document = new T_XML_Document_Body;
    if (x_Trap_Opt(*document == NULL)) {
        strcpy(l_errors,x_Internal_String(STRING_UNEXPECTED_ERROR));
        delete parser;
        delete contents;
        delete element;
        return(-4);
        }
    (*document)->contents = contents;
    (*document)->root = element;

    /* dispose of the parser object that we no longer need
    */
    delete parser;
    return(retval);
}


/* Output everything at the start of an xml dtd
*/
static long         Start_DTD(C_Napkin * output)
{
    /* Validate our parameters
    */
    if (x_Trap_Opt(output == NULL))
        return(-30);

    /* Output the xml version and standalone-ness of the document - for this
        API, it's version 1.0 and 'yes'.
    */
    output->Append("<?xml version=\"1.0\" standalone=\"yes\"?>\n");

    /* Output the document start bit.
    */
    output->Append("<!DOCTYPE document [\n");

    /* Return success.
    */
    return(0);
}


/* Recursive function called to prepare an xml element and its children for
    being output as a DTD.
*/
static long         Prepare_DTD(T_XML_Element * root, T_Int32U depth)
{
    long                i, result;
    T_XML_Child *       child;

    /* Validate our parameters
    */
    if (x_Trap_Opt(root == NULL))
        return(-30);

    /* If our depth is more than 20, then things are probably recursive. Get
        out before we blow the stack.
    */
    if (depth >= 20)
        return(0);

    /* Mark this element as having not been output yet
    */
    root->is_output_done = false;

    /* Prepare all the children of this element
    */
    for (i = 0; i < root->child_count; i++) {
        child = &root->child_set[i];
        result = Prepare_DTD(child->elem_info, depth + 1);
        if (x_Trap_Opt(result != 0))
            return(result);
        }

    /* Return success
    */
    return(0);
}


/* Recursive function called to render an xml element and its children into an
    output napkin provided.
*/
static long         Write_DTD(C_Napkin * output, T_XML_Element * root)
{
    long                i, j, result;
    T_XML_Attribute *   attrib;
    T_XML_Child *       child;

    /* Validate our parameters
    */
    if (x_Trap_Opt((output == NULL) || (root == NULL)))
        return(-30);

    /* If this element has already been output, we need do nothing more
    */
    if (root->is_output_done)
        return(0);

    /* Render the details of this element
    */
    output->Append("<!ELEMENT ");
    output->Append(root->name);
    output->Append(' ');

    /* If the element can have any children, just output "ANY".
    */
    if (root->is_any)
        output->Append("ANY");

    /* If the element has no PCDATA and no children, just output "EMPTY".
    */
    else if ((root->pcdata_type == e_xml_no_pcdata) && (root->child_count == 0))
        output->Append("EMPTY");

    /* Otherwise, render the PCDATA style and children of this element
    */
    else {
        output->Append('(');
        if (root->pcdata_type != e_xml_no_pcdata)
            output->Append("#PCDATA");

        /* Render a summary of all the children of this element
        */
        for (i = 0; i < root->child_count; i++) {
            if ((i != 0) || (root->pcdata_type != e_xml_no_pcdata))
                output->Append(", ");
            child = &root->child_set[i];
            output->Append(child->elem_info->name);
            switch (child->nature) {
                case e_xml_optional :       output->Append('?'); break;
                case e_xml_one_plus :       output->Append('+'); break;
                case e_xml_zero_plus :      output->Append('*'); break;
                case e_xml_single :         break; // default case is e_xml_single, where we append nothing
                }
            }
        output->Append(')');
        }
    output->Append(">\n");

    /* Render all the attributes of this element
    */
    for (i = 0; i < root->attrib_count; i++) {
        output->Append("<!ATTLIST ");
        output->Append(root->name);
        output->Append(' ');
        attrib = &root->attrib_list[i];
        output->Append(attrib->name);
        output->Append(' ');

        /* Output CDATA or the list of valid values for this attribute
        */
        if (attrib->nature != e_xml_set)
            output->Append("CDATA ");
        else {
            output->Append('(');
            for (j = 0; j < attrib->set_count; j++) {
                if (j != 0)
                    output->Append(" | ");
                output->Append(attrib->value_set[j]);
                }
            output->Append(")");
            }

        /* Output the default value for this attribute, or #IMPLIED, or #FIXED
            and then the fixed value.
        */
        switch (attrib->nature) {
            case e_xml_required :       output->Append("#REQUIRED"); break;
            case e_xml_implied :        output->Append("#IMPLIED"); break;
            case e_xml_fixed :          output->Append("#FIXED"); break;
            case e_xml_set :            /* do nothing */ break;
            default :                   x_Trap_Opt(1); return(-31);
            }

        /* Don't output the default value if the attribute is required (since
            there is no default) or if it's empty.
        */
        if ((attrib->nature != e_xml_required) && (attrib->def_value != NULL) && (attrib->def_value[0] != '\0')) {
            output->Append(" \"");
            output->Append(attrib->def_value);
            output->Append('\"');
            }

        /* End this line
        */
        output->Append(">\n");
        }

    /* End this element
    */
    output->Append("\n");

    /* Mark this element as having been fully output - that way, if two or more
        elements have this as a child, it only gets output once. Note that we
        mark the element as done BEFORE we start outputting the children - this
        protects us from infinite recursion when outputting loose-cannon
        recursive DTDs.
    */
    root->is_output_done = true;

    /* Render all the children of this element
    */
    for (i = 0; i < root->child_count; i++) {
        child = &root->child_set[i];
        result = Write_DTD(output, child->elem_info);
        if (x_Trap_Opt(result != 0))
            return(result);
        }

    /* Return success
    */
    return(0);
}


/* Output everything at the end of an xml dtd
*/
static long         Finish_DTD(C_Napkin * output)
{
    /* Validate our parameters
    */
    if (x_Trap_Opt(output == NULL))
        return(-30);

    /* Close the DOCTYPE.
    */
    output->Append("]>\n");

    /* Return success.
    */
    return(0);
}


/* ***************************************************************************
    XML_Extract_Document

    Treat the buffer provided as source data and extract an XML document that
    matches the XML hierarchy definition provided. The hierarchy is given as its
    root node, which is assumed to describe the entire underlying tree of child
    nodes. If the contents correctly match the hierarchy definition, a document
    handle is created and returned that can be used in subsequent calls to this
    API. When the document handle is no longer needed, you should call the
    function XML_Destroy_Document.

    Note! The buffer is no utilized after this function returns, so it may be
    freely discarded by the caller.

    document    <-- created XML document object containing extracted contents
    root        --> pointer to root node of the XML hierarchy definition
    buffer      --> buffer containing the XML contents to parse
    is_dynamic  --> whether to allocate nodes dynamically or use a pool
    is_warnings --> whether warnings should be allowed, or treated as errors
    return      <-- whether the document was read successfully (0 = Success,
                    < 0 = failure, > 0 created with warning)
**************************************************************************** */

long        XML_Extract_Document(T_XML_Document * document,T_XML_Element * root,
                                const T_Glyph * buffer,bool is_dynamic,bool is_warnings)
{
    C_String    text;
    long        result;

    /* reset our error buffer to empty
    */
    *l_errors = '\0';
    l_line = 0;

    /* convert the buffer into an STL string so we can parse it
    */
    text = buffer;

    /* load the XML from the in-memory string
    */
    result = Load_XML(document,root,text,is_dynamic,is_warnings);
    return(result);
}


/* ***************************************************************************
    XML_Read_Document

    Open the file specified and attempt to read its contents in as an XML
    representation that matches the XML hierarchy definition provided. The
    hierarchy is given as its root node, which is assumed to describe the entire
    underlying tree of child nodes. If the contents correctly match the
    hierarchy definition, a document handle is created and returned that can be
    used in subsequent calls to this API. When the document handle is no longer
    needed, you should call the function XML_Destroy_Document.

    Note! The file is opened and closed by this function. No open files are
    maintained, as the entire contents are parsed into memory by this function.

    document    <-- created XML document object containing file contents
    root        --> pointer to root node of the XML hierarchy definition
    filename    --> filename to open and read the XML contents from
    is_dynamic  --> whether to allocate nodes dynamically or use a pool
    is_warnings --> whether warnings should be allowed, or treated as errors
    return      <-- whether the document was read successfully (0 = Success,
                    < 0 = failure, > 0 created with warning)
**************************************************************************** */

long        XML_Read_Document(T_XML_Document * document,T_XML_Element * root,
                                const T_Glyph * filename,bool is_dynamic,bool is_warnings)
{
    C_String    text;
    long        result;

    /* reset our error buffer to empty
    */
    *l_errors = '\0';
    l_line = 0;

    /* read the file into memory so we can parse it
    */
    try {
        text = Read_File(filename);
        }
    catch (...) {
        x_Break_Opt();
        String_Printf(l_errors,x_Internal_String(STRING_FILE_READ_ERROR),filename);
        return(-1);
        }

    /* load the XML from the in-memory string
    */
    result = Load_XML(document,root,text,is_dynamic,is_warnings);
    return(result);
}


/* ***************************************************************************
    XML_Write_Document

    Write the entire contents of the specified XML document object out to the
    filename indicated. Prior to writing the file, the document object is first
    normalized (see XML_Normalize) and then validated (see XML_Validate), which
    ensures that document object is complete and fully compliant with respect to
    the hierarchy definition specified when the document was created.

    document    --> XML document object to be written out to the file
    filename    --> filename to create and write the XML contents out to
    is_detailed --> whether to output attributes that match the default value
    is_blanks   --> whether to add appropriate blanks at the start of lines
    return      <-- whether the document was written successfully (0 = Success)
**************************************************************************** */

long        XML_Write_Document(T_XML_Document document,const T_Glyph * filename,
                                bool is_detailed, bool is_blanks)
{
    C_String            text;
    C_XML_Contents *    contents;

    /* synthesize the complete text stream for the document
    */
    contents = (C_XML_Contents *) document->contents;
    try {
        text = contents->Synthesize_Output(is_detailed, is_blanks);
        }
    catch (...) {
        x_Break_Opt();
        return(-2);
        }

    /* try to write the stream out to the file specified
    */
    try {
        Quick_Write_Text((T_Glyph_Ptr) filename,(T_Glyph_Ptr) text.c_str());
        }
    catch (...) {
        x_Break_Opt();
        return(-1);
        }
    return(0);
}


/* ***************************************************************************
    XML_Create_Document

    Create an empty XML document object that corresponds to the XML hierarchy
    definition provided. The hierarchy is given as its root node, which is
    assumed to describe the entire underlying tree of child nodes. The document
    object will be enforced to comply with the structure stipulated by the
    hierarchy definition. A document handle is created and returned that can be
    used in subsequent calls to this API. When the document handle is no longer
    needed, you should call the function XML_Destroy_Document.

    document    <-- created XML document object that is empty
    root        --> pointer to root node of the XML hierarchy definition
    is_dynamic  --> whether to dynamically allocated all nodes
    return      <-- whether the document was created successfully (0 = Success)
**************************************************************************** */

long        XML_Create_Document(T_XML_Document * document,T_XML_Element * root,
                                    bool is_dynamic)
{
    C_XML_Element * element;
    C_XML_Root *    contents;
    long            result;

    /* create the element hierarchy for the DTD, starting with the given root
    */
    element = new C_XML_Element(root);
    if (x_Trap_Opt(element == NULL)) {
        result = -3;
        goto error_exit;
        }

    /* allocate a root contents object for the document
    */
    contents = new C_XML_Root(element,is_dynamic);
    if (x_Trap_Opt(contents == NULL)) {
        result = -5;
        goto error_exit;
        }

    /* create the document object for use
    */
    *document = new T_XML_Document_Body;
    if (x_Trap_Opt(*document == NULL)) {
        result = -4;
        goto error_exit;
        }

    /* setup and return the new document object
    */
    (*document)->contents = contents;
    (*document)->root = element;
    return(0);

    /* cleanup after an error
    */
error_exit:
    if (element != NULL)
        delete element;
    if (contents != NULL)
        delete contents;
    return(result);
}


/* ***************************************************************************
    XML_Destroy_Document

    Destroy the specified XML document object, releasing all resources that are
    held in memory. Once closed, any future attempts to access this document or
    any of its child nodes will likely result in a program crash.

    document    --> XML document object to discard
    return      <-- whether the document was discarded successfully (0 = Success)
**************************************************************************** */

void        XML_Destroy_Document(T_XML_Document document)
{
    /* dispose of the document contents and root element objects
    */
    if (document->contents != NULL) {
        delete document->contents;
        delete document->root;
        }

    /* dispose of the document itself
    */
    delete document;
}


/* ***************************************************************************
    XML_Normalize

    Perform normalization on the document object, recursing through all children
    during the process. Normalization results in all child tags being sorted
    into compliance with the sequence specified by the hierarchy definition used
    when the document was created.

    document    --> XML document object to normalize
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Normalize(T_XML_Document document)
{
    C_XML_Contents *    contents;
    long                result;

    contents = (C_XML_Contents *) document->contents;
    result = contents->Normalize();
    return(result);
}


/* ***************************************************************************
    XML_Validate

    Perform validation on the document object, recursing through all children
    during the process. Validation checks to ensure that all tags occur with the
    correct frequency specified by the hierarchy definition used when the
    document was created. In addition, all required attributes are verified to
    be specified within the document, and attributes that are governed by a set
    of valid values are confirmed to be one of those values.

    Note! Before validation is performed, the document is automatically
    normalized.

    document    --> XML document object to validate
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Validate(T_XML_Document document)
{
    C_XML_Contents *    contents;
    long                result;

    contents = (C_XML_Contents *) document->contents;
    result = contents->Validate();
    return(result);
}


/* ***************************************************************************
    XML_Validate_Node

    As XML_Validate, above, but the specified node and all children are
    validated, rather than the entire document.

    Note! Before validation is performed, the node is automatically
    normalized.

    node        --> XML node to validate
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Validate_Node(T_XML_Node node)
{
    C_XML_Contents *    contents;
    long                result;

    contents = (C_XML_Contents *) node;
    result = contents->Validate();
    return(result);
}


/* ***************************************************************************
    XML_Get_Document_Node

    Retrieve the XML contents node that corresponds to the root node of the
    specified XML document. All navigational functions in this API utilize
    nodes, and this function retrieves the root node for the document (i.e. the
    starting point for all processing).

    Note! Unlike documents, node handles do NOT allocate any resources, and
    therefore may be ignored once they are no longer needed.

    document    --> XML document object to retrieve the root node for
    return      <-- whether the node was retrieved successfully (0 = Success)
**************************************************************************** */

long        XML_Get_Document_Node(T_XML_Document document,T_XML_Node * node)
{
    *node = (T_XML_Node) document->contents;
    return(0);
}


/* ***************************************************************************
    XML_Get_Child_Count

    Retrieve the total number of all child nodes for the specified node.

    node        --> node to retrieve the child count for
    return      <-- number of children possessed by the node
**************************************************************************** */

long        XML_Get_Child_Count(T_XML_Node node)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Get_Child_Count());
}


/* ***************************************************************************
    XML_Get_First_Child

    Retrieve the first child node of the specified node. If the node has any
    children, the first node will be returned in the provided parameter. To
    retrieve subsequent children of the node, use XML_Get_Next_Child to continue
    iterating through the list.

    Note! Each node maintains a single "next" indicator. Any time that one of
    the "Get_First" functions is invoked, that "next" indicator is reset based
    on the function called and all subsequent calls will utilize the updated
    "next" position.

    node        --> node to retrieve the first child of
    child       <-- first child node of the given parent
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_First_Child(T_XML_Node node,T_XML_Node * child)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    *child = (T_XML_Node) contents->Get_First_Child();
    return((*child == NULL) ? -1 : 0);
}


/* ***************************************************************************
    XML_Get_Next_Child

    Retrieve the next child node of the specified node. This function should be
    called repeatedly until it returns a non-zero result, which indicates that
    all of the node's children have been exhausted. Prior to calling this
    function, call XML_Get_First_Child to initialize the search to the first
    child node.

    node        --> node to retrieve the next child of
    child       <-- next child node of the given parent
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_Next_Child(T_XML_Node node,T_XML_Node * child)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    *child = (T_XML_Node) contents->Get_Next_Child();
    return((*child == NULL) ? -1 : 0);
}


/* ***************************************************************************
    XML_Get_Named_Child_Count

    Retrieve the total number of all child nodes for the specified node that
    possess the indicated name.

    node        --> node to retrieve the child count for
    name        --> name to restrict the search to
    return      <-- number of children possessed by the node with the given name
**************************************************************************** */

long        XML_Get_Named_Child_Count(T_XML_Node node,const T_Glyph * name)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Get_Named_Child_Count(name));
}


/* ***************************************************************************
    XML_Get_First_Named_Child

    This function works the same as XML_Get_First_Child, except that the set of
    children retrieved is restricted to those possessing the specified name.
    Once this function is called, the given name is cached internally, so it
    does not need to be provided in subsequent calls to the function
    XML_Get_Next_Named_Child.

    node        --> node to retrieve the first child of
    name        --> name to restrict the search to
    child       <-- first child node of the given parent with the given name
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_First_Named_Child(T_XML_Node node,const T_Glyph * name,
                                        T_XML_Node * child)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    *child = (T_XML_Node) contents->Get_First_Named_Child(name);
    return((*child == NULL) ? -1 : 0);
}


/* ***************************************************************************
    XML_Get_Next_Named_Child

    This function works the same as XML_Get_Next_Child, except that the set of
    children retrieved is restricted to those possessing the specified name. It
    is assumed that XML_Get_First_Named_Child is called first, as the name given
    to that function call is cached and re-used by calls to this function.

    node        --> node to retrieve the next child of
    child       <-- next child node of the given parent with the established name
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_Next_Named_Child(T_XML_Node node,T_XML_Node * child)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    *child = (T_XML_Node) contents->Get_Next_Named_Child();
    return((*child == NULL) ? -1 : 0);
}


/* ***************************************************************************
    XML_Get_Named_Child_With_Attr_Count

    Retrieve the total number of all child nodes for the specified node that
    possess the indicated name and an indicated attribute with the indicated
    value.

    node        --> node to retrieve the child count for
    name        --> name to restrict the search to
    attr_name   --> name of attribute to restrict the search to
    attr_value  --> value of attribute to restrict the search to
    return      <-- number of children possessed by the node with the given name
**************************************************************************** */

long        XML_Get_Named_Child_With_Attr_Count(T_XML_Node node,const T_Glyph * name,
                                        const T_Glyph * attr_name, const T_Glyph * attr_value)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Get_Named_Child_With_Attr_Count(name, attr_name, attr_value));
}


/* ***************************************************************************
    XML_Get_First_Named_Child_With_Attr

    This function works the same as XML_Get_First_Named_Child, except that the
    set of children retrieved is restricted to those possessing a named attribute
    with the specified value. Once this function is called, the given attribute
    and value are cached internally, so it does not need to be provided in
    subsequent calls to the function XML_Get_Next_Named_Child_With_Attr.

    node        --> node to retrieve the first child of
    name        --> name of element to restrict the search to
    attr_name   --> name of attribute to restrict the search to
    attr_value  --> value of attribute to restrict the search to
    child       <-- first child node of the given parent with the given name
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_First_Named_Child_With_Attr(T_XML_Node node,const T_Glyph * name,
                                        const T_Glyph * attr_name, const T_Glyph * attr_value,
                                        T_XML_Node * child)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    *child = (T_XML_Node) contents->Get_First_Named_Child_With_Attr(name, attr_name, attr_value);
    return((*child == NULL) ? -1 : 0);
}


/* ***************************************************************************
    XML_Get_Next_Named_Child_With_Attr

    This function works the same as XML_Get_Next_Named_Child, except that the set of
    children retrieved is restricted to those possessing a named attribute with
    the specified value. It is assumed that XML_Get_First_Named_Child_With_Attr is
    called first, as the name given to that function call is cached and re-used by
    calls to this function.

    node        --> node to retrieve the next child of
    child       <-- next child node of the given parent with the established name
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_Next_Named_Child_With_Attr(T_XML_Node node,T_XML_Node * child)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    *child = (T_XML_Node) contents->Get_Next_Named_Child_With_Attr();
    return((*child == NULL) ? -1 : 0);
}


/* ***************************************************************************
    XML_Get_Name

    Retrieve the name of the specified node, placing it in the buffer provided.

    node        --> node to retrieve the name for
    name        <-- name of the node
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_Name(T_XML_Node node,T_Glyph * name)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    strcpy(name,contents->Get_Name());
    return(0);
}


/* ***************************************************************************
    XML_Is_PCDATA

    Determine whether the node has PCDATA.

    node        --> node to check for PCDATA
    return      <-- whether the node possesses any PCDATA
**************************************************************************** */

bool        XML_Is_PCDATA(T_XML_Node node)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Is_PCDATA());
}


/* ***************************************************************************
    XML_Get_PCDATA_Size

    Retrieve the size of the PCDATA information stored within the node. The size
    returned DOES include storage for the null-terminator.

    node        --> node to retrieve the PCDATA size for
    return      <-- length in characters of the PCDATA information
**************************************************************************** */

long        XML_Get_PCDATA_Size(T_XML_Node node)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Get_PCDATA_Size());
}


/* ***************************************************************************
    XML_Get_PCDATA

    Retrieve the PCDATA information stored within the node, copying it into the
    buffer provided. The buffer is assumed to be large enough to hold the data
    retrieved. You can determine how large the PCDATA is by calling the function
    XML_Get_PCDATA_Size.

    node        --> node to retrieve the PCDATA information from
    pcdata      <-- buffer into which the PCDATA information is placed
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_PCDATA(T_XML_Node node,T_Glyph * pcdata)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    strcpy(pcdata,contents->Get_PCDATA());
    return(0);
}


/* ***************************************************************************
    XML_Get_PCDATA_Pointer

    Retrieve the PCDATA information stored within the node, returning a direct
    pointer to the data stored internally. This data must be treated as
    read-only, but it avoids the need to copy the data. If the node has no
    PCDATA, then an empty string is returned.

    node        --> node to retrieve the PCDATA information from
    return      <-- contents of PCDATA within the node
**************************************************************************** */

const T_Glyph * XML_Get_PCDATA_Pointer(T_XML_Node node)
{
    C_XML_Contents *    contents;
    const T_Glyph *     ptr;

    contents = (C_XML_Contents *) node;
    ptr = contents->Get_PCDATA();
    return(ptr);
}


/* ***************************************************************************
    XML_Set_PCDATA

    Set the PCDATA information for the node to be the string value specified.

    node        --> node to set the PCDATA information for
    pcdata      --> new information to store as the PCDATA for the node
    use_pool    --> whether to allocate new storage from pool or save the pointer
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Set_PCDATA(T_XML_Node node,const T_Glyph * pcdata,bool use_pool)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    contents->Set_PCDATA(pcdata);
    return(0);
}


/* ***************************************************************************
    XML_Get_First_Attribute

    Retrieve the name of the first attribute of the specified node. If the node
    has any attributes, the name of the first attribute will be returned in the
    provided parameter. To retrieve subsequent attributes of the node, use
    XML_Get_Next_Attribute to continue iterating through the list.

    node        --> node to retrieve the first attribute of
    name        <-- name of the first attribute of the given node
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_First_Attribute(T_XML_Node node,T_Glyph * name)
{
    C_XML_Contents *    contents;
    const T_Glyph *     ptr;

    contents = (C_XML_Contents *) node;
    ptr = contents->Get_First_Attribute_Name();
    if (ptr == NULL)
        return(-1);
    strcpy(name,ptr);
    return(0);
}


/* ***************************************************************************
    XML_Get_Next_Attribute

    Retrieve the name of the next attribute of the specified node. This function
    should be called repeatedly until it returns a non-zero result, which
    indicates that all of the node's attributes have been enumerated. Prior to
    calling this function, call XML_Get_First_Attribute to initialize the search
    to the first attribute.

    node        --> node to retrieve the next attribute of
    name        <-- name of the next attribute of the given node
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_Next_Attribute(T_XML_Node node,T_Glyph * name)
{
    C_XML_Contents *    contents;
    const T_Glyph *     ptr;

    contents = (C_XML_Contents *) node;
    ptr = contents->Get_Next_Attribute_Name();
    if (ptr == NULL)
        return(-1);
    strcpy(name,ptr);
    return(0);
}


/* ***************************************************************************
    XML_Get_Attribute_Size

    Retrieve the size of the information stored for the named attribute within
    the node. The size returned DOES include storage for the null-terminator.

    node        --> node to retrieve the attribute size for
    name        --> name of the attribute to retrieve the size for
    return      <-- length in characters of the attribute contents
**************************************************************************** */

long        XML_Get_Attribute_Size(T_XML_Node node,const T_Glyph * name,bool use_default)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Get_Attribute_Size(name,use_default));
}


/* ***************************************************************************
    XML_Get_Attribute

    Retrieve the information stored for the named attribute within the node,
    copying it into the buffer provided. The buffer is assumed to be large
    enough to hold the data retrieved. You can determine how large the
    attribute's contents are by calling the function XML_Get_Attribute_Size.

    node        --> node to retrieve the attribute contents for
    name        --> name of the attribute to retrieve the contents for
    value       <-- buffer into which the attribute contents are placed
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_Attribute(T_XML_Node node,const T_Glyph * name,T_Glyph * value,bool use_default)
{
    C_XML_Contents *    contents;
    const T_Glyph *     ptr;

    contents = (C_XML_Contents *) node;
    ptr = contents->Get_Attribute(name,use_default);
    if (ptr == NULL)
        return(-1);
    strcpy(value,ptr);
    return(0);
}


/* ***************************************************************************
    XML_Get_Attribute_Pointer

    Retrieve the value of the named attribute within the node, returning a
    direct pointer to the data stored internally. This data must be treated as
    read-only, but it avoids the need to copy the data.

    node        --> node to retrieve the attribute contents for
    name        --> name of the attribute to retrieve the contents for
    return      <-- contents of the attribute within the node
**************************************************************************** */

const T_Glyph * XML_Get_Attribute_Pointer(T_XML_Node node,const T_Glyph * name,bool use_default)
{
    C_XML_Contents *    contents;
    const T_Glyph *     ptr;

    contents = (C_XML_Contents *) node;
    ptr = contents->Get_Attribute(name,use_default);
    return(ptr);
}


/* ***************************************************************************
    XML_Set_Attribute

    Set the information for the named attribute of the node to be the string
    value specified.

    node        --> node to set the attribute information for
    name        --> name of the attribute to set the contents for
    value       --> new information to store as the attribute contents
    use_pool    --> whether to allocate new storage from pool or save the pointer
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Set_Attribute(T_XML_Node node,const T_Glyph * name,
                                const T_Glyph * value,bool use_pool)
{
    C_XML_Contents *    contents;
    long                result;

    contents = (C_XML_Contents *) node;
    result = contents->Set_Attribute(name,value,use_pool);
    return(result);
}


/* ***************************************************************************
    XML_Create_Child

    Create a new node within the document hierarchy. The new node is created as
    a child of the node specified, being an instance of the name indicated. A
    handle the newly created node is returned in the parameter provided. The new
    node will be enforced to comply with all of the structure rules established
    for nodes with the given name.

    node        --> node to create a new child for
    name        --> name of the child node to create
    child       <-- newly created child node
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Create_Child(T_XML_Node node,const T_Glyph * name,
                                    T_XML_Node * child)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    *child = (T_XML_Node) contents->Create_Child(name);
    return((*child == NULL) ? -1 : 0);
}


/* ***************************************************************************
    XML_Delete_Node

    Delete a node within the document hierarchy. The node is removed from the
    XML document and all memory for it is deallocated. (This means that all
    pointers to it, to its children and to its attributes will be invalidated.)

    node        --> node to delete
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Delete_Node(T_XML_Node node)
{
    C_XML_Contents *    contents;
    C_XML_Contents *    parent;
    long                result;

    contents = (C_XML_Contents *) node;
    parent = contents->Get_Parent();

    /* If we don't have a parent node, this is the root, which we shouldn't be
        allowed to delete! Otherwise delete the node from its parent, and free
        the memory allocated for it.
    */
    if (parent == NULL)
        return(-51);
    result = parent->Delete_Child(contents);
    if (result != 0)
        return(result);
    contents->Release();
    return(0);
}


/* ***************************************************************************
    XML_Duplicate_Node

    Given an XML node, this function makes a duplicate with the same attribute
    contents and children, then returns it. If a node is passed in as the
    'duplicate' parameter, its contents are deleted and/or overwritten. If a
    node is passed in as the 'duplicate' parameter, its parent is not changed -
    if no node is passed in, a new node is created with the same parent as the
    first node.

    node        --> node to duplicate
    duplicate   --> target node to receive duplication (if NULL, new node created).
    is_overwrite--> if true, contents of the target node are overwritten. Otherwise
                    they are preserved and the values from the source are merely
                    copied.
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Duplicate_Node(T_XML_Node node, T_XML_Node * duplicate, bool is_overwrite)
{
    C_XML_Contents *    contents;
    C_XML_Contents *    dupe_contents;
    C_XML_Contents *    parent;
    long                result;

    contents = (C_XML_Contents *) node;
    parent = contents->Get_Parent();

    /* If no duplicate node was passed in, create a new node
    */
    if (*duplicate == NULL) {
        result = XML_Create_Child((T_XML_Node) parent, contents->Get_Name(), duplicate);
        if (x_Trap_Opt(result != 0))
            return(result);
        }
    dupe_contents = (C_XML_Contents *) *duplicate;

    /* Begin the duplication, which will proceed depth-first until the entire
        node is duplicated.
    */
    result = dupe_contents->Duplicate_Contents(contents, is_overwrite);
    return(result);
}


/* ***************************************************************************
    XML_Delete_Named_Children

    Deletes all children of a node that have a specified name. Note that if
    there are no children present with the name, or no children present at all,
    this function will still return 0 for success.

    node        --> node to look at
    name        --> name of child nodes to delete
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Delete_Named_Children(T_XML_Node node, const T_Glyph * name)
{
    C_XML_Contents *    contents;
    long                result;

    contents = (C_XML_Contents *) node;
    result = contents->Destroy_Named_Children(name);
    return(result);
}


/* ***************************************************************************
    XML_Delete_Named_Children_With_Attr

    Deletes all children of a node that have a specified name and an attribute
    with the specified name and value. Note that if there are no children
    present with the name, or no children present at all, this function will
    still return 0 for success.

    node        --> node to look at
    name        --> name of child nodes to delete if attribute present
    attr_name   --> name of the attribute to look for
    attr_value  --> value of the attribute required for deletion
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Delete_Named_Children_With_Attr(T_XML_Node node, const T_Glyph * name,
                                                const T_Glyph * attr_name, const T_Glyph * attr_value)
{
    C_XML_Contents *    contents;
    long                result;

    contents = (C_XML_Contents *) node;
    result = contents->Destroy_Named_Children_With_Attr(name, attr_name, attr_value);
    return(result);
}


/* ***************************************************************************
    XML_Handle_Unknowns

    Specify whether the XML parser will ignore tags and attributes that it
    doesn't recognize. If not, then all unrecognized tags/attributes will result
    in a failed parse.

    One of the hallmarks of XML is that any XML document must be well-formed. As
    such, it must always contain exactly the set of information that is
    specified by the DTD. Unfortunately, in the world of evolving software, this
    is not always a good thing. If code is written that assumes a particular DTD
    and then a new version of the DTD is released that contains augmented data,
    the old code will fail to recognize the new contents and require being
    revised. To work around this problem, the XML parser can be told to simply
    "ignore" any new tags and/or attributes that it doesn't recognize. This
    means that a new XML file that only ADDS new information (i.e. no existing
    contents are changed or removed) can continue to be read successfully. It is
    recommended that this feature be always enabled when you are writing code
    that parses XML files that could be evolving over time.

    is_ignore   --> whether parser should ignore unrecognized tags and attributes
**************************************************************************** */

void        XML_Handle_Unknowns(bool is_ignore)
{
    C_XML_Contents::Ignore_Unknowns(is_ignore);
}


/* ***************************************************************************
    XML_Set_Error_Buffer

    Configure the XML engine to use the specified buffer as the buffer in which
    all error messages are written. The caller must ensure that the buffer is
    large enough. If NULL is specified, an internal buffer is used instead.

    buffer      --> buffer to write all error messages into
**************************************************************************** */

void        XML_Set_Error_Buffer(T_Glyph * buffer)
{
    l_errors = (buffer != NULL) ? buffer : l_error_buffer;
    C_XML_Parser::Set_Error_Buffer(l_errors);
    C_XML_Element::Set_Error_Buffer(l_errors);
    C_XML_Contents::Set_Error_Buffer(l_errors);
}


/* ***************************************************************************
    XML_Set_Error

    Set the error message to be the string specified. If an error has already
    been posted, the old error is retained and the new one ignored.

    msg         --> error message to be posted
**************************************************************************** */

void        XML_Set_Error(T_Glyph * msg)
{
    if (*l_errors == '\0')
        strcpy(l_errors,msg);
}


/* ***************************************************************************
    XML_Get_Error

    Retrieve the last error message that was reported by the XML engine.

    return      --> latest error message
**************************************************************************** */

const T_Glyph * XML_Get_Error(void)
{
    return(l_errors);
}


/* ***************************************************************************
    XML_Get_Line

    Retrieve the line number on which the last reported error occurred.

    return      <-- line number on which latest error occurred
**************************************************************************** */

long        XML_Get_Line(void)
{
    return(l_line);
}


/* ***************************************************************************
    XML_Get_Line

    Retrieve the line number on which the specified node begins within the
    source data. This is helpful for reporting meta-errors where the XML is
    technically valid but the contents don't comply with meta-rules for the data.

    node        --> node to get the line number for
    return      <-- line number on which the node begins
**************************************************************************** */

long        XML_Get_Line(T_XML_Node node)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Get_Line());
}


/* ***************************************************************************
    XML_Write_DTD

    Given an XML element to use as the root of a structure, renders the
    structure into DTD format. (The style of the output is 'depth first' -
    that is, the first node is displayed, then the first child of the first
    node, then the first child of that node, and so on.)

    filename    --> name of file to render DTD into
    root        --> XML element to use as the root of the DTD
    is_html     --> Whether to output the DTD as HTML
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Write_DTD(T_Glyph * filename, T_XML_Element * root, bool is_html)
{
// We can't use a C_Grow_String if we're building the public XML stuff, so
// disable this function in that case
#ifdef _LONEWOLF
    C_Napkin        output;
    long            result;
    T_Glyph_Ptr     ptr;
    T_Glyph_Ptr     search[] = { "<", ">", "\n" };
    T_Glyph_Ptr     replace[] = { "&lt;", "&gt;", "<BR>\n" };
    T_Glyph         buffer[10000];
    C_Grow_String   replacer(buffer, sizeof(buffer), 10000);

    /* Render everything that goes before the actual elements in the DTD
    */
    result = Start_DTD(&output);
    if (result != 0)
        return(result);

    /* Prepare the XML structure to be output
    */
    result = Prepare_DTD(root, 0);
    if (result != 0)
        return(result);

    /* Render the elements of the XML structure
    */
    result = Write_DTD(&output, root);
    if (result != 0)
        return(result);

    /* Render everything that should close the DTD
    */
    result = Finish_DTD(&output);
    if (result != 0)
        return(result);
    ptr = output.Get();

    /* If we're outputting HTML, make some changes. We need to replace the
        following: < with "&lt;", > with "&gt;", and append <BR> to all
        lines.
    */
    if (is_html) {
        replacer.Grow_Set_Text(output.Get());
        replacer.Grow_Replace(search, replace, x_Array_Size(search));
        ptr = replacer.Grow_Get_Replace_Text();
        }

    /* Now we have the full DTD, write it out to a file.
    */
    try {
        Quick_Write_Text(filename, ptr);
        result = 0;
        }
    catch (...) {
        x_Break_Opt();
        String_Printf(l_errors,x_Internal_String(STRING_FILE_WRITE_ERROR),filename);
        result = -32;
        }
    return(result);
#else
    return(-1);
#endif
}


/* ***************************************************************************
    XML_Set_Default_Character_Encoding

    Sets the encoding method assumed by the XML layer when none is specified by
    the xml document.

    encoding    --> new encoding method to use
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Set_Default_Character_Encoding(T_Glyph * encoding)
{
    long        result;

    /* set the new document encoding
    */
    result = C_XML_Root::Set_Default_Encoding(encoding);
    return(result);
}


/* ***************************************************************************
    XML_Set_Character_Encoding

    Sets the encoding method used in the XML file. If one is not specified,
    the default is used (this can be found at the top of xml.h).

    document    --> XML document to set encoding for
    encoding    --> new encoding method to use
**************************************************************************** */

long        XML_Set_Character_Encoding(T_XML_Document document, T_Glyph * encoding)
{
    long        result;

    /* set the new document encoding
    */
    result = document->contents->Set_Encoding(encoding);
    return(result);
}


/* ***************************************************************************
    XML_Get_Character_Encoding

    Retrieves the encoding used by an XML file into the provided buffer.

    document    --> XML document to get encoding from
    return      <-- encoding method in use
**************************************************************************** */

const T_Glyph * XML_Get_Character_Encoding(T_XML_Document document)
{
    /* retrieve the document encoding
    */
    return(document->contents->Get_Encoding());
}




/* ***************************************************************************
    XML_Get_Hierarchy

    Retrieves the hierarchy of an XML node and puts it in a buffer. For
    example, if the XML structure is:
    <document>
      <A>
        <B>
          <C/>
          </B>
        </A>
      </document>
    And this function is called on node C, the buffer should contain
    "A -> B -> C" if is_skip_top == true. If is_skip_top == false, the buffer
    would contain "document -> A -> B -> C".

    buffer      --> text buffer to put hierarchy in
    node        --> XML node to retrieve hierarchy for
    is_skip_top --> whether to output the top-level node or not
    return      <-- whether the operation was successful (0 = Success)
**************************************************************************** */

long        XML_Get_Hierarchy(T_Glyph_Ptr buffer, T_XML_Node node, bool is_skip_top)
{
    C_XML_Contents *    contents;

    contents = (C_XML_Contents *) node;
    return(contents->Get_Hierarchy(buffer, is_skip_top));
}