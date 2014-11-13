/*  FILE:   XMLCONT.CPP

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

    Implementation of basic code for managing XML-based data streams.
*/


#include    "private.h"


/*  define constants used below
*/
#define SIZE_INCREMENT      10

#define UNDEFINED           "_$#@*%_"

#define VERSION_TAG         "<?xml version=\"1.0\" encoding=\"%s\"?>"
#define STYLESHEET_TAG      "<?xml-stylesheet href=\"%s\" type=\"%s\"?>"

#define BLANKS              "                                        "
#define INDENT_PER_LEVEL    2

#define DYNAMIC_TEXT_SIZE   30


/*  declare a static variable that is used as a flag below; set this flag to
    "true" in order to output blanks at the start of each line appropriate to
    the current indentation level; this will make the output more readable to
    the human eye, but the file size grows significantly
    NOTE: If this flag OR the XML root blanks flag is set, then extra
        blanks will be output.
*/
static      bool    l_is_blanks = false;


/*  declare static member variables
*/
bool            C_XML_Contents::s_is_ignore = false;
T_Glyph *       C_XML_Contents::s_errors = NULL;
T_Glyph         C_XML_Root::s_default_encoding[] = { XML_ENCODING_UTF_8 };


void        C_Dyna_Text::Set_Text(C_Pool * pool,const T_Glyph * contents)
{
    long     len;

    x_Trap_Opt((pool == NULL) || (contents == NULL));

    /* get the length of the string to save; if storage is needed, acquire it
    */
    len = strlen(contents) + 1;
    if ((m_size == NULL) || (len > *m_size)) {

        /* calculate the storage we need in a block units
        */
        len = ((len - 1) / DYNAMIC_TEXT_SIZE) + 1;
        len *= DYNAMIC_TEXT_SIZE;

        /* acquire the storage we need
        */
        m_size = (short *) pool->Acquire((len * sizeof(T_Glyph)) + sizeof(short));
        if (m_size == NULL)
            x_Exception(-1234);

        /* save the size and apportion storage for our actual string contents
        */
        *m_size = (short) len;
        m_text = (T_Glyph *) (m_size + 1);
        }

    /* save the contents
    */
    strcpy(m_text,contents);
}


C_XML_Root::C_XML_Root(C_XML_Element * root,bool is_dynamic) : C_XML_Contents()
{
    long        size;

    /* start out with some safe values
    */
    m_storage = NULL;
    m_children = NULL;
    m_output = NULL;
    m_is_detailed = false;
    m_is_blanks = false;
    m_encoding = NULL;
    m_is_encoding_ok = true;
    m_style_href = NULL;
    m_style_type = NULL;

    /* save whether we're dynamic or not
    */
    m_is_dynamic = is_dynamic;

    /* calculate our maximum number of attributes throughout the hierarchy
    */
    m_max_attributes = root->Get_Maximum_Attributes();

    /* override the root node to be this node
    */
    m_root = this;

    /* allocate a pool for all of the first memory allocations to be drawn from
    */
    m_storage = new C_Pool(100000,100000);
    if (x_Trap_Opt(m_storage == NULL))
        x_Exception(-10001);

    /* Now we've allocated our pool, set the encoding up appropriately
    */
    Set_Encoding(XML_ENCODING_ISO_8859_1);

    /* allocate a pool for all C_XML_Contents instances to be acquired from
    */
    if (!m_is_dynamic) {
        size = sizeof(C_XML_Contents) * 500;
        m_children = new C_Pool(size,size);
        if (x_Trap_Opt(m_children == NULL))
            x_Exception(-10001);
        }

    /* construct ourselves normally with no parent
        NOTE! This must be done AFTER we setup m_root and allocate all storage.
    */
    Constructor(NULL);

    /* initialize ourselves as the root
    */
    Initialize(root);
}


C_XML_Root::~C_XML_Root()
{
    /* perform standard base class destruction on ourselves
        NOTE! We must do this BEFORE we delete our pool storage below.
    */
    Destructor();

    /* release any rendering napkin and pool objects that were allocated
    */
    if (m_output != NULL)
        delete m_output;
    if (m_storage != NULL)
        delete m_storage;
    if (m_children != NULL)
        delete m_children;
}


C_XML_Contents* C_XML_Root::Acquire_Contents(C_XML_Contents * parent)
{
    C_XML_Contents* child;

    /* if dynamic, use "new" to allocate a contents object; otherwise, allocate
        a contents object from the pool and reset to empty for safety
    */
    if (m_is_dynamic)
        child = new C_XML_Contents();
    else {
        child = (C_XML_Contents *) m_children->Acquire(sizeof(C_XML_Contents));
        memset(child,0,sizeof(C_XML_Contents));
        }

    /* make sure our allocation succeeded
    */
    if (child == NULL) {
        Set_Error(x_Internal_String(STRING_OUT_OF_MEMORY));
        x_Exception(-207);
        }

    /* invoke the manual constructor mechanism
    */
    child->Constructor(parent);

    /* our child is ready for use
    */
    return(child);
}


void        C_XML_Root::Release_Contents(C_XML_Contents * contents)
{
    /* perform standard destruction on the object
    */
    contents->Destructor();

    /* if the object was dynamically allocated, delete it
    */
    if (m_is_dynamic)
        delete contents;
}


void        C_XML_Root::Allocate_Output(void)
{
    m_output = new C_Napkin(500000,500000);
}


long        C_XML_Root::Set_Encoding(const T_Glyph * encoding)
{
    T_Int32U        i, count, length;
    T_Glyph *       ptr;
    long            result = 0;

    m_is_encoding_ok = true;
    count = XML_Get_Character_Encoding_Count();
    for (i = 0; i < count; i++)
        if (stricmp(XML_Get_Character_Encoding_Name(i), encoding) == 0)
            break;

    /* If the encoding is not a known type, set a flag that we can look at
        later
    */
    if (i >= count) {
        result = 500;
        m_is_encoding_ok = false;
        }

    /* Save the encoding
    */
    length = strlen(encoding);
    if ((m_encoding == NULL) || (length > strlen(m_encoding))) {
        ptr = (T_Glyph *) Acquire_Storage((length + 1) * sizeof(T_Glyph));
        m_encoding = ptr;
        }
    strcpy(m_encoding, encoding);
    return(result);
}


void        C_XML_Root::Set_Stylesheet_Href(T_Glyph * href)
{
    x_Trap_Opt(href == NULL);

    m_style_href = (T_Glyph *) Acquire_Storage((strlen(href) + 1) * sizeof(T_Glyph));
    strcpy(m_style_href,href);
}


void        C_XML_Root::Set_Stylesheet_Type(T_Glyph * type)
{
    x_Trap_Opt(type == NULL);

    m_style_type = (T_Glyph *) Acquire_Storage((strlen(type) + 1) * sizeof(T_Glyph));
    strcpy(m_style_type,type);
}


long            C_XML_Root::Set_Default_Encoding(const T_Glyph * encoding)
{
    T_Int32U        i, count;

    /* Make sure the encoding exists
    */
    count = XML_Get_Character_Encoding_Count();
    for (i = 0; i < count; i++)
        if (stricmp(XML_Get_Character_Encoding_Name(i), encoding) == 0)
            break;

    /* If the encoding is not a known type, return failure
    */
    if (i >= count)
        return(-500);

    /* Save the new encoding as our default
    */
    strncpy(s_default_encoding, encoding, MAX_CHARACTER_ENCODING_LENGTH);
    s_default_encoding[MAX_CHARACTER_ENCODING_LENGTH] = '\0';
    return(0);
}


C_XML_Contents::C_XML_Contents(void)
{
    /* do nothing, allowing for pool-based allocation */
}


C_XML_Contents::~C_XML_Contents()
{
    /* do nothing, allowing for pool-based allocation */
}


void    C_XML_Contents::Constructor(C_XML_Contents * parent)
{
    long        i;

    /* initialize the basic member variables appropriately
    */
    m_parent = parent;
    if (parent != NULL)
        m_root = parent->m_root;
    m_pcdata = NULL;
    m_reference = NULL;
    m_count = 0;
    m_contents = NULL;
    m_slots = 0;
    m_next = 0;

    /* allocate sufficient storage for dynamic attributes
    */
    i = sizeof(bool) * m_root->Get_Max_Attributes();
    m_is_attrib = (bool *) m_root->Get_Storage()->Acquire(i);
    i = sizeof(T_Glyph *) * m_root->Get_Max_Attributes();
    m_attrib_text = (T_Glyph **) m_root->Get_Storage()->Acquire(i);
    if (!m_root->Is_Dynamic())
        m_attrib_text_edit = NULL;
    else {
        i = sizeof(C_Dyna_Text) * m_root->Get_Max_Attributes();
        m_attrib_text_edit = (C_Dyna_Text *) m_root->Get_Storage()->Acquire(i);
        }

    /* initialize all attribute values to empty
        NOTE! We must do this AFTER we allocate the storage above.
    */
    for (i = 0; i < m_root->Get_Max_Attributes(); i++)
        m_attrib_text[i] = NULL;

    /* initialize our various dynamic text fields appropriately
    */
    m_child_name.Constructor();
    m_attribute_name.Constructor();
    m_attribute_value.Constructor();
    if (m_root->Is_Dynamic()) {
        m_pcdata_edit.Constructor();
        for (i = 0; i < m_root->Get_Max_Attributes(); i++)
            m_attrib_text_edit[i].Constructor();
        }

    /* set the line number to be whatever the current line number is according
        to the parser
    */
    m_line = C_XML_Parser::Get_Line();
}


void    C_XML_Contents::Destructor(void)
{
    long        i;

    /* if this node has any children, release them appropriately
    */
    if (m_contents != NULL) {
        for (i = 0; i < m_count; i++)
            m_contents[i]->Release();
        free(m_contents);
        }
}


void        C_XML_Contents::Initialize(C_XML_Element * element)
{
    long                i;

    /* reset all attributes to empty
    */
    for (i = 0; i < m_root->Get_Max_Attributes(); i++)
        m_is_attrib[i] = false;

    /* finish initializing the contents object
    */
    m_reference = element;
    m_count = 0;
    m_contents = NULL;
}


void        C_XML_Contents::Release(void)
{
    m_root->Release_Contents(this);
}


C_XML_Contents* C_XML_Contents::Get_New_Child(void)
{
    C_XML_Contents *    child;

    /* if we don't have another child ready to hand out, secure more children
    */
    if (m_count >= m_slots) {

        /* if we haven't allocated storage yet, do so now
        */
        if (m_contents == NULL) {
            m_contents = (C_XML_Contents **) malloc(sizeof(C_XML_Contents *) * SIZE_INCREMENT);
            m_slots = SIZE_INCREMENT;
            }

        /* otherwise, increase the size of the current storage
        */
        else {
            m_slots += SIZE_INCREMENT;
            m_contents = (C_XML_Contents **) realloc(m_contents,sizeof(C_XML_Contents *) * m_slots);
            }

        /* if we couldn't get more memory, we're in deep doodoo
        */
        if (x_Trap_Opt(m_contents == NULL)) {
            Set_Error(x_Internal_String(STRING_OUT_OF_MEMORY));
            x_Exception(-208);
            }
        }

    /* allocate a new contents object, save it in the next available slot, and
        return it to the caller
    */
    child = m_root->Acquire_Contents(this);
    m_contents[m_count++] = child;
    return(child);
}


C_XML_Contents* C_XML_Contents::Create_Child(const T_Glyph * name)
{
    C_XML_Contents* child;
    C_XML_Element * element;
    long            i,count;

    /* get the element reference for the parent (this node) and cycle through
        its children to find the child that references the given element info
    */
    for (i = 0, count = m_reference->Get_Child_Count(); i < count; i++)
        if (strcmp(m_reference->Get_Child_Name(i),name) == 0)
            break;
    if (x_Trap_Opt(i >= count))
        return(NULL);
    element = m_reference->Get_Child_Element(i);

    /* retrieve a new child from the contents storage and initialize it
    */
    child = Get_New_Child();
    child->Initialize(element);
    return(child);
}


//NOTE: This function only deletes a child node from the list of children held
//      by an XML node; it does not release the contents of that child.
long                C_XML_Contents::Delete_Child(C_XML_Contents * child)
{
    long            i, result;

    /* Find the child in the contents list.
    */
    for (i = 0; i < m_count; i++)
        if (m_contents[i] == child)
            break;

    /* If we didn't find it, something is amiss. Return failure.
    */
    if (x_Trap_Opt(i >= m_count))
        return(-50);

    /* Call our overloaded function to delete the node at index i.
    */
    result = Delete_Child(i);
    return(result);
}


//NOTE: This function only deletes a child node from the list of children held
//      by an XML node; it does not release the contents of that child.
long            C_XML_Contents::Delete_Child(long index)
{
    void *          dest;
    void *          source;

    /* Make sure we have a valid index
    */
    if (x_Trap_Opt(index >= m_count))
        return(-50);

    /* If this wasn't the last of the nodes in the list, move the rest of the
        contents list 'up' one space over the deleted node, preserving its order.
    */
    if (index != (m_count - 1)) {
        dest = &(m_contents[index]);
        source = &(m_contents[index+1]);
        memmove(dest, source, (m_count - index - 1) * sizeof(C_XML_Contents *));
        }

    /* If the 'next' member of this object is set at or past the index of the
        deleted node, decrement it. If we don't do this, then m_next ends up
        being the index of one element further along, and we stand the chance
        of missing elements in our search.
    */
    if (m_next >= index)
        m_next--;

    /* Reduce the contents count by one to reflect the deletion, and return
        success.
    */
    m_count--;
    return(0);
}


long            C_XML_Contents::Destroy_Named_Children(const T_Glyph * name)
{
    long                i, result;
    C_XML_Contents *    contents;

    /* If there are no children to delete, just return success.
    */
    if (m_count == 0)
        return(0);

    /* Iterate through the list of children, backwards. That way the number of
        memory moves we have to do (when we delete a child node and move the
        ones below it 'up') is minimized, especially in the case where all the
        children of a node have the same name. It also means we don't have to
        worry about correcting our index when we delete a node, because we've
        already looked at all the nodes that will be affected by the change.
    */
    for (i = (m_count - 1) ; i >= 0; i--)
        if (strcmp(m_contents[i]->Get_Name(),name) == 0) {
            contents = m_contents[i];
            result = Delete_Child(i);
            if (x_Trap_Opt(result != 0))
                return(result);
            contents->Release();
            }
    return(0);
}


long            C_XML_Contents::Destroy_Named_Children_With_Attr(const T_Glyph * name, const T_Glyph * attr_name,
                                                                    const T_Glyph * attr_value)
{
    long                i, result;
    C_XML_Contents *    contents;
    const T_Glyph *     value;

    /* If there are no children to delete, just return success.
    */
    if (m_count == 0)
        return(0);

    /* Iterate through the list of children, backwards. That way the number of
        memory moves we have to do (when we delete a child node and move the
        ones below it 'up') is minimized, especially in the case where all the
        children of a node have the same name. It also means we don't have to
        worry about correcting our index when we delete a node, because we've
        already looked at all the nodes that will be affected by the change.
    */
    for (i = (m_count - 1) ; i >= 0; i--)
        if (strcmp(m_contents[i]->Get_Name(),name) == 0) {
            contents = m_contents[i];
            value = contents->Get_Attribute(attr_name);
            if ((value != NULL) && (strcmp(value, attr_value) == 0)) {
                result = Delete_Child(i);
                if (x_Trap_Opt(result != 0))
                    return(result);
                contents->Release();
                }
            }
    return(0);
}


T_Glyph_CPtr        C_XML_Contents::Get_Name(void)
{
    return(m_reference->Get_Name());
}


C_XML_Contents *    C_XML_Contents::Get_First_Child(void)
{
    m_next = -1;
    return(Get_Next_Child());
}


C_XML_Contents *    C_XML_Contents::Get_Next_Child(void)
{
    if (++m_next >= m_count)
        return(NULL);
    return(m_contents[m_next]);
}


long            C_XML_Contents::Get_Named_Child_Count(const T_Glyph * name)
{
    long        i,count;

    for (i = count = 0; i < m_count; i++)
        if (strcmp(m_contents[i]->Get_Name(),name) == 0)
            count++;
    return(count);
}


C_XML_Contents *    C_XML_Contents::Get_First_Named_Child(const T_Glyph * name)
{
    m_child_name.Set_Text(m_root->Get_Storage(),name);
    m_next = -1;
    return(Get_Next_Named_Child());
}


C_XML_Contents *    C_XML_Contents::Get_Next_Named_Child(void)
{
    for (m_next++; m_next < m_count; m_next++)
        if (strcmp(m_contents[m_next]->Get_Name(),m_child_name.Get_Text()) == 0)
            return(m_contents[m_next]);
    return(NULL);
}


long            C_XML_Contents::Get_Named_Child_With_Attr_Count(const T_Glyph * name,
                                                        const T_Glyph * attr_name, const T_Glyph * attr_value)
{
    long            i,count;
    const T_Glyph * attr;


    for (i = count = 0; i < m_count; i++)
        if (strcmp(m_contents[i]->Get_Name(),name) == 0) {
            attr = m_contents[i]->Get_Attribute(attr_name);
            if (strcmp(((attr == NULL) ? "" : attr),attr_value) == 0)
                count++;
            }
    return(count);
}


C_XML_Contents *    C_XML_Contents::Get_First_Named_Child_With_Attr(const T_Glyph * name,
                                                        const T_Glyph * attr_name, const T_Glyph * attr_value)
{
    m_child_name.Set_Text(m_root->Get_Storage(),name);
    m_attribute_name.Set_Text(m_root->Get_Storage(),attr_name);
    m_attribute_value.Set_Text(m_root->Get_Storage(),attr_value);
    m_next = -1;
    return(Get_Next_Named_Child_With_Attr());
}


C_XML_Contents *    C_XML_Contents::Get_Next_Named_Child_With_Attr(void)
{
    const T_Glyph * attr;

    for (m_next++; m_next < m_count; m_next++)
        if (strcmp(m_contents[m_next]->Get_Name(),m_child_name.Get_Text()) == 0) {
            attr = m_contents[m_next]->Get_Attribute(m_attribute_name.Get_Text());
            if (strcmp(((attr == NULL) ? "" : attr),m_attribute_value.Get_Text()) == 0)
                return(m_contents[m_next]);
                }
    return(NULL);
}


const T_Glyph * C_XML_Contents::Get_First_Attribute_Name(void)
{
    m_next = -1;
    return(Get_Next_Attribute_Name());
}


const T_Glyph * C_XML_Contents::Get_Next_Attribute_Name(void)
{
    if (++m_next >= m_reference->Get_Attribute_Count())
        return(NULL);
    return(m_reference->Get_Attribute(m_next)->name);
}


long            C_XML_Contents::Get_Attribute_Size(const T_Glyph * name,bool use_default)
{
    long            i;

    for (i = 0; i < m_reference->Get_Attribute_Count(); i++)
        if (strcmp(name,m_reference->Get_Attribute(i)->name) == 0)
            break;
    if (x_Trap_Opt(i >= m_reference->Get_Attribute_Count()))
        return(NULL);

    return(Get_Attribute_Size(i,use_default));
}


const T_Glyph * C_XML_Contents::Get_Attribute(const T_Glyph * name,bool use_default)
{
    long        i;

    for (i = 0; i < m_reference->Get_Attribute_Count(); i++)
        if (strcmp(name,m_reference->Get_Attribute(i)->name) == 0)
            break;
    if (x_Trap_Opt(i >= m_reference->Get_Attribute_Count()))
        return(NULL);

    return(Get_Attribute(i,use_default));
}


long            C_XML_Contents::Set_Attribute(const T_Glyph * name,
                                            const T_Glyph * value,bool use_pool)
{
    long        i;

    for (i = 0; i < m_reference->Get_Attribute_Count(); i++)
        if (strcmp(name,m_reference->Get_Attribute(i)->name) == 0)
            break;
    if (x_Trap_Opt(i >= m_reference->Get_Attribute_Count()))
        return(-1);

    return(Set_Attribute(i,value,use_pool));
}


long            C_XML_Contents::Get_Attribute_Size(long index,bool use_default)
{
    const T_Glyph *     ptr;
    T_XML_Attribute *   attribute;

    /* if the original allocation is valid, use its contents
    */
    if (m_attrib_text[index] != NULL)
        return(strlen(m_attrib_text[index]));

    /* if we're not dynamic, use the default value or an empty string
    */
    if (!m_root->Is_Dynamic()) {
        attribute = m_reference->Get_Attribute(index);
        if (use_default && (attribute->def_value != NULL))
            return(strlen(attribute->def_value));
        return(1);
        }

    /* if there is no edited data, then report any default value present
    */
    if (m_attrib_text_edit[index].Is_Empty()) {
        attribute = m_reference->Get_Attribute(index);
        if (use_default && (attribute->def_value != NULL))
            return(strlen(attribute->def_value));
        }

    /* otherwise, use the contents of the edited data
    */
    ptr = m_attrib_text_edit[index].Get_Text();
    return(strlen(ptr) + 1);
}


const T_Glyph * C_XML_Contents::Get_Attribute(long index,bool use_default)
{
    T_XML_Attribute *   attribute;

    /* if the original allocation is valid, get its contents
    */
    if (m_attrib_text[index] != NULL)
        return(m_attrib_text[index]);

    /* if we're not dynamic, use the default value or an empty string
    */
    if (!m_root->Is_Dynamic()) {
        attribute = m_reference->Get_Attribute(index);
        if (use_default && (attribute->def_value != NULL))
            return(attribute->def_value);
        return("");
        }

    /* if there is no edited data, then report any default value present
    */
    if (m_attrib_text_edit[index].Is_Empty()) {
        attribute = m_reference->Get_Attribute(index);
        if (use_default && (attribute->def_value != NULL))
            return(attribute->def_value);
        }

    /* otherwise, get the contents of the edited data
    */
    return(m_attrib_text_edit[index].Get_Text());
}


long            C_XML_Contents::Set_Attribute(long index,const T_Glyph * value,bool use_pool)
{
    /* if there is no value stored yet or we've been explicitly told to use the
        pool, allocate it from the fast storage pool
    */
    if (use_pool || (m_attrib_text[index] == NULL) || !m_root->Is_Dynamic()) {
        if (value == NULL)
            value = "";
        m_attrib_text[index] = (T_Glyph *) m_root->Acquire_Storage(strlen(value) + 1);
        strcpy(m_attrib_text[index],value);
        }

    /* otherwise, save the new value in the edit buffer and clear the original
        allocation to indicate that edited contents are now used
    */
    else {
        x_Trap_Opt(m_attrib_text_edit == NULL);
        m_attrib_text_edit[index].Set_Text(m_root->Get_Storage(),value);
        m_attrib_text[index] = NULL;
        }

    /* set the flag to indicate that we've got bona fide contents for the attrib
    */
    m_is_attrib[index] = true;
    return(0);
}


bool            C_XML_Contents::Is_Attribute_Present(long index)
{
    T_XML_Attribute *   attribute;

    /* if the attribute is specified explicitly, it's present
    */
    if (m_is_attrib[index])
        return(true);

    /* otherwise, check if there is a default value for the attribute
    */
    attribute = m_reference->Get_Attribute(index);
    return(attribute->def_value != NULL);
}


bool            C_XML_Contents::Is_PCDATA(void)
{
    /* if the original allocation is valid, we have data
    */
    if (m_pcdata != NULL)
        return(true);

    /* if we're not dynamic, we don't have data
    */
    if (!m_root->Is_Dynamic())
        return(false);

    /* if we have edited data, we have data
    */
    return(!m_pcdata_edit.Is_Empty());
}


void            C_XML_Contents::Set_PCDATA(const T_Glyph * pcdata,bool use_pool)
{
    /* if there is no PCDATA stored yet or we've been explicitly told to use the
        pool, allocate it from the fast storage pool
    */
    if (use_pool || (m_pcdata == NULL) || !m_root->Is_Dynamic()) {
        if (pcdata == NULL)
            pcdata = "";
        m_pcdata = (T_Glyph *) m_root->Acquire_Storage(strlen(pcdata) + 1);
        strcpy(m_pcdata,pcdata);
        }

    /* otherwise, save the new PCDATA in the edit buffer and clear the original
        allocation to indicate that edited contents are now used
    */
    else {
        m_pcdata_edit.Set_Text(m_root->Get_Storage(),pcdata);
        m_pcdata = NULL;
        }
}


long            C_XML_Contents::Get_PCDATA_Size(void)
{
    const T_Glyph * ptr;

    /* if the original allocation is valid, get its contents
    */
    if (m_pcdata != NULL)
        return(strlen(m_pcdata));

    /* if we're not dynamic, use an empty string
    */
    if (!m_root->Is_Dynamic())
        return(0);

    /* otherwise, get the contents of the edited data
    */
    ptr = m_pcdata_edit.Get_Text();
    return(strlen(ptr));
}


const T_Glyph * C_XML_Contents::Get_PCDATA(void)
{
    /* if the original allocation is valid, get its contents
    */
    if (m_pcdata != NULL)
        return(m_pcdata);

    /* if we're not dynamic, use an empty string
    */
    if (!m_root->Is_Dynamic())
        return("");

    /* otherwise, get the contents of the edited data
    */
    return(m_pcdata_edit.Get_Text());
}


long                C_XML_Contents::Normalize(void)
{
    long                i,j,done,result;
    C_XML_Contents *    tmp;
    const T_Glyph *     name;
    bool                old_is_ignore;

    /* determine whether we're supposed to ignore unrecognized tags; start with
        the default and override in case of the special flag of both child and
        attribute counts being -1
    */
    old_is_ignore = s_is_ignore;
    if ((m_reference->Get_Child_Count() == -1) && (m_reference->Get_Attribute_Count() == -1))
        s_is_ignore = true;

    /* sort all of the children beneath the current node so that they are in the
        exact order specified within the DTD (i.e. the order given by the
        hierarchy)
        NOTE! This process ensures that when we write out the contents, it will
            always be in a valid sequence conforming to the DTD.
    */
    for (done = 0, i = 0; i < m_reference->Get_Child_Count(); i++) {

        /* retrieve the name that should appear next in sequence
        */
        name = m_reference->Get_Child_Name(i);

        /* skip over all children with this name - they're already sorted right!
        */
        for (j = done; j < m_count; j++, done++)
            if (strcmp(m_contents[j]->Get_Name(),name) != 0)
                break;

        /* search for any other children with this name, swapping them into the
            proper next position in the sequence
        */
        for (j++; j < m_count; j++)
            if (strcmp(m_contents[j]->Get_Name(),name) == 0) {
                tmp = m_contents[done];
                m_contents[done] = m_contents[j];
                m_contents[j] = tmp;
                done++;
                }
        }

    /* if we still have children left over, then something is very wrong (unless
        we are supposed to be ignoring unrecognized tags)
    */
    if (!s_is_ignore)
        if (x_Trap_Opt(done < m_count)) {
            result = -300;
            goto finished;
            }

    /* normalize each of the children
    */
    for (i = 0; i < m_count; i++) {
        result = m_contents[i]->Normalize();
        if (result != 0)
            goto finished;
        }

    /* if we got here, all was succesful
    */
    result = 0;

    /* restore the previous "is_ignore" state and return the result
    */
finished:
    s_is_ignore = old_is_ignore;
    return(result);
}


long                C_XML_Contents::Validate_Node(void)
{
    long                i,j,count,next,result;
    bool                old_is_ignore;
    T_XML_Attribute *   attribute;
    const T_Glyph *     name;
    T_Glyph             buffer[500];

    /* if this element is ignored, there's nothing to validate
    */
    if (m_reference->Is_Omit())
        return(0);

    /* determine whether we're supposed to ignore unrecognized tags; start with
        the default and override in case of the special flag of both child and
        attribute counts being -1
    */
    old_is_ignore = s_is_ignore;
    if ((m_reference->Get_Child_Count() == -1) && (m_reference->Get_Attribute_Count() == -1))
        s_is_ignore = true;

    /* iterate through all of the attributes for the node and confirm accuracy
    */
    for (i = 0, count = m_reference->Get_Attribute_Count(); i < count; i++) {

        /* if the attribute is omitted, just ignore it
        */
        attribute = m_reference->Get_Attribute(i);
        if (attribute->is_omit)
            continue;

        /* validate the attribute based on its nature
        */
        switch (attribute->nature) {
            case e_xml_required :
                if (x_Trap_Opt(!Is_Attribute_Present(i))) {
                    result = -200;
                    goto finished;
                    }
                break;
            case e_xml_fixed :
                if (x_Trap_Opt(strcmp(Get_Attribute(i),attribute->def_value) != 0)) {
                    result = -201;
                    goto finished;
                    }
                if (x_Trap_Opt(!Is_Attribute_Present(i))) {
                    result = -201;
                    goto finished;
                    }
                break;
            case e_xml_set :
                for (j = 0; j < attribute->set_count; j++)
                    if (strcmp(Get_Attribute(i),attribute->value_set[j]) == 0)
                        break;
                if (x_Trap_Opt((j >= attribute->set_count) && (attribute->def_value == NULL))) {
                    String_Printf(buffer,x_Internal_String(STRING_BAD_ATTRIBUTE_VALUE),Get_Attribute(i));
                    Set_Error(buffer);
                    result = -202;
                    goto finished;
                    }
                break;
            case e_xml_implied :
                /* nothing to do */
                break;
            }
        }

    /* iterate through all of the children; make sure that the children are in
        the correct order and that the proper number of each is present
    */
    for (i = next = 0, count = m_reference->Get_Child_Count(); i < count; i++) {

        /* retrieve the name that should appear next in sequence
        */
        name = m_reference->Get_Child_Name(i);

        /* if this child element has been marked as omitted, skip it
            NOTE! Do this BEFORE we tally the count below, since the presence of
                    this element is invalid and needs to be treated as leftovers.
        */
        if (m_reference->Get_Child_Element(i)->Is_Omit())
            continue;

        /* count up the number of children that have this name
        */
        for (j = 0; (next + j) < m_count; j++)
            if (strcmp(name,m_contents[next + j]->m_reference->Get_Name()) != 0)
                break;
        next += j;

        /* if this element has free-form contents, there's nothing to check
            NOTE! Be sure to do this AFTER we tally the count above.
        */
        if (m_reference->Is_Any())
            continue;

        /* verify that we have the correct number of children with this name
        */
        switch (m_reference->Get_Child(i)->nature) {
            case e_xml_single :
                if (x_Trap_Opt(j != 1)) {
                    result = -203;
                    goto finished;
                    }
                break;
            case e_xml_optional :
                if (x_Trap_Opt(j > 1)) {
                    result = -204;
                    goto finished;
                    }
                break;
            case e_xml_one_plus :
                if (x_Trap_Opt(j == 0)) {
                    result = -205;
                    goto finished;
                    }
                break;
            case e_xml_zero_plus :
                break;
            }
        }

    /* verify that we don't have any children left over (unless we are supposed
        to ignore unrecognized tags)
    */
    if (!s_is_ignore)
        if (x_Trap_Opt(next < m_count)) {
            result = -206;
            goto finished;
            }

    /* validate each of the children
    */
    for (i = 0; i < m_count; i++) {
        result = m_contents[i]->Validate_Node();
        if (result != 0)
            goto finished;
        }

    /* if we got here, all is well
    */
    result = 0;

    /* restore the original ignore state and return our result
    */
finished:
    s_is_ignore = old_is_ignore;
    return(result);
}


// This method will re-verify that all children satisfy count restrictions that
// are dictated by the child frequency nature.
long                C_XML_Contents::Validate(void)
{
    /* normalize the hierarchy before doing anything else
    */
    Normalize();

    /* start validation with the current node
    */
    return(Validate_Node());
}


void                C_XML_Contents::Output_Node(long depth)
{
    static  T_Glyph l_blanks[] = BLANKS;

    long            i,count;
    T_Glyph         ch,buffer[MAX_XML_BUFFER_SIZE+1];
    T_Glyph *       dest;
    const T_Glyph * src;
    const T_Glyph * ptr;
    const T_Glyph * spec;
    bool            is_special,is_high,is_preserve;
    T_XML_Attribute *   attribute;

    /* if this element is ignored, there's nothing to output
    */
    if (m_reference->Is_Omit())
        return;

    /* indent to the proper depth
    */
    if (l_is_blanks || m_root->Is_Blanks()) {
        l_blanks[depth * INDENT_PER_LEVEL] = '\0';
        m_root->Get_Output()->Append(l_blanks);
        l_blanks[depth * INDENT_PER_LEVEL] = ' ';
        }

    /* start the tag with the node name
    */
    m_root->Get_Output()->Append('<');
    m_root->Get_Output()->Append(m_reference->Get_Name());

    /* output all attributes for the tag
    */
    for (i = 0, count = m_reference->Get_Attribute_Count(); i < count; i++)
        if (Is_Attribute_Present(i)) {

            /* if the attribute has been marked as omitted, just skip it
            */
            attribute = m_reference->Get_Attribute(i);
            if (attribute->is_omit)
                continue;

            /* if the attribute is optional and its value matches the default,
                we can skip it (but only if detailed output is not requested)
            */
            src = Get_Attribute(i);
            if (!m_root->Is_Detailed() &&
                ((attribute->nature == e_xml_implied) || (attribute->nature == e_xml_set)) &&
                (attribute->def_value != NULL) &&
                (strcmp(src,attribute->def_value) == 0))
                continue;

            /* if the attribute value contains any special characters, we must
                convert them
            */
            for (dest = buffer; *src != '\0'; src++)
                switch (*src) {
                    case '<' :  strcpy(dest,"&lt;");    dest += 4;  break;
                    case '>' :  strcpy(dest,"&gt;");    dest += 4;  break;
                    case '&' :  strcpy(dest,"&amp;");   dest += 5;  break;
                    case '\"' : strcpy(dest,"&quot;");  dest += 6;  break;
                    case '\'' : strcpy(dest,"&apos;");  dest += 6;  break;
                    default :
                        if ((T_Int8U) *src <= 127)
                            *dest++ = *src;
                        else {
                            sprintf(dest,"&#%d;",(T_Int8U) *src);
                            dest += strlen(dest);
                            }
                        break;
                    }
            *dest = '\0';

            /* output the mapped attribute value
            */
            m_root->Get_Output()->Append(' ');
            m_root->Get_Output()->Append(m_reference->Get_Attribute(i)->name);
            m_root->Get_Output()->Append('=');
            m_root->Get_Output()->Append('\"');
            m_root->Get_Output()->Append(buffer);
            m_root->Get_Output()->Append('\"');
            }

    /* retrieve the PCDATA for the node
    */
    ptr = Get_PCDATA();

    /* if the node has no children possible and no PCDATA, then terminate the
        tag immediately and we're done
    */
    if ((*ptr == '\0') && (m_reference->Get_Child_Count() == 0)) {
        m_root->Get_Output()->Append("/>");
        m_root->Get_Output()->Append('\n');
        return;
        }

    /* otherwise, close the tag normally and continue
    */
    m_root->Get_Output()->Append('>');

    /* output any PCDATA for the node
    */
    if (*ptr != '\0') {

        /* determine whether the element preserves whitespace
        */
        is_preserve = (m_reference->Get_PCDATA_Type() == e_xml_preserve);

        /* determine if there are any special characters embedded in the text or
            blocks of whitespace that must be preserved; also detect whether any
            characters with ASCII values larger than 127 are present
        */
        for (spec = ptr, is_high = is_special = false; *spec != '\0'; ) {
            ch = *spec++;
            if ((ch == '<') || (ch == '>') || (ch == '&') ||
                (ch == '\'') || (ch == '\"'))
                is_special = true;
            if (!is_preserve)
                if ((ch == '\n') || (ch == '\t') || ((ch == ' ') && (*spec == ' ')))
                    is_special = true;
            if ((T_Int8U) ch >= 128)
                is_high = true;
            }

        /* if the text is a single blank space, convert it to an empty string;
            this is due to our hack with empty strings
            NOTE! Do this AFTER the tests above.
        */
        if ((ptr[0] == ' ') && (ptr[1] == '\0')) {
            buffer[0] = '\0';
            ptr = buffer;
            }

        /* if there are no special characters or high-byte codes, just output
        */
        if (!is_special && !is_high)
            m_root->Get_Output()->Append(ptr);

        /* if there are special characters embedded and no high-byte ASCII
            codes, wrap the entire thing within a CDATA literal block
        */
        else if (is_special && !is_high) {
            m_root->Get_Output()->Append('<');
            m_root->Get_Output()->Append(CDATA_START);
            m_root->Get_Output()->Append(ptr);
            m_root->Get_Output()->Append(CDATA_END);
            }

        /* if there are any high-byte ASCII codes, we have to synthesize our
            output on a character basis to properly escape any codes; while
            we're at it, make sure to put CDATA blocks around blocks of
            whitespace
        */
        else
            for ( ; *ptr != '\0'; ) {

                /* proper handle a block of preserved whitespace (if necessary)
                */
                ch = *ptr++;
                if ((ch == '\n') || (ch == '\t') || ((ch == ' ') && (*ptr == ' '))) {
                    if (is_preserve)
                        m_root->Get_Output()->Append(ch);
                    else {
                        m_root->Get_Output()->Append('<');
                        m_root->Get_Output()->Append(CDATA_START);
                        m_root->Get_Output()->Append(ch);
                        while (*ptr != '\0') {
                            ch = *ptr++;
                            if ((ch == '\n') || (ch == '\t') || ((ch == ' ') && (*ptr == ' ')))
                                m_root->Get_Output()->Append(ch);
                            else
                                break;
                            }
                        m_root->Get_Output()->Append(CDATA_END);
                        }
                    continue;
                    }

                /* recognize appropriate special characters and output correctly
                */
                dest = buffer;
                switch (ch) {
                    case '<' :  strcpy(dest,"&lt;");    dest += 4;  break;
                    case '>' :  strcpy(dest,"&gt;");    dest += 4;  break;
                    case '&' :  strcpy(dest,"&amp;");   dest += 5;  break;
                    case '\"' : strcpy(dest,"&quot;");  dest += 6;  break;
                    case '\'' : strcpy(dest,"&apos;");  dest += 6;  break;
                    default :
                        if ((T_Int8U) ch <= 127) {
                            m_root->Get_Output()->Append(ch);
                            continue;
                            }
                        sprintf(dest,"&#%d;",(T_Int8U) ch);
                        dest += strlen(dest);
                        break;
                    }
                *dest = '\0';
                m_root->Get_Output()->Append(buffer);
                }

        }

    /* if the node has any children, we need to format them specially
    */
    if (m_count > 0) {

        /* increment the depth so everything is properly indented beneath
        */
        depth++;

        /* output all children of the node
            NOTE! Be sure to skip any elements marked as omitted.
        */
        m_root->Get_Output()->Append('\n');
        for (i = 0; i < m_count; i++)
            if (!m_reference->Is_Omit())
                m_contents[i]->Output_Node(depth);
        }

    /* indent properly if we had any children - otherwise the closing element
        will be misaligned
    */
    if ((m_count > 0) && (l_is_blanks || m_root->Is_Blanks())) {
        l_blanks[depth * INDENT_PER_LEVEL] = '\0';
        m_root->Get_Output()->Append(l_blanks);
        l_blanks[depth * INDENT_PER_LEVEL] = ' ';
        }

    /* output the terminal tag for the node
    */
    m_root->Get_Output()->Append("</");
    m_root->Get_Output()->Append(m_reference->Get_Name());
    m_root->Get_Output()->Append('>');
    m_root->Get_Output()->Append('\n');
}


C_String            C_XML_Contents::Synthesize_Output(bool is_detailed, bool is_blanks)
{
    long        result;
    T_Glyph     buffer[1000];

    /* validate the contents
    */
    result = Validate();
    if (result != 0)
        return(NULL);

    /* configure the root with whether the output is detailed / has blanks or not
    */
    m_root->Set_Detailed(is_detailed);
    m_root->Set_Blanks(is_blanks);

    /* allocate a napkin object to incrementally render into - for the ROOT
        node only; if a napkin already exists, just re-use it
    */
    if (m_root->Get_Output() != NULL)
        m_root->Get_Output()->Reset();
    else {
        m_root->Allocate_Output();
        if (x_Trap_Opt(m_root->Get_Output() == NULL))
            return(NULL);
        }

    /* emit the version tag properly
    */
    sprintf(buffer,VERSION_TAG,m_root->Get_Encoding());
    m_root->Get_Output()->Append(buffer);
    m_root->Get_Output()->Append('\n');

    /* if stylesheet information is present, emit the stylesheet tag properly
    */
    if ((m_root->Get_Stylesheet_Href() != NULL) && (m_root->Get_Stylesheet_Type() != NULL)) {
        sprintf(buffer,STYLESHEET_TAG,m_root->Get_Stylesheet_Href(),m_root->Get_Stylesheet_Type());
        m_root->Get_Output()->Append(buffer);
        m_root->Get_Output()->Append('\n');
        }

    /* synthesize the XML output via recursive descent through the nodes
    */
    Output_Node(0);

    /* return the output we synthesized
    */
    return(m_root->Get_Output()->Get());
}


long            C_XML_Contents::Duplicate_Contents(C_XML_Contents * source, bool is_overwrite)
{
    long            result;
    T_Glyph_CPtr    name;
    C_XML_Contents* child;
    C_XML_Contents* node;

    /* Check the names of the nodes to make sure they're the same - we can't
        copy between two nodes with different names, since who knows if they're
        compatible or not.
    */
    if (x_Trap_Opt(strcmp(Get_Name(), source->Get_Name()) != 0))
        return(-1);

    /* Iterate through the attributes of the source, copying them to the target.
    */
    name = Get_First_Attribute_Name();
    while (name != NULL) {

        /* Skip this attribute if not overwriting and it's already set
        */
        if (is_overwrite || (strlen(Get_Attribute(name)) == 0)) {
            result = Set_Attribute(name, source->Get_Attribute(name));
            if (x_Trap_Opt(result != 0))
                return(result);
            }

        name = Get_Next_Attribute_Name();
        }

    /* Duplicate the PCDATA of the source node
    */
    Set_PCDATA(source->Get_PCDATA());

    /* Delete all our children, so they don't interfere with whatever we're
        duplicating, if we have to overwrite the old node.
    */
    if (is_overwrite) {
        child = Get_First_Child();
        while (child != NULL) {
            result = Delete_Child(child);
            if (x_Trap_Opt(result != 0))
                return(result);
            child->Release();

            /* Since we just deleted our first child, we can get the first child
                again!
            */
            child = Get_First_Child();
            }
        }

    /* Iterate through the children of our source
    */
    node = source->Get_First_Child();
    while (node != NULL) {

        /* Create a new child on the target
        */
        child = Create_Child(node->Get_Name());
        if (x_Trap_Opt(child == NULL))
            return(-1);

        /* Recursively duplicate the child node.
        */
        result = child->Duplicate_Contents(node);
        if (x_Trap_Opt(result != 0))
            return(result);

        /* Do the same with the next child on the source.
        */
        node = source->Get_Next_Child();
        }

    return(0);
}


long            C_XML_Contents::Get_Hierarchy(T_Glyph_Ptr buffer, bool is_skip_top_level)
{
    T_Glyph_Ptr         ptr;

    /* If we don't have a parent node, empty our buffer. Otherwise, call this
        method recursively on our parent and append a separator.
    */
    if (m_parent == NULL)
        buffer[0] = '\0';
    else
        m_parent->Get_Hierarchy(buffer, is_skip_top_level);

    /* Find the current end of the buffer, where we'll need to append things.
        If stuff has been added to it already, add a separator.
    */
    ptr = buffer + strlen(buffer);
    if (ptr != buffer) {
        strcpy(ptr, " -> ");
        ptr += strlen(ptr);
        }

    /* Append the name of this node to the hierarchy, if appropriate
    */
    if (!is_skip_top_level || (m_parent != NULL))
        strcpy(ptr, Get_Name());
    return(0);
}
