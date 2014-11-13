/*  FILE:   XMLPARSE.CPP

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

    Implementation of basic parser for XML-based data streams.
*/


#include    "private.h"


/*  define constants used below
*/
#define TOKEN_START         1000
#define TOKEN_GROW          1000

#define RAW_START           10000
#define RAW_GROW            10000


/*  declare static member variables used below
*/
T_Glyph *       C_XML_Parser::s_errors = NULL;
long            C_XML_Parser::s_line = 0;


/*  declare other static variables
*/
T_Glyph *       l_encoding_types[] = { XML_ENCODING_ISO_8859_1 , XML_ENCODING_UTF_8 };


C_XML_Parser::C_XML_Parser(void)
{
    /* We should have one entry in our array per encoding type.
    */
    x_Trap_Opt(x_Array_Size(l_encoding_types) != NUM_ENCODING_TYPES);

    /* Initialize members
    */
    m_text = "";
    m_pos = 0;
    m_raw_text = (T_Glyph *) malloc(RAW_START);
    m_raw_size = RAW_START;
    m_raw_pos = 0;
    m_token = (T_Glyph *) malloc(TOKEN_START);
    m_token_size = TOKEN_START;
    m_token_pos = 0;
    m_has_text = false;
}


C_XML_Parser::~C_XML_Parser()
{
    if (m_raw_text != NULL)
        free(m_raw_text);
    if (m_token != NULL)
        free(m_token);
}


void    C_XML_Parser::Grow_Raw_Text(void)
{
    T_Glyph *   ptr;

    ptr = (T_Glyph *) realloc(m_raw_text,m_raw_size + RAW_GROW);
    if (ptr == NULL) {
        Set_Error(x_Internal_String(STRING_OUT_OF_MEMORY));
        x_Exception(-9999);
        }
    m_raw_text = ptr;
    m_raw_size += RAW_GROW;
}


void    C_XML_Parser::Grow_Token(void)
{
    T_Glyph *   ptr;

    ptr = (T_Glyph *) realloc(m_token,m_token_size + TOKEN_GROW);
    if (ptr == NULL) {
        Set_Error(x_Internal_String(STRING_OUT_OF_MEMORY));
        x_Exception(-9999);
        }
    m_token = ptr;
    m_token_size += TOKEN_GROW;
}


const T_Glyph * C_XML_Parser::Get_Token(void)
{
    if (m_token_pos >= m_token_size)
        Grow_Token();
    m_token[m_token_pos] = '\0';
    return(m_token);
}


bool    C_XML_Parser::Has_Text(void) const
{

    return(m_has_text);
}


bool    C_XML_Parser::Is_End_Of_Text(void) const
{
    bool is_eot = false;

    if (!m_has_text)
        is_eot = true;

    /* NOTE! The test below should really be >= and not just >, because
        if we're == to the size of the string we're past the last character in
        it. However, this XML code appears to assume that it's OK to fetch one
        character past the end of the string, which it assumes will be a '\0'.
        This is true for character arrays, but not for std::strings; however,
        I don't have time to fix this properly, so see comment in
        C_XML_Parser::Next_Glyph, below, for how this is being handled.
    */
    else if (m_pos > m_text.size())
        is_eot = true;
    return(is_eot);
}


bool    C_XML_Parser::Is_Whitespace(const T_Glyph glyph) const
{
    bool is_space = false;

    if ((glyph == ' ') || (glyph == '\n') || (glyph == '\r') || (glyph == '\t'))
        is_space = true;
    return(is_space);
}


bool    C_XML_Parser::Is_Delimiter(const T_Glyph glyph) const
{
    bool is_delim = false;

    if ((glyph == '<') || (glyph == '=') || //(glyph == '\"') ||
        (glyph == '/') || (glyph == '>'))
        is_delim = true;
    return(is_delim);
}


bool    C_XML_Parser::Is_Assignment_Delim(void)
{
    bool is_delim = false;

    if ((m_token_pos == 1) && (*m_token == '='))
        is_delim = true;
    return(is_delim);
}


bool    C_XML_Parser::Is_String_Delim(const T_Glyph glyph) const
{
    bool is_delim = false;

    if (glyph == '\"')
        is_delim = true;
    return(is_delim);
}


bool    C_XML_Parser::Is_Tag_Open(void) const
{
    bool is_open = false;

    if ((m_token_pos == 1) && (*m_token == '<'))
        is_open = true;
    return(is_open);
}


bool    C_XML_Parser::Is_Tag_Close(void) const
{
    bool is_close = false;

    if ((m_token_pos == 1) && (*m_token == '>'))
        is_close = true;
    return(is_close);
}


bool    C_XML_Parser::Is_Tag_Terminal(void) const
{
    bool is_term = false;

    if ((m_token_pos == 1) && (*m_token == '/'))
        is_term = true;
    return(is_term);
}


T_Glyph *   C_XML_Parser::Get_Raw_Text(void)
{
    return(m_raw_text);
}


void    C_XML_Parser::Clear_Raw_Text(void)
{

    m_raw_pos = 0;
}


T_Glyph C_XML_Parser::Next_Glyph(void)
{
    T_Glyph glyph;

    /* NOTE! Due to a bad test in Is_End_Of_Text, above, we might end up with
        m_pos being equal to one past the last character of the string here. In
        Visual Studio 2005, this is fine, because it appears that retrieving
        one character past the end of a std::string simply returns a '\0'.
        However, in 2010, it causes a debug assertion. I don't have time to
        handle all the tendrils of fixing this properly, so we do a specific
        test here to make sure we're not retrieving one past the end of the
        string to duplicate the VS2005 behavior. (Anything beyond that
        character should still cause whatever horrible thing to happen should
        normally happen.)
    */
    if (m_pos != m_text.size())
        glyph = m_text[m_pos];
    else
        glyph = '\0';
    m_pos++;

    if (glyph == '\n')
        s_line++;
    if (m_raw_pos >= m_raw_size)
        Grow_Raw_Text();
    m_raw_text[m_raw_pos++] = glyph;
    return(glyph);
}


T_Glyph C_XML_Parser::Peek_Glyph(void)
{
    return(m_text[m_pos]);
}


void    C_XML_Parser::Unget_Glyph(const T_Glyph glyph)
{
    if (glyph == '\n')
        s_line--;
    m_pos--;
    m_raw_pos--;
}


long    C_XML_Parser::Compare_Glyphs(const T_Glyph * compare_to)
{
    long        result;

    /* Compare characters in the compare string to our next glyphs.
    */
    result = strncmp(compare_to, &m_text[m_pos], strlen(compare_to));
    return(result);
}


/* extract all text up to the end of the string, designated by a '\"'
*/
void    C_XML_Parser::Parse_String(void)
{
    T_Glyph glyph;

    while (!Is_End_Of_Text()) {
        glyph = Next_Glyph();
        if (Is_String_Delim(glyph))
            break;
        if (m_token_pos >= m_token_size)
            Grow_Token();
        m_token[m_token_pos++] = glyph;
        }
    if (m_token_pos >= m_token_size)
        Grow_Token();
    m_token[m_token_pos] = '\0';
}


/* extract all text up to the next delimiter of any type
*/
void    C_XML_Parser::Parse_Rest_Of_Word(void)
{
    T_Glyph glyph;

    while (!Is_End_Of_Text()) {
        glyph = Next_Glyph();
        if (Is_Delimiter(glyph) || Is_Whitespace(glyph)) {
            Unget_Glyph(glyph);
            break;
            }
        if (m_token_pos >= m_token_size)
            Grow_Token();
        m_token[m_token_pos++] = glyph;
        }
    if (m_token_pos >= m_token_size)
        Grow_Token();
    m_token[m_token_pos] = '\0';
}


/* extract the next token from the text stream
*/
bool    C_XML_Parser::Next_Token(void)
{
    T_Int32U        i, length;
    T_Glyph         glyph;
    T_Glyph *       ptr;

    /* skip over all whitespace
    */
    m_token_pos = 0;
    while (!Is_End_Of_Text()) {
        glyph = Next_Glyph();
        if (!Is_Whitespace(glyph))
            break;
        }

    /* if we've run out of the input stream, we're out of tokens
    */
    if (Is_End_Of_Text())
        return(false);

    /* if this is the opening of a new tag or the close of a tag, note that fact
    */
    if (glyph == '<')
        m_is_in_tag = true;
    else if (glyph == '>')
        m_is_in_tag = false;

    /* if this is the start of a string, extract the full string contents; but
        we only care about strings within a tag (i.e. for attributes)
    */
    if (m_is_in_tag && Is_String_Delim(glyph))
        Parse_String();

    /* if this is a delimiter of any type, return the delimiter as our token
    */
    else if (Is_Delimiter(glyph)) {
        m_token[0] = glyph;
        m_token[1] = '\0';
        m_token_pos = 1;
        }

    /* otherwise, this is a one-word token, so extract it properly
    */
    else {

        /* If the token starts with a ! character, check to see if it's our
            CDATA or comments identifiers. If it is, just return that token.
        */
        if (glyph == '!') {

            /* Try and match our CDATA or comments start blocks. We've already
                matched the '!' at the start.
            */
            ptr = CDATA_START;
            if (Compare_Glyphs(ptr + 1) != 0) {
                ptr = COMMENTS_START;
                if (Compare_Glyphs(ptr + 1) != 0)
                    ptr = NULL;
                }

            /* If we had a match, extract the matching string into our token,
                and return.
            */
            if (ptr != NULL) {
                length = strlen(ptr);
                m_token[0] = glyph;
                for (i = 1; i < length; i++)
                    m_token[i] = Next_Glyph();
                m_token[length] = '\0';
                m_token_pos = strlen(m_token);
                return(true);
                }
            }
        m_token[0] = glyph;
        m_token_pos = 1;
        Parse_Rest_Of_Word();
        }
    return(true);
}


void    C_XML_Parser::Unget_Token(void)
{
    m_pos -= m_token_pos;
    m_raw_pos -= m_token_pos;
}


// extract all data up to the start of the next XML tag, but do NOT consume the
// token that indicates the tag start
void    C_XML_Parser::Parse_PCData(bool is_preserve)
{
    T_Int32U        i,length,value;
    T_Glyph         glyph,mapping;
    T_Glyph *       ptr;
    bool            is_whitespace = false,is_comments,is_cdata;

    while (!Is_End_Of_Text()) {
        glyph = Next_Glyph();
        switch (glyph) {

            /* if we got whitespace, our response depends on whether we're
                preserving it or not
                NOTE! We have to handle /r here, as that character is present
                in windows files that are read in on mac os.
            */
            case ' ' :
            case '\n' :
            case '\r' :
            case '\t' :

                /* if we're preserving whitespace, just append the glyph
                */
                if (is_preserve) {
                    if (m_token_pos >= m_token_size)
                        Grow_Token();
                    m_token[m_token_pos++] = glyph;
                    continue;
                    }

                /* otherwise, we have to map the whitespace to a simple space
                    character; if we already have whitespace, we ignore it
                */
                if (!is_whitespace) {
                    if (m_token_pos >= m_token_size)
                        Grow_Token();
                    m_token[m_token_pos++] = ' ';
                    is_whitespace = true;
                    }
                break;

            /* if we got a '<', we have encountered a new tag; the only tags we
                recognize within PCDATA are the CDATA literal block and XML
                comments, so verify that's what we've got, else bail out and
                unget the glyph
            */
            case '<' :

                /* clear the whitespace flag
                */
                is_whitespace = false;

                /* look at the next glyph; if it's not the next character of a
                    CDATA literal or comments block, unget our '<' glyph and
                    bail out - PCDATA is done
                */
                glyph = Peek_Glyph();
                if (glyph != '!') {
                    Unget_Glyph('<');
                    return;
                    }

                /* Work out if this is a CDATA or comments block
                */
                is_comments = false;
                is_cdata = false;
                ptr = COMMENTS_START;
                if (Compare_Glyphs(ptr) == 0)
                    is_comments = true;
                else {
                    ptr = CDATA_START;
                    if (Compare_Glyphs(ptr) == 0)
                        is_cdata = true;

                    /* Otherwise, we're done with PCDATA; unget the '<' glyph,
                        and return.
                    */
                    else {
                        Unget_Glyph('<');
                        return;
                        }
                    }

                /* Get rid of any of the glyphs we just compared against; we
                    know what they are, so we don't need them any more.
                */
                length = strlen(ptr);
                for (i = 0; i < length; i++)
                    Next_Glyph();

                /* If this was a comments block, parse it out and we're done
                    for this iteration.
                */
                if (is_comments) {
                    Parse_Comments();
                    break;
                    }
                x_Assert(is_cdata);

                /* If this is a CDATA literal block, just keep grabbing text
                    until we reach the end of the block ("]]>")
                */
                while (1) {

                    /* get the next glyph; if not a ']', add it and keep going;
                        if we hit the end of the text stream, then bail out
                        (this is a safety test in case the CDATA block is not
                        properly terminated)
                    */
                    glyph = Next_Glyph();
                    if (glyph != ']') {
                        if (m_token_pos >= m_token_size)
                            Grow_Token();
                        m_token[m_token_pos++] = glyph;
                        if (x_Trap_Opt(Is_End_Of_Text())) {
                            Set_Error(x_Internal_String(STRING_CDATA_TERMINATION));
                            x_Exception(-140);
                            }
                        continue;
                        }

                    /* we got a ']', but we need to save it in case we haven't
                        reached the end of the block
                    */
                    if (m_token_pos >= m_token_size)
                        Grow_Token();
                    m_token[m_token_pos++] = glyph;

                    /* if either of the next two glyphs don't complete the end
                        of the block, we need to unget the glyph and continue
                        parsing; this ensures that we resume parsing after only
                        the first glyph is consumed
                    */
                    glyph = Next_Glyph();
                    if ((glyph != ']') || ('>' != Peek_Glyph())) {
                        Unget_Glyph(glyph);
                        continue;
                        }

                    /* when we get here, we've got the pending ']'character
                        accumulated into the token, so we must undo it; we must
                        also consume the pending '>' glyph; after that, we're
                        done
                    */
                    m_token_pos--;
                    Next_Glyph();
                    break;
                    }
                break;

            /* if this is an '&', we need to translate the token to the
                corresponding single character
            */
            case '&' :

                /* clear the whitespace flag
                */
                is_whitespace = false;

                /* translate the token to the proper character
                */
                glyph = Next_Glyph();
                switch (glyph) {
                    case 'l' :  if (x_Trap_Opt('t' != Next_Glyph())) {
                                    Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                                    x_Exception(-130);
                                    }
                                mapping = '<';
                                break;
                    case 'g' :  if (x_Trap_Opt('t' != Next_Glyph())) {
                                    Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                                    x_Exception(-130);
                                    }
                                mapping = '>';
                                break;
                    case 'q' :  if (x_Trap_Opt(('u' != Next_Glyph()) ||
                                               ('o' != Next_Glyph()) ||
                                               ('t' != Next_Glyph()))) {
                                    Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                                    x_Exception(-130);
                                    }
                                mapping = '\"';
                                break;
                    case 'a' :  glyph = Next_Glyph();
                                if (('m' == glyph) &&
                                    ('p' == Next_Glyph()))
                                    mapping = '&';
                                else if (('p' == glyph) &&
                                         ('o' == Next_Glyph()) &&
                                         ('s' == Next_Glyph()))
                                    mapping = '\'';
                                else {
                                    x_Break_Opt();
                                    Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                                    x_Exception(-130);
                                    }
                                break;
                    case '#' :  glyph = Next_Glyph();
                                if (glyph != 'x')
                                    for (value = glyph - '0'; x_Is_Digit(Peek_Glyph()); )
                                        value = (value * 10) + (Next_Glyph() - '0');
                                else {
                                    for (value = 0; true; ) {
                                        glyph = Next_Glyph();
                                        if ((glyph >= '0') && (glyph <= '0'))
                                            glyph -= '0';
                                        else if ((glyph >= 'a') && (glyph <= 'f'))
                                            glyph -= 'a' - 10;
                                        else if ((glyph >= 'A') && (glyph <= 'F'))
                                            glyph -= 'A' - 10;
                                        else
                                            break;
                                        value = (value * 16) + glyph;
                                        }
                                    Unget_Glyph(glyph);
                                    }
                                if (x_Trap_Opt((value == 0) || (value >= 255))) {
                                    Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                                    x_Exception(-130);
                                    }
                                mapping = (T_Glyph) value;
                                break;
                    default :   x_Break_Opt();
                                Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                                x_Exception(-130);
                                break;
                    }
                if (x_Trap_Opt(';' != Next_Glyph())) {
                    Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                    x_Exception(-130);
                    }
                if (m_token_pos >= m_token_size)
                    Grow_Token();
                m_token[m_token_pos++] = mapping;
                break;

            /* anything else is just a normal character, so add it to our text
            */
            default :
                is_whitespace = false;
                if (m_token_pos >= m_token_size)
                    Grow_Token();
                m_token[m_token_pos++] = glyph;
                break;
            }
        }
}


// extract the assigned value for an attribute; convert special characters
void    C_XML_Parser::Parse_Assignment(void)
{
    const T_Glyph * src;
    long            result;
    T_Glyph         buffer[MAX_XML_BUFFER_SIZE+1];

    if (Next_Token() && Is_Assignment_Delim())
        if (Is_String_Delim(Peek_Glyph()) && Next_Token()) {
            src = Get_Token();
            result = XML_Extract_Attribute_Value(buffer, src);
            if (result != 0) {
                x_Break_Opt();
                Set_Error(x_Internal_String(STRING_SPECIAL_TOKEN));
                x_Exception(-100);
                }
            strcpy(m_token,buffer);
            m_token_pos = strlen(m_token);
            return;
            }

Debug_Printf("Parse_Assignment: Failed\n");
    x_Break_Opt();
    Set_Error(x_Internal_String(STRING_ASSIGNMENT_SYNTAX));
    x_Exception(-100);
}


// pass over all text until the end of the comments block
// NOTE: According to the XML rules, nested comments aren't allowed, and you
//       can't have -- inside a comment except to <!-- open and close --> it.
void    C_XML_Parser::Parse_Comments(void)
{
    T_Glyph         glyph;

    while (1) {

        /* Get the next glyph. Check we're not at the end of our text - if we
            are, the current glyph isn't going to be any help to us, so we
            might as well just throw an exception.
        */
        glyph = Next_Glyph();
        if (x_Trap_Opt(Is_End_Of_Text()))
            goto end_of_text;

        /* If the current glyph isn't a '-' character, just continue.
        */
        if (glyph != '-')
            continue;

        /* Check if the next character is a - as well. If not, we don't need to
            look at it, as it's not going to be relevant to our comment, so
            just throw it away and continue.
        */
        glyph = Next_Glyph();
        if (x_Trap_Opt(Is_End_Of_Text()))
            goto end_of_text;
        if (glyph != '-')
            continue;

        /* Since we just found a "--", the next character should be '>' to close
            the comment (these are the rules of XML, no "--" sequences inside
            comments except to open and close them). If the next character
            isn't '>', complain to the user.
        */
        glyph = Next_Glyph();
        if (x_Trap_Opt(Is_End_Of_Text()))
            goto end_of_text;
        if (glyph != '>') {
            Set_Error(x_Internal_String(STRING_COMMENT_DOUBLE_DASH));
            x_Exception(-150);
            }

        /* The comment has finished, so just return - we've parsed it out.
        */
        return;
        }
    x_Trap_Opt(1);
    return;

end_of_text:
    Set_Error(x_Internal_String(STRING_COMMENT_TERMINATION));
    x_Exception(-140);
}


/* parse an XML version tag and throw it away, allowing for optional attributes
    for version and encoding
*/
void    C_XML_Parser::Parse_Version(void)
{
    /* Initialize our encoding to the default value
    */
    m_contents->Set_Encoding(C_XML_Root::Get_Default_Encoding());

    while (!Is_End_Of_Text()) {
        if (Next_Token()) {
            if (strcmp(Get_Token(),"version") == 0)
                Parse_Assignment();
            else if (strcmp(Get_Token(),"encoding") == 0) {
                Parse_Assignment();
                m_contents->Set_Encoding(m_token);
                }
            else if ((m_token_pos == 1) && (*m_token == '?'))
                if (Next_Token() && Is_Tag_Close())
                    return;
            else
                break;
            }
        }
Debug_Printf("Parse_Version: Failed\n");
    x_Break_Opt();
    Set_Error(x_Internal_String(STRING_BAD_VERSION_TAG));
    x_Exception(-101);
}


/* parse an XML stylesheet tag and preserve the values in our root so that the
    stylesheet can be output again if we emit the document
*/
void    C_XML_Parser::Parse_Stylesheet(void)
{
    while (!Is_End_Of_Text()) {
        if (Next_Token()) {
            if (strcmp(Get_Token(),"href") == 0) {
                Parse_Assignment();
                m_contents->Set_Stylesheet_Href(m_token);
                }
            else if (strcmp(Get_Token(),"type") == 0) {
                Parse_Assignment();
                m_contents->Set_Stylesheet_Type(m_token);
                }
            else if ((m_token_pos == 1) && (*m_token == '?'))
                if (Next_Token() && Is_Tag_Close())
                    return;
            else
                break;
            }
        }
Debug_Printf("Parse_Stylesheet: Failed\n");
    x_Break_Opt();
    Set_Error(x_Internal_String(STRING_BAD_STYLESHEET_TAG));
    x_Exception(-101);
}


C_XML_Root *    C_XML_Parser::Parse_Document(C_XML_Element * document,
                                    const C_String& text,bool is_dynamic,bool is_debug)
{
    bool            is_token,is_document;
    C_XML_Root*     root;
    const T_Glyph * token;

if (is_debug) Debug_Printf("\nParsing Document:\n");
    /* reset our error message
    */
    x_Assert_Opt(s_errors != NULL);
    *s_errors = '\0';
    s_line = 1;

    /* initialize everything for the parse
    */
    m_text = text;
    m_pos = 0;
    m_token_pos = 0;
    m_raw_pos = 0;
    m_has_text = true;
    m_is_in_tag = false;
    m_is_preserve = false;
    m_is_debug = is_debug;
    m_depth = -1;

    /* create a contents object to hold the parsed results
    */
    root = new C_XML_Root(document,is_dynamic);
    m_contents = root;

    /* loop until we run out of data
    */
    try {
        is_document = false;
        while (1) {

            /* grab the next token; if we've run out of input, bail out
            */
            Next_Token();
            if (Is_End_Of_Text())
                break;

            /* if this is not the start of tag, something is very wrong
            */
            if (x_Trap_Opt(!Is_Tag_Open())) {
                Set_Error(x_Internal_String(STRING_EXPECTED_START_TAG));
                x_Exception(-102);
                }

            /* grab the next token, which will be the tag to process; if no
                token is found, it's an unexpected end of input
            */
            is_token = Next_Token();
            if (x_Trap_Opt(!is_token)) {
                Set_Error(x_Internal_String(STRING_PREMATURE_END));
                x_Exception(-122);
                }

            /* If this is a comment, parse it out and continue.

                NOTE: We have to do this BEFORE checking if we're past the end
                        of the document, because comments are allowed after the
                        root element has closed.
            */
            token = Get_Token();
            if (strcmp(token, COMMENTS_START) == 0) {
                Parse_Comments();
                continue;
                }


            /* if this is data past the end of the document, it's an error
            */
            if (x_Trap_Opt(is_document)) {
                Set_Error(x_Internal_String(STRING_EXTRANEOUS_DATA));
                x_Exception(-123);
                }

            /* if the tag is an XML version tag, consume it properly and discard it
            */
            if (strcmp(token,"?xml") == 0)
                Parse_Version();

            /* if the tag is an XML stylesheet tag, parse and preserve it
            */
            else if (strcmp(token,"?xml-stylesheet") == 0)
                Parse_Stylesheet();

            /* if the tag is our top-level document tag, begin recursive descent
            */
            else if (strcmp(token,document->Get_Name()) == 0) {
                document->Parse(this,m_contents);
                is_document = true;
                }

            /* anything else is a nasty error
            */
            else {
                x_Break_Opt();
                Set_Error(x_Internal_String(STRING_BAD_TOPLEVEL_TAG));
                x_Exception(-103);
                }
            }
        }
    catch (C_Exception except) {
        delete root;
        m_contents = NULL;
        x_Exception(except.Get_Error());
        }

    /* if we don't have a valid document yet, it's an error
    */
    if (x_Trap_Opt(!is_document)) {
        Set_Error(x_Internal_String(STRING_INVALID_XML_DOCUMENT));
        x_Exception(-123);
        }

    /* we've consumed all the text - return the contents
    */
    m_has_text = false;
    return(root);
}


C_XML_Root *    C_XML_Parser::Parse_Element(C_XML_Element * document,
                                    const C_String& text,bool is_dynamic,bool is_debug)
{
    C_XML_Root *    root;

    /* reset our error message
    */
    x_Assert_Opt(s_errors != NULL);
    *s_errors = '\0';
    s_line = 1;

    /* initialize everything for the parse
    */
    m_text = text;
    m_pos = 0;
    m_token_pos = 0;
    m_raw_pos = 0;
    m_has_text = true;
    m_is_in_tag = false;
    m_is_preserve = false;
    m_is_debug = is_debug;
    m_depth = -1;

    /* create a contents object to hold the parsed results
    */
    root = new C_XML_Root(document,is_dynamic);
    m_contents = root;

    /* loop until we run out of data
    */
    try {
        while (1) {

            /* grab the next token; if we've run out of input, bail out
            */
            Next_Token();
            if (Is_End_Of_Text())
                break;

            /* if this is not the start of tag, something is very wrong
            */
            if (x_Trap_Opt(!Is_Tag_Open())) {
                Set_Error(x_Internal_String(STRING_EXPECTED_START_TAG));
                x_Exception(-102);
                }

            /* grab the next token, which will be the tag to process
            */
            Next_Token();

            /* if the tag is our top-level document tag, begin recursive descent
            */
            if (strcmp(Get_Token(),document->Get_Name()) == 0)
                document->Parse(this,m_contents);

            /* anything else is a nasty error
            */
            else {
                x_Break_Opt();
                Set_Error(x_Internal_String(STRING_BAD_TOPLEVEL_TAG));
                x_Exception(-103);
                }
            }
        }
    catch (C_Exception except) {
        delete root;
        m_contents = NULL;
        x_Exception(except.Get_Error());
        }

    /* we've consumed all the text - return the contents
    */
    m_has_text = false;
    return(root);
}


T_Int32U        XML_Get_Character_Encoding_Count(void)
{
    return(x_Array_Size(l_encoding_types));
}


T_Glyph *       XML_Get_Character_Encoding_Name(T_Int32U index)
{
    x_Assert(index < x_Array_Size(l_encoding_types));
    return(l_encoding_types[index]);
}


#ifdef  _DEBUG
// iterate through all tokens in the document and output them - for testing only
void    C_XML_Parser::Parse_Debug(const C_String& text)
{
    m_text = text;
    m_pos = 0;
    m_token_pos = 0;
    m_raw_pos = 0;
    m_has_text = true;
    m_is_in_tag = false;

    while (!Is_End_Of_Text()) {
        Next_Token();
        std::cout << Get_Token() << " ";
Debug_Printf("%s ",Get_Token());
        if (Is_Tag_Close()) {
            std::cout << std::endl;
Debug_Printf("\n");
            }
        }

    m_has_text = false;
}
#endif
