/*  FILE:   XML.H

    Copyright (c) 2000-2009 by Lone Wolf Development.  All rights reserved.

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

    Public API definitions for XML data handling.
*/


#ifndef __XML__
#define __XML__


/*  define encoding types
*/
#define XML_ENCODING_ISO_8859_1     "ISO-8859-1"    // "Latin-1"
#define XML_ENCODING_UTF_8          "UTF-8"
#define NUM_ENCODING_TYPES          2


/*  define other important constants
*/
#define MAX_XML_BUFFER_SIZE   20000


/*  define types to encapsulate the size of a character; some platforms require
    that Unicode be utilized, in which case this will not be a "char"
*/
typedef char        T_Glyph;
typedef T_Glyph *   T_Glyph_Ptr;


/*  declare a forward reference to a private class referenced below
*/
class   C_XML_Element;


/*  declare a forward reference to objects referenced circularly below
*/
struct  T_XML_Element;


/*  define a callback function for extra validation of contents; if the data is
    valid, true is returned (else false)
*/
typedef bool        (* T_Fn_XML_Validate)(const T_Glyph * data);


/*  define a macro to compute the size of an array
*/
#define x_Array_Size(array)     (sizeof(array) / sizeof((array)[0]))


/*  define the set of valid XML attribute types
*/
typedef enum {
    e_xml_required,     // attribute is required
    e_xml_implied,      // attribute is optional (any value or none)
    e_xml_fixed,        // attribute must always be the specified value
    e_xml_set           // attribute may be one of a set of values
    }   E_XML_Attribute;


/*  define the structure of an attribute specification
*/
struct  T_XML_Attribute
{
    T_Glyph *           name;       // name of attribute (must match)
    E_XML_Attribute     nature;     // nature of attribute (dictates requirements)
    T_Glyph *           def_value;  // default value for the attribute
    T_Fn_XML_Validate   valid_func; // callback function for extra validation
    long                set_count;  // number of valid values for the attribute
    T_Glyph * *         value_set;  // list of valid values for the attribute
    bool                is_omit;    // whether to omit the attribute from processing
};


/*  define the set of valid XML child element occurrence frequencies
*/
typedef enum {
    e_xml_single,       // element occurs exactly once
    e_xml_optional,     // element may appear 0 or 1 time
    e_xml_one_plus,     // element must appear 1 or more times
    e_xml_zero_plus     // element may appear zero or more times
    }   E_XML_Child;


/*  define the structure of a child specification for an XML element
    NOTE! Only the first two fields must be specified (elem_info & nature).
*/
struct  T_XML_Child
{
    T_XML_Element *     elem_info;  // element info for the child
    E_XML_Child         nature;     // repetition frequency for the element
    C_XML_Element *     element;    // element object used as the child
                                    // used at run-time only
    C_XML_Element **    reset_ptr;  // pointer used for cleanup at destruction
                                    // used at run-time only
};


/*  define the valid set of PCDATA types that can be specified
*/
typedef enum {
    e_xml_no_pcdata,    // element cannot have PCDATA
    e_xml_pcdata,       // element can have PCDATA
    e_xml_preserve      // element can have PCDATA with preserved whitespace
    }   E_XML_PCDATA;


/*  define the structure of an XML element specification
    NOTE! As a special case, if both the child and attribute counts are -1, the
            element is treated as having arbitrary contents that are not
            verified in any way. Only this element and its children are exempt.
*/
struct  T_XML_Element
{
    T_Glyph *           name;       // name of element (must match)
                                    // empty ("") indicates synthetic element to
                                    // handle parenthetical sub-expressions
    bool                is_any;     // whether all children are required or any one
                                    // handles the case of the expr (a | b | c)
    E_XML_PCDATA        pcdata_type;// whether element can contain PCDATA
    T_Fn_XML_Validate   valid_func; // callback function for extra validation
    long                child_count;// number of child elements (0 = Empty)
    T_XML_Child *       child_set;  // list of child elements
    long                attrib_count;// number of attributes
    T_XML_Attribute *   attrib_list;// list of attributes
    bool                is_omit;    // whether to omit the element from processing

    bool                is_output_done;// for internal use only.
};


/*  define public abstract types for managing an XML document and its nodes
*/
typedef struct T_XML_Document_ *    T_XML_Document;
typedef struct T_XML_Node_ *        T_XML_Node;


/*  declare the public API for managing XML documents
*/
void        XML_Handle_Unknowns(bool is_ignore);

long        XML_Read_Document(T_XML_Document * document,T_XML_Element * root,
                                    const T_Glyph * filename,bool is_dynamic = false,
                                    bool is_warnings = false);
long        XML_Write_Document(T_XML_Document document,const T_Glyph * filename,
                                    bool is_detailed = false, bool is_blanks = false);

long        XML_Create_Document(T_XML_Document * document,T_XML_Element * root,
                                    bool is_dynamic = false);
void        XML_Destroy_Document(T_XML_Document document);

long        XML_Extract_Document(T_XML_Document * document,T_XML_Element * root,
                                    const T_Glyph * buffer,bool is_dynamic = false,
                                    bool is_warnings = false);

long        XML_Get_Document_Node(T_XML_Document document,T_XML_Node * node);
long        XML_Get_Named_Child_Count(T_XML_Node node,const T_Glyph * name);
long        XML_Get_First_Named_Child(T_XML_Node node,const T_Glyph * name,
                                        T_XML_Node * child);
long        XML_Get_Next_Named_Child(T_XML_Node node,T_XML_Node * child);
long        XML_Get_Child_Count(T_XML_Node node);
long        XML_Get_First_Child(T_XML_Node node,T_XML_Node * child);
long        XML_Get_Next_Child(T_XML_Node node,T_XML_Node * child);
long        XML_Get_Named_Child_With_Attr_Count(T_XML_Node node,const T_Glyph * name,
                                        const T_Glyph * attr_name, const T_Glyph * attr_value);
long        XML_Get_First_Named_Child_With_Attr(T_XML_Node node,const T_Glyph * name,
                                        const T_Glyph * attr_name, const T_Glyph * attr_value,
                                        T_XML_Node * child);
long        XML_Get_Next_Named_Child_With_Attr(T_XML_Node node,T_XML_Node * child);

long        XML_Get_Name(T_XML_Node node,T_Glyph * name);

bool        XML_Is_PCDATA(T_XML_Node node);
long        XML_Get_PCDATA_Size(T_XML_Node node);
long        XML_Get_PCDATA(T_XML_Node node,T_Glyph * pcdata);
long        XML_Set_PCDATA(T_XML_Node node,const T_Glyph * pcdata,bool use_pool = false);
const T_Glyph * XML_Get_PCDATA_Pointer(T_XML_Node node);

long        XML_Get_First_Attribute(T_XML_Node node,T_Glyph * name);
long        XML_Get_Next_Attribute(T_XML_Node node,T_Glyph * name);

long        XML_Get_Attribute_Size(T_XML_Node node,const T_Glyph * name,bool use_default = true);
long        XML_Get_Attribute(T_XML_Node node,const T_Glyph * name,
                                        T_Glyph * value,bool use_default = true);
long        XML_Set_Attribute(T_XML_Node node,const T_Glyph * name,
                                        const T_Glyph * value,bool use_pool = false);
const T_Glyph * XML_Get_Attribute_Pointer(T_XML_Node node,const T_Glyph * name,bool use_default = true);

long        XML_Create_Child(T_XML_Node node,const T_Glyph * name,
                                        T_XML_Node * child);
long        XML_Delete_Node(T_XML_Node node);
long        XML_Duplicate_Node(T_XML_Node node, T_XML_Node * duplicate, bool is_overwrite = true);
long        XML_Delete_Named_Children(T_XML_Node node, const T_Glyph * name);
long        XML_Delete_Named_Children_With_Attr(T_XML_Node node, const T_Glyph * name,
                                                const T_Glyph * attr_name, const T_Glyph * attr_value);

long        XML_Normalize(T_XML_Document document);
long        XML_Validate(T_XML_Document document);
long        XML_Validate_Node(T_XML_Node node);

void        XML_Set_Error_Buffer(T_Glyph * buffer);
void        XML_Set_Error(T_Glyph * msg);
const T_Glyph * XML_Get_Error(void);

void        XML_Enable_Unknown_Trace(bool is_enable);

long        XML_Get_Line(void);
long        XML_Get_Line(T_XML_Node node);

long        XML_Get_Hierarchy(T_Glyph_Ptr buffer, T_XML_Node node, bool is_skip_top_level = true);

T_Glyph *   XML_Read_Text_Attribute(T_XML_Node xml,T_Glyph * name,T_Glyph * buffer);
bool        XML_Read_Boolean_Attribute(T_XML_Node xml,T_Glyph * name);
long        XML_Read_Integer_Attribute(T_XML_Node xml,T_Glyph * name);
double      XML_Read_Float_Attribute(T_XML_Node xml,T_Glyph * name);

void        XML_Write_Text_Attribute(T_XML_Node xml,T_Glyph * name,const T_Glyph * value);
void        XML_Write_Boolean_Attribute(T_XML_Node xml,T_Glyph * name,bool is_true);
void        XML_Write_Integer_Attribute(T_XML_Node xml,T_Glyph * name,long value);
void        XML_Write_Float_Attribute(T_XML_Node xml,T_Glyph * name,double value);

void        XML_Encode_Decode_Setup(void);
void        XML_Encode_Decode_Wrapup(void);
long        XML_Encode_Node(T_XML_Node node,bool is_children = false);
long        XML_Decode_Node(T_XML_Node node,bool is_children = false);

long        XML_Write_DTD(T_Glyph_Ptr filename, T_XML_Element * root, bool is_html);

long        XML_Set_Default_Character_Encoding(T_Glyph * encoding);
long        XML_Set_Character_Encoding(T_XML_Document document, T_Glyph * encoding);
const T_Glyph * XML_Get_Character_Encoding(T_XML_Document document);

long        XML_Extract_Attribute_Value(T_Glyph_Ptr dest, const T_Glyph * src);

T_Glyph_Ptr XML_Safe(T_Glyph_Ptr dest,T_Glyph_Ptr text);

#endif  // __XML__
