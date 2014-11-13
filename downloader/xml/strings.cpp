/*  STRINGS.CPP

    Copyright (c) 2001-2009 by Lone Wolf Development. All rights reserved.

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

    Internal strings used by the module.
*/


#include    "private.h"


static  T_Internal_String   l_strings[] = {
            { STRING_OUT_OF_MEMORY,         "Out of memory" },
            { STRING_OUT_OF_RESOURCES,      "Out of resources" },
            { STRING_UNEXPECTED_ERROR,      "Unexpected internal error" },
            { STRING_UNSPECIFIED_ERROR,     "Unspecified error parsing XML file" },
            { STRING_FILE_READ_ERROR,       "Error reading file: %0" },
            { STRING_FILE_WRITE_ERROR,      "Error writing file: %0" },

            { STRING_TAG_CLOSURE,           "Unexpected closure of tag" },
            { STRING_ATTRIBUTE_REPEATED,    "Attribute '%0' specified multiple times within element '%1'" },
            { STRING_UNRECOGNIZED_ATTRIBUTE,"Unrecognized attribute '%0' in element '%1'" },
            { STRING_VALUE_NOT_IN_SET,      "Attribute '%0' value does not match the specified set of valid values" },
            { STRING_FIXED_INCORRECT,       "Fixed attribute '%0' does not match specified value" },
            { STRING_ATTRIBUTE_NONCOMPLIANT,"Attribute '%0' is not compliant with specified rules" },
            { STRING_REQUIRED_ATTRIBUTE,    "One or more required attribute(s) (%0) are not specified" },
            { STRING_TAG_MISSING,           "One or more required child tag(s) (%0) are missing" },
            { STRING_PCDATA_NONCOMPLIANT,   "Element '%0' PCDATA is not compliant with specified rules" },
            { STRING_EXPECTED_CLOSE_TAG,    "Expected close tag for element '%0'"    },
            { STRING_PCDATA_NOT_ALLOWED,    "Encountered PCDATA for element '%0' proscribed against PCDATA" },
            { STRING_UNKNOWN_TAG,           "Encountered unknown element tag '%0'"   },
            { STRING_ELEMENT_LIMIT_ONE,     "Child element '%0' appears more times than its limit (1) in '%1'" },
            { STRING_ELEMENT_EXACTLY_ONCE,  "Element tag '%0' does not appear exactly once in '%1'" },
            { STRING_ELEMENT_REQUIRED,      "Element tag '%0' must appear at least once in '%1'" },
            { STRING_UNEXPECTED_END,        "Unexpected end of data - ill-formed XML" },
            { STRING_CDATA_TERMINATION,     "CDATA block not properly terminated" },
            { STRING_SPECIAL_TOKEN,         "Unrecognized special token following '&'" },
            { STRING_ASSIGNMENT_SYNTAX,     "Invalid value syntax for attribute assignment" },
            { STRING_BAD_VERSION_TAG,       "Invalid version tag"   },
            { STRING_EXPECTED_START_TAG,    "Expected start of an element tag"  },
            { STRING_BAD_TOPLEVEL_TAG,      "Invalid top-level element tag" },
            { STRING_COMMENT_DOUBLE_DASH,   "The character sequence '--' may not be used inside a comment" },
            { STRING_COMMENT_TERMINATION,   "Comment block not properly terminated" },
            { STRING_BAD_CHARACTER_ENCODING,"Unrecognized character encoding specified" },
            { STRING_ELEMENT_TERMINATION,   "Expected closing '>' for element '%0'"  },
            { STRING_BAD_ATTRIBUTE_VALUE,   "Value specified for attribute '%0' is not valid" },
            { STRING_PREMATURE_END,         "Premature end of document encountered" },
            { STRING_ATTRIBUTE_YES_NO,      "Value for attribute '%0' must be 'yes' or 'no'" },
            { STRING_EXTRANEOUS_DATA,       "Extraneous data encountered past end of top-level element" },
            { STRING_BAD_STYLESHEET_TAG,    "Invalid stylesheet tag"   },
            { STRING_INVALID_XML_DOCUMENT,  "Document does not possess a valid structure" }
            };

T_Internal_String_Table g_xml_strings = {
                            MODULE_ID_XML, x_Array_Size(l_strings), l_strings, FALSE
                            };
