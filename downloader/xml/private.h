/*  FILE:   PRIVATE.H

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

    Private definitions for managing XML-based data streams.
*/


/*  include the public definitions for the XML interface
*/
#ifdef  _LONEWOLF
#include    "xml.h"
#else
#include    "pubxml.h"
#endif


/*  include the necessary headers to support the XML implementation
*/
#include    <stdio.h>   // only needed for debug tracing
#include    <iostream>
#include    <fstream>
#include    <string>

#ifdef  _LONEWOLF
#include    <utilplus.h>
#else
#include    "tools.h"
#endif


/*  OS X uses slightly different names for these functions, but they're
    otherwise the same
*/
#if defined(_OSX)
#define     stricmp     strcasecmp
#define     strnicmp    strncasecmp
#endif


/*  define a basic type to encapsulate the string class; some compilers require
    the "std::" namespace specification, while others choke on it; you can
    change this typedef to make everything work properly throughout the code
*/
typedef std::string     C_String;


/*  define the set of internal string ids
*/
#define STRING_OUT_OF_MEMORY            100
#define STRING_OUT_OF_RESOURCES         101
#define STRING_UNEXPECTED_ERROR         102
#define STRING_UNSPECIFIED_ERROR        103
#define STRING_FILE_READ_ERROR          104
#define STRING_FILE_WRITE_ERROR         105

#define STRING_TAG_CLOSURE              200
#define STRING_ATTRIBUTE_REPEATED       201
#define STRING_UNRECOGNIZED_ATTRIBUTE   202
#define STRING_VALUE_NOT_IN_SET         203
#define STRING_FIXED_INCORRECT          204
#define STRING_ATTRIBUTE_NONCOMPLIANT   205
#define STRING_REQUIRED_ATTRIBUTE       206
#define STRING_TAG_MISSING              207
#define STRING_PCDATA_NONCOMPLIANT      208
#define STRING_EXPECTED_CLOSE_TAG       209
#define STRING_PCDATA_NOT_ALLOWED       210
#define STRING_UNKNOWN_TAG              211
#define STRING_ELEMENT_LIMIT_ONE        212
#define STRING_ELEMENT_EXACTLY_ONCE     213
#define STRING_ELEMENT_REQUIRED         214
#define STRING_UNEXPECTED_END           215
#define STRING_CDATA_TERMINATION        216
#define STRING_SPECIAL_TOKEN            217
#define STRING_ASSIGNMENT_SYNTAX        218
#define STRING_BAD_VERSION_TAG          219
#define STRING_EXPECTED_START_TAG       220
#define STRING_BAD_TOPLEVEL_TAG         221
#define STRING_COMMENT_DOUBLE_DASH      222
#define STRING_COMMENT_TERMINATION      223
#define STRING_BAD_CHARACTER_ENCODING   224
#define STRING_ELEMENT_TERMINATION      225
#define STRING_BAD_ATTRIBUTE_VALUE      226
#define STRING_PREMATURE_END            227
#define STRING_ATTRIBUTE_YES_NO         228
#define STRING_EXTRANEOUS_DATA          229
#define STRING_BAD_STYLESHEET_TAG       230
#define STRING_INVALID_XML_DOCUMENT     231


/*  declare an external reference to the internal strings table and a macro to
    retrieve strings from the table
*/
extern  T_Internal_String_Table g_xml_strings;

#ifdef  _LONEWOLF
#define x_Internal_String(id)   Core_Load_String(id,&g_xml_strings)
#else
#define MODULE_ID_XML       0
#define x_Internal_String(id)   Tools_Load_String(id,&g_xml_strings)
#endif


/*  define a macro to retrieve the C-string from within an STL string object;
    this macro handles the special case where the STL treats a string with empty
    contents as NULL
*/
#define x_String(str)   (((str).c_str() == NULL) ? "" : (str).c_str())


/*  define shared constants for the XML mechanism
*/
#define CDATA_START                         "![CDATA["
#define CDATA_END                           "]]>"
#define COMMENTS_START                      "!--"
#define MAX_CHARACTER_ENCODING_LENGTH       100


/*  declare all classes in advance to enable circular references
*/
struct  T_XML_Child;
struct  T_XML_Element;
class   C_XML_Parser;
class   C_XML_Element;
class   C_XML_Contents;
class   C_XML_Root;


/*  declare a class for managing a dynamically sized string using pool storage
    NOTE! This class can be block allocated due to empty constructor/destructor.
*/
class   C_Dyna_Text
{
public:
    C_Dyna_Text(void)
        { /* do nothing */ }
    ~C_Dyna_Text()
        { /* do nothing */ }

    inline  T_Glyph *   Get_Text(void)
                            { return((m_text == NULL) ? (T_Glyph_Ptr) "" : m_text); }
    inline  bool        Is_Empty(void)
                            { return(m_size == NULL); }

    inline  void        Constructor(void)
                            { m_size = NULL; m_text = ""; }

    void        Set_Text(C_Pool * pool,const T_Glyph * contents);

private:
    short *         m_size;
    T_Glyph *       m_text;
};


/*  declare a class for managing the recursively defined contents of an XML
    document
*/
class   C_XML_Contents
{
public:
    C_XML_Contents(void);
    ~C_XML_Contents();

    static  inline  void    Ignore_Unknowns(bool is_ignore)
                                { s_is_ignore = is_ignore; }
    static  inline  bool    Is_Ignore(void)
                                { return(s_is_ignore); }

    static  inline  void    Set_Error_Buffer(T_Glyph * buffer)
                                { s_errors = buffer; }

    inline  C_XML_Contents* Get_Parent(void)
                                { return(m_parent); }

    inline  long            Get_Child_Count(void)
                                { return(m_count); }

    inline  long            Get_Line(void)
                                { return(m_line); }

    void                Release(void);

    C_XML_Contents *    Create_Child(const T_Glyph * name);
    long                Delete_Child(C_XML_Contents * child);
    long                Destroy_Named_Children(const T_Glyph * name);
    long                Destroy_Named_Children_With_Attr(const T_Glyph * name, const T_Glyph * attr_name,
                                                            const T_Glyph * attr_value);

    C_XML_Contents *    Get_New_Child(void);

    T_Glyph_CPtr        Get_Name(void);

    C_XML_Contents *    Get_First_Child(void);
    C_XML_Contents *    Get_Next_Child(void);

    long                Get_Named_Child_Count(const T_Glyph * name);
    C_XML_Contents *    Get_First_Named_Child(const T_Glyph * name);
    C_XML_Contents *    Get_Next_Named_Child(void);

    long                Get_Named_Child_With_Attr_Count(const T_Glyph * name,
                                                const T_Glyph * attr_name, const T_Glyph * attr_value);
    C_XML_Contents *    Get_First_Named_Child_With_Attr(const T_Glyph * name,
                                                const T_Glyph * attr_name, const T_Glyph * attr_value);
    C_XML_Contents *    Get_Next_Named_Child_With_Attr(void);

    const T_Glyph *     Get_First_Attribute_Name(void);
    const T_Glyph *     Get_Next_Attribute_Name(void);

    long                Set_Attribute(const T_Glyph * name,
                                        const T_Glyph * value,bool use_pool = false);
    long                Get_Attribute_Size(const T_Glyph * name,bool use_default = true);
    const T_Glyph *     Get_Attribute(const T_Glyph * name,bool use_default = true);

    bool                Is_PCDATA(void);
    void                Set_PCDATA(const T_Glyph * pcdata,bool use_pool = false);
    long                Get_PCDATA_Size(void);
    const T_Glyph *     Get_PCDATA(void);

    void                Encode(bool is_children = false);
    void                Decode(bool is_children = false);

    long                Normalize(void);
    long                Validate(void);

    long                Duplicate_Contents(C_XML_Contents * source, bool is_overwrite = true);

    C_String            Synthesize_Output(bool is_detailed = false, bool is_blanks = false);

    long                Get_Hierarchy(T_Glyph_Ptr buffer, bool is_skip_top_level);

protected:
    inline  void        Set_Error(T_Glyph * msg)
                            { if (*s_errors == '\0') strcpy(s_errors,msg); }

    long                Delete_Child(long index);

    void                Constructor(C_XML_Contents * parent);
    void                Initialize(C_XML_Element * element);
    void                Destructor(void);

    C_XML_Root *        m_root;     // root node in the hierarchy

private:
    long                Set_Attribute(long index,const T_Glyph * value,bool use_pool = false);
    long                Get_Attribute_Size(long index,bool use_default = true);
    const T_Glyph *     Get_Attribute(long index,bool use_default = true);

    bool                Is_Attribute_Present(long index);

    static  bool            s_is_ignore;// whether to ignore unknown tags/attributes
    static  T_Glyph *       s_errors;

    C_XML_Element *     m_reference;// element that the contents pertain to
    T_Glyph *           m_pcdata;   // any PCDATA for the instance
    C_Dyna_Text         m_pcdata_edit;// any edited PCDATA for the instance
    long                m_count;    // number of children
    C_XML_Contents **   m_contents; // list of child contents instances
    T_Glyph **          m_attrib_text;// attribute text for instance
    C_Dyna_Text *       m_attrib_text_edit;// any edited attribute text
    bool *              m_is_attrib;// whether each attrib was defined
    long                m_slots;    // number of child slots allocated thus far
    long                m_next;     // index of next thing to retrieve
    C_Dyna_Text         m_child_name;// name of child elements to retrieve
    C_Dyna_Text         m_attribute_name;// attribute name used when retrieving an element
    C_Dyna_Text         m_attribute_value;// attribute value used when retrieving an element
    C_XML_Contents *    m_parent;   // parent node in the hierarchy
    long                m_line;

    friend class    C_XML_Root;
    friend class    C_XML_Element;

    long                Validate_Node(void);
    void                Output_Node(long depth);
};


/*  declare a class for the root node contents of an XML document
*/
class   C_XML_Root  : public C_XML_Contents
{
public:
    C_XML_Root(C_XML_Element * root,bool is_dynamic = false);
    ~C_XML_Root();

    static inline const T_Glyph *   Get_Default_Encoding(void)
                                        { return(s_default_encoding); }

    static  long            Set_Default_Encoding(const T_Glyph * encoding);

    inline  C_Pool *    Get_Storage(void)
                            { return(m_storage); }
    inline  C_Napkin *  Get_Output(void)
                            { return(m_output); }

    inline  void *      Acquire_Storage(long size)
                            { return(m_storage->Acquire(size)); }

    inline  long        Get_Max_Attributes(void)
                            { return(m_max_attributes); }

    inline  bool        Is_Dynamic(void)
                            { return(m_is_dynamic); }

    inline  void        Set_Detailed(bool is_detailed)
                            { m_is_detailed = is_detailed; }
    inline  void        Set_Blanks(bool is_blanks)
                            { m_is_blanks = is_blanks; }
    inline  bool        Is_Detailed(void)
                            { return(m_is_detailed); }
    inline  bool        Is_Blanks(void)
                            { return(m_is_blanks); }

    inline  T_Glyph *   Get_Encoding(void)
                            { return(m_encoding); }
    inline  bool        Is_Encoding_OK(void)
                            { return(m_is_encoding_ok); }

    inline  T_Glyph *   Get_Stylesheet_Href(void)
                            { return(m_style_href); }
    inline  T_Glyph *   Get_Stylesheet_Type(void)
                            { return(m_style_type); }

    C_XML_Contents *    Acquire_Contents(C_XML_Contents * parent);
    void                Release_Contents(C_XML_Contents * contents);

    void                Allocate_Output(void);

    long                Set_Encoding(const T_Glyph * encoding);

    void                Set_Stylesheet_Href(T_Glyph * href);
    void                Set_Stylesheet_Type(T_Glyph * type);

private:

    static T_Glyph      s_default_encoding[MAX_CHARACTER_ENCODING_LENGTH+1];

    bool                m_is_dynamic;   //whether to allocate contents with "new"
    long                m_max_attributes;//max # of attributes in hierarchy
    C_Napkin *          m_output;   // stream to synthesize output
    C_Pool *            m_storage;  // pool for all initial memory allocations
    C_Pool *            m_children; // pool for all child contents
    bool                m_is_detailed;
    bool                m_is_blanks;
    bool                m_is_encoding_ok;
    T_Glyph *           m_encoding;
    T_Glyph *           m_style_href;
    T_Glyph *           m_style_type;
};


/*  declare the class for parsing the contents of an XML document into a
    recursive contents hierarchy
*/
class   C_XML_Parser
{
public:
    C_XML_Parser(void);
    ~C_XML_Parser();

    static  inline  void    Set_Error_Buffer(T_Glyph * buffer)
                                { s_errors = buffer; }

    static  inline  long    Get_Line(void)
                                { return(s_line); }

    inline  C_XML_Contents* Contents(void)
                                { return(m_contents); }

    inline  bool            Is_Debug(void)
                                { return(m_is_debug); }

    inline  void            Descend(void)
                                { m_depth++; }
    inline  void            Ascend(void)
                                { m_depth--; }
    inline  long            Get_Depth(void)
                                { return(m_depth); }

    inline  void            Reset_Token(void)
                                { m_token_pos = 0; }

    const T_Glyph *     Get_Token(void);

    C_XML_Root *        Parse_Document(C_XML_Element * document,const C_String& text,
                                        bool is_dynamic = false,bool is_debug = false);
    C_XML_Root *        Parse_Element(C_XML_Element * element,const C_String& text,
                                        bool is_dynamic = false,bool is_debug = false);
#ifdef  _DEBUG
    void                Parse_Debug(const C_String& text);
#endif

    bool                Has_Text(void) const;
    bool                Is_End_Of_Text(void) const;
    bool                Is_Whitespace(const T_Glyph glyph) const;
    bool                Is_Delimiter(const T_Glyph glyph) const;
    bool                Is_Assignment_Delim(void);
    bool                Is_String_Delim(const T_Glyph glyph) const;
    bool                Is_Tag_Open(void) const;
    bool                Is_Tag_Close(void) const;
    bool                Is_Tag_Terminal(void) const;

    T_Glyph *           Get_Raw_Text(void);
    void                Clear_Raw_Text(void);

    T_Glyph             Next_Glyph(void);
    T_Glyph             Peek_Glyph(void);
    void                Unget_Glyph(const T_Glyph glyph);
    bool                Next_Token(void);
    void                Unget_Token(void);

    long                Compare_Glyphs(const T_Glyph * compare_to);

    void                Parse_PCData(bool is_preserve);
    void                Parse_Assignment(void);
    void                Parse_Comments(void);

    void                Parse_String(void);
    void                Parse_Rest_Of_Word(void);

private:
    inline  void        Set_Error(T_Glyph * msg)
                            { if (*s_errors == '\0') strcpy(s_errors,msg); }

    void                Parse_Version(void);
    void                Parse_Stylesheet(void);

    void                Grow_Raw_Text(void);
    void                Grow_Token(void);

    static  T_Glyph *       s_errors;
    static  long            s_line;

    C_String            m_text;
    T_Glyph *           m_raw_text;
    unsigned long       m_raw_pos;
    unsigned long       m_raw_size;
    T_Glyph *           m_token;
    unsigned long       m_token_pos;
    unsigned long       m_token_size;
    unsigned long       m_pos;
    bool                m_has_text;
    bool                m_is_in_tag;
    bool                m_is_preserve;
    bool                m_is_debug;
    long                m_depth;
    C_XML_Root *        m_contents;
};


/*  declare the class that describes an element in an XML DTD
*/
class   C_XML_Element
{
public:
    C_XML_Element(const T_Glyph * name,bool is_any,E_XML_PCDATA pcdata_type,
                    T_Fn_XML_Validate valid_func,
                    long child_count,T_XML_Child * children,
                    long attrib_count,T_XML_Attribute * attribs);
    C_XML_Element(T_XML_Element * info);
    ~C_XML_Element();

    static  inline  void    Set_Error_Buffer(T_Glyph * buffer)
                                { s_errors = buffer; }

    inline  T_Glyph_CPtr    Get_Name(void) const
                                { return(x_String(m_name)); }
    inline  long            Get_Child_Count(void)
                                { return(m_child_count); }
    inline  T_XML_Child *   Get_Child(long index) const
                                { return(&m_children[index]); }
    inline  C_XML_Element * Get_Child_Element(long index) const
                                { return(m_children[index].element); }
    inline  T_Glyph_CPtr    Get_Child_Name(long index) const
                                { return(x_String(m_children[index].element->m_name)); }
    inline  bool            Is_Any(void)
                                { return(m_is_any);}
    inline  bool            Is_Omit(void)
                                { return(m_is_omit); }
    inline  E_XML_PCDATA    Get_PCDATA_Type(void)
                                { return(m_pcdata_type); }
    inline  long            Get_Attribute_Count(void)
                                { return(m_attrib_count); }
    inline  T_XML_Attribute *
                            Get_Attribute(long index) const
                                { return(&m_attrib_list[index]); }

    void                Parse(C_XML_Parser * parser,C_XML_Contents * contents);
    void                Parse_Children(C_XML_Parser * parser,
                                        C_XML_Contents * contents);

    long                Get_Maximum_Attributes(long depth = 0);

protected:
    C_String            m_name;
    bool                m_is_any;
    E_XML_PCDATA        m_pcdata_type;
    T_Fn_XML_Validate   m_valid_func;
    long                m_child_count;
    T_XML_Child *       m_children;
    long                m_attrib_count;
    T_XML_Attribute *   m_attrib_list;
    bool                m_is_omit;

private:
    C_XML_Element(T_XML_Element * info,bool is_two_stage);

    inline  void        Set_Error(T_Glyph * msg)
                            { if (*s_errors == '\0') strcpy(s_errors,msg); }

    long        Construct(const T_Glyph * name,bool is_any,
                            E_XML_PCDATA pcdata_type,T_Fn_XML_Validate valid_func,
                            long child_count,T_XML_Child * children,
                            long attrib_count,T_XML_Attribute * attribs,
                            bool is_two_stage,bool is_omit);
    long        Initialize_Children(T_XML_Child * children);
    void        Parse_Unknown_Children(C_XML_Parser * parser, T_Glyph_Ptr name);
    void        Parse_Unknown_Tag(C_XML_Parser * parser);

    static  T_Glyph *       s_errors;
};


/*  declare function prototypes
*/
T_Int32U        XML_Get_Character_Encoding_Count(void);
T_Glyph *       XML_Get_Character_Encoding_Name(T_Int32U index);
