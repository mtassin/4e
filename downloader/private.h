/*  PRIVATE.H

    Copyright (c) 2008-2012 by Lone Wolf Development.  All rights reserved.

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

    Private definitions for D&D insider crawler.
*/


#ifdef _WIN32
#include "xml\tools.h"
#include "xml\pubxml.h"
#elif defined(_OSX)
#include "xml/tools.h"
#include "xml/pubxml.h"
#else
#error Unknown platform!
#endif

#include    <ctype.h>
#include    <vector>

using namespace std;


/*  define basic types used in certain places
*/
typedef char                T_Glyph;
typedef char                T_Int8S;
typedef unsigned char       T_Int8U;
typedef short               T_Int16S;
typedef unsigned short      T_Int16U;
typedef long                T_Int32S;
typedef unsigned long       T_Int32U;

typedef float               T_Single;
typedef double              T_Double;

typedef T_Glyph *           T_Glyph_Ptr;
typedef const T_Glyph *     T_Glyph_CPtr;
typedef T_Int8U *           T_Byte_Ptr;
typedef void *              T_Void_Ptr;

#define MAX_FILE_NAME           512
typedef T_Glyph             T_Filename[MAX_FILE_NAME + 1];

typedef T_Int32U            T_Status;


/*  define platform-specific types and constants
*/
#ifdef _WIN32
typedef __int64             T_Int64S;
typedef unsigned __int64    T_Int64U;

#define DIR                 "\\"

#elif defined(_OSX)
typedef long long           T_Int64S;
typedef unsigned long long  T_Int64U;

#define DIR                 "/"

#define     stricmp     strcasecmp
#define     strnicmp    strncasecmp

#else
#error Unknown platform!
#endif


/*  define useful macros
*/
#define SUCCESS                     0
#define LWD_ERROR                   0x1
#define LWD_ERROR_NOT_IMPLEMENTED   0x2
#define WARN_CORE_FILE_NOT_FOUND    0x3
#define x_Status_Return_Success()   return(SUCCESS)
#define x_Status_Return(x)          return(x)
#define x_Is_Success(x)             (x == SUCCESS)
#define x_Trace(x)                  (void)


/* Define a structure that holds basic details about anything in DDI
*/
struct T_Base_Info {
    T_XML_Node      node; // XML node
    T_Glyph_Ptr     name; // Acid Arrow
    T_Glyph_Ptr     id; // Unique id - filled in during output
    T_Glyph_Ptr     source; // Player's Handbook
    T_Glyph         url[100]; // partial url to the individual page
    T_Glyph_Ptr     flavor; // flavor text
    T_Glyph_Ptr     prerequisite; // pre-requisite text
    T_Glyph_Ptr     description; // Raw text
    T_Glyph_Ptr     filename; // Filename of xml document to put it in
    bool            is_duplicate; // if true, entry is not output
    bool            is_partial; // if true, we do not have details for this entry
    bool            is_teaser; // if true, we have teaser information for this entry
    T_Base_Info *   parent_info; // parent info, if this is added by a power or something
};


/* Define a structure that holds appropriate details about a deity
*/
struct T_Deity_Info : public T_Base_Info {
    T_Glyph_Ptr     alignment;
    T_Glyph_Ptr     gender;
    T_Glyph_Ptr     sphere;
    T_Glyph_Ptr     dominion;
    T_Glyph_Ptr     priests;
    T_Glyph_Ptr     adjective;
};


/* Define a structure that holds appropriate details about a skill
*/
struct T_Skill_Info : public T_Base_Info {
    T_Glyph_Ptr     attribute; // STRENGTH / DEXTERITY / etc.
    T_Glyph_Ptr     forclass; // each skill is listed once for each class
    bool            armorcheck;
};


/* Define a structure that holds appropriate details about a ritual
*/
struct T_Ritual_Info : public T_Base_Info {
    T_Glyph_Ptr     level;
    T_Glyph_Ptr     category;
    T_Glyph_Ptr     time;
    T_Glyph_Ptr     duration;
    T_Glyph_Ptr     componentcost;
    T_Glyph_Ptr     price;
    T_Glyph_Ptr     keyskill;
};


/* Define a structure that holds appropriate details about a class
*/
struct T_Class_Info : public T_Base_Info {
    T_Glyph_Ptr     type; // player / other stuff??
    T_Glyph_Ptr     role; // Leader. You channel arcane power into etc etc.
    T_Glyph_Ptr     powersource; // Arcane. The cryptic formulas of etc etc.
    T_Glyph_Ptr     keyabilities; // Intelligence, Constitution
    T_Glyph_Ptr     armorprofs; // Leather, hide, cloth.
    T_Glyph_Ptr     weaponprofs; // Simple melee, simple ranged, longbow.
    T_Glyph_Ptr     implement; // orbs, rods, staffs, wands
    T_Glyph_Ptr     defbonuses; // +1 Fortitude, +1 Will.
    T_Glyph_Ptr     hp_level1; // 12
    T_Glyph_Ptr     hp_perlevel; // 5
    T_Glyph_Ptr     surges; // 7
    T_Glyph_Ptr     skl_trained; // Religion. From the class skills list below, choose 3 more trained skills at 1st level.
    T_Glyph_Ptr     skl_class; // Arcana (Int), Diplomacy (Cha), Heal (Wis), History (Int), Insight (Wis), Religion (Int).
    T_Glyph_Ptr     features; // First Strike, Rogue Tactics, Rogue Weapon Talent, Sneak Attack.
    T_Glyph_Ptr     hybrid_talents; // Any talents for hybrid classes
    T_Glyph_Ptr     details; // Block of text to pull class feature details from
    T_Glyph_Ptr     original_name; // Original pre-essentials name
};


/* Define a structure that holds appropriate details about a race
*/
struct T_Race_Info : public T_Base_Info {
    T_Glyph_Ptr     height;
    T_Glyph_Ptr     weight;
    T_Glyph_Ptr     abilityscores;
    T_Glyph_Ptr     size;
    T_Glyph_Ptr     speed;
    T_Glyph_Ptr     vision;
    T_Glyph_Ptr     languages;
    T_Glyph_Ptr     skillbonuses;
    T_Int32U        power_count;
    T_Int32U        powers[20];
};


/* Define a structure that holds appropriate details about a paragon path
*/
struct T_Paragon_Info : public T_Base_Info {
    T_Glyph_Ptr     features; // big block of text including path features
};


/* Define a structure that holds appropriate details about an epic destiny
*/
struct T_Epic_Info : public T_Base_Info {
    T_Glyph_Ptr     features; // big block of text including destiny features
    T_Int32U        powers[50];
    T_Int32U        power_count;
};


/* Define a structure that holds appropriate details about a power
*/
#define POWER_ATWILL            "atwillpower"
#define POWER_ENCOUNTER         "encounterpower"
#define POWER_DAILY             "dailypower"
#define POWER_DAILY_SURGE       "dailysurge"
#define POWER_CONSUMABLE        "consumable"
struct T_Power_Info : public T_Base_Info {
    T_Glyph_Ptr     forclass; // Cleric
    T_Glyph_Ptr     level; // 22
    T_Glyph_Ptr     action; // Standard / Immediate Reaction / Minor / etc
    T_Glyph_Ptr     type; // Wizard Attack 22 / Fighter Utility 7 / etc
    T_Glyph_Ptr     use; // atwillpower / encounterpower / dailypower / dailysurge / consumable
    T_Glyph_Ptr     keywords[100];
    T_Int32U        keyword_count;
    T_Glyph_Ptr     range; // Melee weapon / Ranged 6 / Close Burst 5
    T_Glyph_Ptr     limit; // Channel Divinity / etc
    T_Glyph_Ptr     special; // e.g. use more than once
    T_Glyph_Ptr     requirement; // random text
    T_Glyph_Ptr     target; // blank / One creature / etc
    T_Glyph_Ptr     attack; // Intelligence vs. Fortitude / etc
    T_Glyph_Ptr     idnumber; // NULL or a number; used to handle multiple item powers
};


/* Define a structure that holds appropriate details about a feat
*/
struct T_Feat_Info : public T_Base_Info {
    T_Glyph_Ptr     tier;
    T_Glyph_Ptr     descriptor;
    T_Glyph_Ptr     multiclass;
    T_Int32U        powers[10];
    T_Int32U        power_count;
};


/* Define a structure that holds appropriate details about an item
*/
#define FILE_WEAPONS        "ddi_weapons.dat"
#define FILE_ARMOR          "ddi_armor.dat"
#define FILE_WEAPONS_MAGIC  "ddi_magic_weapons.dat"
#define FILE_ARMOR_MAGIC    "ddi_magic_armor.dat"
#define FILE_EQUIPMENT      "ddi_equipment.dat"
#define FILE_MOUNTS         "ddi_mounts.dat"

struct T_Item_Info : public T_Base_Info {
    T_Glyph_Ptr     itemcategory;
    T_Glyph_Ptr     subcategory;
    T_Glyph_Ptr     cost;
    T_Glyph_Ptr     weight;
    T_Glyph_Ptr     super_impl_type;
    T_Glyph_Ptr     rarity;

    /* Weapons only
    */
    T_Glyph_Ptr     damage;
    T_Glyph_Ptr     proficient;
    T_Glyph_Ptr     range;
    T_Glyph_Ptr     properties;
    T_Glyph_Ptr     groups;

    /* Armor only
    */
    T_Glyph_Ptr     ac;
    T_Glyph_Ptr     minenhance;
    T_Glyph_Ptr     armorcheck;
    T_Glyph_Ptr     speed;
    T_Glyph_Ptr     armortype;
    T_Glyph_Ptr     special;

    /* Common to all magic items
    */
    T_Glyph_Ptr     level;
    T_Glyph_Ptr     enhbonus;
    T_Glyph_Ptr     enhancement;
    T_Int32U        powers[10];
    T_Int32U        power_count;
    bool            is_artifact;

    /* Magic weapons only
    */
    T_Glyph_Ptr     weaponreq;
    T_Glyph_Ptr     critical;

    /* Magic armor only
    */
    T_Glyph_Ptr     armorreq;

    /* Other magic items only
    */
    T_Glyph_Ptr     implement;
};


/* Define a structure that holds appropriate details about a monster
*/
struct T_Monster_Info : public T_Base_Info {
//?
};


/* Define a structure that holds appropriate details about a background
*/
struct T_Background_Info : public T_Base_Info {
    T_Glyph_Ptr     type;
    T_Glyph_Ptr     campaign;
    T_Glyph_Ptr     skills;
    T_Glyph_Ptr     benefit;
};


/* Structure to hold a filename and an XML document / root
*/
struct T_XML_Container {
    T_Glyph         filename[25];
    T_XML_Document  document;
    T_XML_Node      root;
};

#define MAX_XML_CONTAINERS      20


/*  define constants and types associated with unique id management
*/
#define UNIQUE_ID_LENGTH    10

typedef T_Int64U            T_Unique;

typedef vector<T_Unique> T_List_Ids;


/*  define uniqueness options
*/
#define UNIQUENESS_NONE         0
#define UNIQUENESS_UNIQUE       1
#define UNIQUENESS_USER_ONCE    2


/*  define types associated with WWW access
*/
typedef struct T_WWW_ *             T_WWW;


/* Define the delay between retrieving web pages - we don't want to DDOS the
    web server
*/
#define PAGE_RETRIEVE_DELAY 750             // ms


/* General purpose structure used to hold a variety of things that map from one
    string to another
*/
struct T_Tuple {
    T_Glyph_Ptr     a;
    T_Glyph_Ptr     b;
    T_Glyph_Ptr     c;
};


/* Class that holds multiple tuples and a unique id to identify them
*/
struct T_Mapping {
    T_Unique        id;
    vector<T_Tuple> list;
};


/* Vector that holds a list of URLs that weren't retrieved properly - declared
    here, defined in ddicrawler.cpp
*/
struct T_Failed {
    T_Base_Info *   info;
    T_Glyph         url[500];
    T_Filename      filename;
};
extern vector<T_Failed>   l_failed_downloads;


/*  define the set of file attributes
 */
typedef enum {
    e_filetype_normal = 0,
    e_filetype_directory = 0x0001,
    e_filetype_read_only = 0x0002,
    e_filetype_system = 0x0004,
    e_filetype_hidden = 0x0100
}   T_File_Attribute;


/*  define typedefs for various function pointers used by platform-specific
    functions
*/
typedef T_Status        (* T_Fn_File_Enum)(T_Glyph_Ptr filename,
                                           T_File_Attribute attributes,
                                           T_Void_Ptr context);


/* Define a class that lets us generalise output mechanisms for DDI stuff
*/
template<class T> class C_DDI_Output {
public:
    C_DDI_Output(T_Glyph_Ptr term, T_Glyph_Ptr first_filename, C_Pool * pool);
    virtual ~C_DDI_Output();

    inline T_Int32U         Get_Item_Count(void)
                                { return(m_list.size()); }

    T_Status        Process(void);
    T_Status        Post_Process(T_Filename temp_folder);

    T_Int32U        Append_Item(T * info);
    void            Append_Potential_Item(T * info);
    T *             Get_Item(T_Int32U index);

    /* Warning - finding an item by name does a linear search of the list. If
        you want to use it a lot, see if you can optimize it.
    */
    T *             Find_Item(T_Glyph_CPtr item_name);

protected:
    virtual T_Status    Output_Entry(T_XML_Node root, T * info) = 0;
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T * info) = 0;

    virtual bool        Is_Potential_Match(T * info, T * potential);

    C_Pool *        m_pool;

    T_Glyph_Ptr     m_term;

    vector<T>       m_list;
    vector<T>       m_potentials;

    T_XML_Document  m_output_document;
    T_List_Ids      m_id_list;

    T_XML_Container m_docs[MAX_XML_CONTAINERS];

private:

    T_Status        Output(void);
    void            Resolve_Potentials(void);
};


/* Define a 'common' class that's an ancestor of all C_DDI_Crawler classes. This
    lets us add them to a vector and perform certain common operations on them
    without a lot of boilerplate code.
*/
class C_DDI_Common {
public:
    C_DDI_Common(void)
                { }
    virtual ~C_DDI_Common()
                { }

    virtual T_Status    Download_Index(T_Filename folder, T_WWW internet) = 0;
    virtual T_Status    Read_Index(T_Filename folder) = 0;
    virtual T_Status    Download_Content(T_Filename folder, T_WWW internet) = 0;
    virtual T_Status    Read_Content(T_Filename folder) = 0;
    virtual T_Status    Process(void) = 0;
    virtual T_Status    Post_Process(T_Filename output_folder) = 0;
};


/*  Define a class that allows for the downloading of stuff from the Compendium
*/
template<class T> class C_DDI_Crawler : public C_DDI_Common, public C_DDI_Output<T> {
public:
    C_DDI_Crawler(T_Glyph_Ptr tab_name, T_Glyph_Ptr term, T_Glyph_Ptr post_param,
                    T_Int32U tab_index, T_Int32U last_cell_index, bool is_filter_dupes,
                    C_Pool * pool);
    virtual ~C_DDI_Crawler();

    static inline C_DDI_Crawler<T> *Get_Crawler(void)
                                        { return(s_crawler); }
    static inline T_XML_Document    Get_Output_Document(void)
                                        { return(s_crawler->m_output_document); }

    /* Define our processing functions that are required by C_DDI_Common,
        redirecting them to the C_DDI_Output base class
    */
    virtual inline T_Status Process(void)
                                { return(C_DDI_Output<T>::Process()); }
    virtual inline T_Status Post_Process(T_Filename output_folder)
                                { return(C_DDI_Output<T>::Post_Process(output_folder)); }

    T_Status    Download_Index(T_Filename folder, T_WWW internet);
    T_Status    Read_Index(T_Filename folder);

    T_Status    Download_Content(T_Filename folder, T_WWW internet);
    T_Status    Read_Content(T_Filename folder);

    /* Other classes may occasionally want to call this function to add powers
        or whatever themselves
    */
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T * info, T_Glyph_Ptr ptr,
                                            T_Glyph_Ptr checkpoint, vector<T> * extras) = 0;

protected:

    inline T_Glyph_Ptr      Get_Term(void)
                                { return(C_DDI_Output<T>::m_term); }
    inline C_Pool *         Get_Pool(void)
                                { return(C_DDI_Output<T>::m_pool); }
    inline vector<T> *      Get_List(void)
                                { return(&(C_DDI_Output<T>::m_list)); }
    inline T *              Get_List_Item(T_Int32U index)
                                { return(&(C_DDI_Output<T>::m_list[index])); }

    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T * info, T_Int32U subtype);

    virtual T_Int32U    Get_URL_Count(void);
    virtual void        Get_URL(T_Int32U index, T_Glyph_Ptr url, T_Glyph_Ptr names[],
                                T_Glyph_Ptr values[], T_Int32U * value_count);

    T_Status            Get_Required_Child_PCDATA(T_Glyph_Ptr * dest, T_XML_Node node,
                                                    T_Glyph_Ptr child_name);

    T_Glyph_Ptr     m_tab_name;
    T_Glyph_Ptr     m_tab_post_param;
    T_Int32U        m_tab_index;
    T_Int32U        m_last_cell_index;
    T_Int32U        m_max_failed_index_pages;
    T_Int32U        m_failed_index_pages;
    bool            m_is_filter_dupes;

private:

    /* NOTE! Since this is a template class, the s_crawler static variable
        effectively appears once for each specialization of the class, not just
        once.
    */
    static C_DDI_Crawler<T> *   s_crawler;

    T_Status        Parse_DDI_Index(T_XML_Node root, T_Int32U subtype);
    T_Status        Extract_Entry_From_Row(T_XML_Node node, T_Int32U subtype);
};


/*  Downloader for deities
*/
class C_DDI_Deities : public C_DDI_Crawler<T_Deity_Info> {
public:
    C_DDI_Deities(C_Pool * pool);
    ~C_DDI_Deities();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Deity_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Deity_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Deity_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Deity_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Deity_Info * info);
};


/*  Downloader for skills
*/
class C_DDI_Skills : public C_DDI_Crawler<T_Skill_Info> {
public:
    C_DDI_Skills(C_Pool * pool);
    ~C_DDI_Skills();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Skill_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Skill_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Skill_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Skill_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Skill_Info * info);
    virtual void        Get_URL(T_Int32U index, T_Glyph_Ptr url, T_Glyph_Ptr names[],
                                T_Glyph_Ptr values[], T_Int32U * value_count);
};


/*  Downloader for rituals - MUST COME AFTER SKILLS, because rituals rely on
    those to work out which skills they use
*/
class C_DDI_Rituals : public C_DDI_Crawler<T_Ritual_Info> {
public:
    C_DDI_Rituals(C_Pool * pool);
    ~C_DDI_Rituals();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Ritual_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Ritual_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Ritual_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Ritual_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Ritual_Info * info);
};


/*  Downloader for items - MUST COME BEFORE CLASSES (since classes need the
    weapon ids).
*/
class C_DDI_Items : public C_DDI_Crawler<T_Item_Info> {
public:
    C_DDI_Items(C_Pool * pool);
    ~C_DDI_Items();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Item_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Item_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Item_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Item_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Item_Info * info);

    T_Status        Output_Equipment(T_XML_Node root, T_Item_Info * info);
    T_Status        Output_Weapon(T_XML_Node root, T_Item_Info * info);
    T_Status        Output_Armor(T_XML_Node root, T_Item_Info * info);
    T_Status        Output_Magic_Weapon(T_XML_Node root, T_Item_Info * info);
    T_Status        Output_Magic_Armor(T_XML_Node root, T_Item_Info * info);
    T_Status        Output_Magic_Item(T_XML_Node root, T_Item_Info * info);

    T_Status        Post_Process_Armor(T_XML_Node root, T_Item_Info * info);
    T_Status        Post_Process_Magic_Weapon(T_XML_Node root, T_Item_Info * info);
    T_Status        Post_Process_Magic_Armor(T_XML_Node root, T_Item_Info * info);
};


/*  Downloader for classes - MUST COME AFTER SKILLS (it relies on them for
    working out class skills), RITUALS and WEAPONS.
*/
class C_DDI_Classes : public C_DDI_Crawler<T_Class_Info> {
public:
    C_DDI_Classes(C_Pool * pool);
    ~C_DDI_Classes();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Class_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Class_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Class_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Class_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Class_Info * info);
};


/*  Downloader for races - MUST COME AFTER SKILLS (it relies on them for
    working out skill bonuses) but BEFORE POWERS (since it needs to add powers
    to be output).
*/
class C_DDI_Races : public C_DDI_Crawler<T_Race_Info> {
public:
    C_DDI_Races(C_Pool * pool);
    ~C_DDI_Races();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Race_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Race_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Race_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Race_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Race_Info * info);
};


/*  Downloader for paragon paths - MUST COME AFTER CLASSES AND RACES (it relies
    on them for generating prereqs).
*/
class C_DDI_Paragons : public C_DDI_Crawler<T_Paragon_Info> {
public:
    C_DDI_Paragons(C_Pool * pool);
    ~C_DDI_Paragons();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Paragon_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Paragon_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Paragon_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Paragon_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Paragon_Info * info);
};


/*  Downloader for epic paths - MUST COME AFTER CLASSES AND RACES (it relies
    on them for generating prereqs).
*/
class C_DDI_Epics : public C_DDI_Crawler<T_Epic_Info> {
public:
    C_DDI_Epics(C_Pool * pool);
    ~C_DDI_Epics();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Epic_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Epic_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Epic_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Epic_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Epic_Info * info);
};


/*  Downloader for feats - MUST COME AFTER CLASSES AND RACES (it relies on them
    for working out pre-requisites) but BEFORE POWERS (since it needs to add
    powers to be output).
*/
class C_DDI_Feats : public C_DDI_Crawler<T_Feat_Info> {
public:
    C_DDI_Feats(C_Pool * pool);
    ~C_DDI_Feats();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Feat_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Feat_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Feat_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Feat_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Feat_Info * info);
};


/*  Downloader for powers - MUST COME AFTER CLASSES, RACES and FEATS
*/
class C_DDI_Powers : public C_DDI_Crawler<T_Power_Info> {
public:
    C_DDI_Powers(C_Pool * pool);
    ~C_DDI_Powers();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Power_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Power_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Power_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Power_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Power_Info * info);

    virtual T_Int32U    Get_URL_Count(void);
    virtual void        Get_URL(T_Int32U index, T_Glyph_Ptr url, T_Glyph_Ptr names[],
                                T_Glyph_Ptr values[], T_Int32U * value_count);
};


/*  Downloader for monsters - not outputting anything right now
*/
class C_DDI_Monsters : public C_DDI_Crawler<T_Monster_Info> {
public:
    C_DDI_Monsters(C_Pool * pool);
    ~C_DDI_Monsters();

private:
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Monster_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Monster_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Monster_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Monster_Info * info);
};


/* Define a 'common' class that's an ancestor of all C_DDI_Single classes. This
    lets us add them to a vector and perform certain common operations on them
    without a lot of boilerplate code.
*/
class C_DDI_Single_Common {
public:
    C_DDI_Single_Common(void)
                { }
    virtual ~C_DDI_Single_Common()
                { }

    virtual T_Status    Download(T_Filename folder, T_WWW internet) = 0;
    virtual T_Status    Read(T_Filename folder) = 0;
    virtual T_Status    Process(void) = 0;
    virtual T_Status    Post_Process(T_Filename output_folder) = 0;
};


/*  Define a class that's used for the downloading of stuff from a single page
*/
template<class T> class C_DDI_Single : public C_DDI_Single_Common, public C_DDI_Output<T> {
public:
    C_DDI_Single(T_Glyph_Ptr term, T_Glyph_Ptr first_filename, T_Glyph_Ptr url,
                    T_Glyph_Ptr filename, C_Pool * pool);
    ~C_DDI_Single();

    /* Define our processing functions that are required by C_DDI_Common,
        redirecting them to the C_DDI_Output base class
    */
    virtual inline T_Status Process(void)
                                { return(C_DDI_Output<T>::Process()); }
    virtual inline T_Status Post_Process(T_Filename output_folder)
                                { return(C_DDI_Output<T>::Post_Process(output_folder)); }

    T_Status    Download(T_Filename folder, T_WWW internet);
    T_Status    Read(T_Filename folder);

private:
    inline T_Glyph_Ptr      Get_Term(void)
                                { return(C_DDI_Output<T>::m_term); }

    virtual T_Status    Read_File(T_Glyph_Ptr ptr) = 0;

    T_Glyph_Ptr     m_url;
    T_Glyph_Ptr     m_filename;
};


/*  Downloader for backgrounds
*/
class C_DDI_Backgrounds : public C_DDI_Crawler<T_Background_Info> {
public:
    C_DDI_Backgrounds(C_Pool * pool);
    ~C_DDI_Backgrounds();

private:
    virtual T_Status    Parse_Index_Cell(T_XML_Node node, T_Background_Info * info, T_Int32U subtype);
    virtual T_Status    Parse_Entry_Details(T_Glyph_Ptr contents, T_Background_Info * info,
                                            T_Glyph_Ptr ptr, T_Glyph_Ptr checkpoint,
                                            vector<T_Background_Info> * extras);
    virtual T_Status    Output_Entry(T_XML_Node root, T_Background_Info * info);
    virtual T_Status    Post_Process_Entry(T_XML_Node root, T_Background_Info * info);
};


bool        Is_Password(void);

/* Get the root XML nodes for interesting data files
*/
T_XML_Node  Get_Language_Root(void);

/* Get the root XML nodes for interesting augmentation files
*/
T_XML_Node  Get_WepProp_Root(void);
T_XML_Node  Get_Source_Root(void);

/* Find a mapping entry in our master list
*/
T_Glyph_Ptr     Mapping_Find(T_Glyph_Ptr mapping, T_Glyph_Ptr search, bool is_partial_mapping = false, bool is_partial_search = false);
T_Tuple *       Mapping_Find_Tuple(T_Glyph_Ptr mapping, T_Glyph_Ptr search, bool is_partial_mapping = false, bool is_partial_search = false);
void            Mapping_Add(T_Glyph_Ptr mapping, T_Glyph_Ptr a, T_Glyph_Ptr b, T_Glyph_Ptr c = NULL);


/* Helper functions - found in helper.cpp
*/
void        Initialize_Helper(ofstream * stream);
void        Shutdown_Helper(void);
void        Log_Message(T_Glyph_Ptr message, bool is_console = false);

T_Status    Mem_Acquire(T_Int32U size, T_Void_Ptr * ptr);
T_Status    Mem_Resize(T_Void_Ptr current, T_Int32U requested, T_Void_Ptr * ptr);
void        Mem_Release(T_Void_Ptr ptr);

T_XML_Element * DTD_Get_Data(void);
T_XML_Element * DTD_Get_Augmentation(void);

bool        Check_Duplicate_Ids(T_Glyph_Ptr text, T_List_Ids * id_list);
bool        Generate_Thing_Id(T_Glyph_Ptr buffer, T_Glyph_Ptr init, T_Glyph_Ptr thing_name,
                                T_List_Ids * id_list, T_Int32U second_length = 3,
                                T_Glyph_Ptr suffix = NULL, bool is_try_again = true);
bool        Generate_Tag_Id(T_Glyph_Ptr buffer, T_Glyph_Ptr tag_name, T_XML_Node root);
T_Int32S    Create_Thing_Node(T_XML_Node parent, T_Glyph_Ptr term, T_Glyph_Ptr compset,
                                T_Glyph_Ptr name, T_Glyph_Ptr id, T_Glyph_Ptr description,
                                T_Int32U uniqueness, T_Glyph_Ptr source, T_XML_Node * node,
                                bool is_no_descript_ok);

void        Split_To_Array(T_Glyph_Ptr buffer, T_Glyph_Ptr chunks[], T_Int32U * chunk_count,
                            T_Glyph_Ptr text, T_Glyph_Ptr splitchars,
                            T_Glyph_Ptr ignore_after = NULL,
                            T_Glyph_Ptr * splitwords = NULL, T_Int32U split_word_count = 0);
void        Split_To_Tags(T_XML_Node node, T_Base_Info * info, T_Glyph_Ptr text,
                            T_Glyph_Ptr splitchars, T_Glyph_Ptr group,
                            T_Glyph_Ptr mapping1 = NULL, T_Glyph_Ptr mapping2 = NULL,
                            bool is_dynamic_name = false, T_Glyph_Ptr ignore_after = NULL,
                            T_Glyph_Ptr * splitwords = NULL, T_Int32U split_word_count = 0);
void        Split_To_Dynamic_Tags(T_XML_Node node, T_Base_Info * info, T_Glyph_Ptr text,
                            T_Glyph_Ptr splitchars, T_Glyph_Ptr group,
                            T_Glyph_Ptr mapping1 = NULL, T_Glyph_Ptr mapping2 = NULL,
                            bool is_dynamic_name = false, T_Glyph_Ptr ignore_after = NULL);

T_Int32S    Output_Source(T_XML_Node node, T_Glyph_Ptr source);
T_Int32S    Output_Tag(T_XML_Node node, T_Glyph_Ptr group, T_Glyph_Ptr tag,
                        T_Base_Info * info, T_Glyph_Ptr mapping1 = NULL, T_Glyph_Ptr mapping2 = NULL,
                        bool is_dynamic_name = false, bool is_dynamic_group = false);
T_Int32S    Output_Link(T_XML_Node node, T_Glyph_Ptr linkage, T_Glyph_Ptr thing,
                        T_Base_Info * info, T_Glyph_Ptr mapping = NULL);
T_Int32S    Output_Field(T_XML_Node node, T_Glyph_Ptr field, T_Glyph_Ptr value, T_Base_Info * info);
T_Int32S    Output_Exprreq(T_XML_Node node, T_Glyph_Ptr message, T_Glyph_Ptr expr, T_Base_Info * info);
T_Int32S    Output_Prereq(T_XML_Node node, T_Glyph_Ptr message, T_Glyph_Ptr script, T_Base_Info * info);
T_Int32S    Output_Bootstrap(T_XML_Node node, T_Glyph_CPtr thing, T_Base_Info * info,
                                T_Glyph_Ptr condition = NULL, T_Glyph_Ptr phase = NULL,
                                T_Glyph_Ptr priority = NULL, T_Glyph_Ptr before = NULL,
                                T_Glyph_Ptr after = NULL,
                                T_Glyph_Ptr assign_field = NULL, T_Glyph_Ptr assign_value = NULL,
                                T_Glyph_Ptr auto_group = NULL, T_Glyph_Ptr auto_tag = NULL,
                                T_Glyph_Ptr auto_group2 = NULL, T_Glyph_Ptr auto_tag2 = NULL);
T_Int32S    Output_Eval(T_XML_Node node, T_Glyph_CPtr script, T_Glyph_Ptr phase,
                                T_Glyph_Ptr priority, T_Base_Info * info, T_Glyph_Ptr before = NULL,
                                T_Glyph_Ptr after = NULL);
T_Int32S    Output_Eval_Rule(T_XML_Node node, T_Glyph_CPtr script, T_Glyph_Ptr phase,
                                T_Glyph_Ptr priority, T_Glyph_Ptr message, T_Base_Info * info,
                                T_Glyph_Ptr before = NULL, T_Glyph_Ptr after = NULL);

T_Int32S    Output_Language(T_XML_Node node, T_Glyph_Ptr name, T_Base_Info * info,
                            T_List_Ids * id_list, C_Pool * pool, T_Glyph_Ptr boot_group = NULL,
                                T_Glyph_Ptr boot_tag = NULL);

void        Output_Prereqs(T_Base_Info * info, T_List_Ids * id_list);

void        Append_Extensions(T_XML_Document document, T_Glyph_Ptr output_folder,
                                T_Glyph_Ptr filename, bool is_partial);

bool        Download_Page(T_WWW www, T_Base_Info * info, T_Glyph_Ptr url, T_Glyph_Ptr filename);

void        Attempt_Login_Again(T_WWW internet);


/* File-related functions in file*.cpp
*/
T_Glyph_Ptr Quick_Read_Text(T_Glyph_Ptr filename);
T_Status    Quick_Write_Text(T_Glyph_Ptr filename,T_Glyph_Ptr text);
bool        FileSys_Does_File_Exist(const T_Glyph_Ptr filename);
void        FileSys_List_Files(T_Glyph_Ptr wildcard);
void        FileSys_Delete_Files(T_Glyph_Ptr wildcard);
T_Status    FileSys_Copy_File(const T_Glyph_Ptr new_name, const T_Glyph_Ptr old_name, T_Boolean is_force);
void        FileSys_Get_Temporary_Folder(T_Glyph_Ptr buffer);
T_Status    FileSys_Enumerate_Matching_Files(T_Glyph_Ptr filespec,
                                             T_Fn_File_Enum enum_func,
                                             T_Void_Ptr context);
T_Status    FileSys_Create_Directory(T_Glyph_CPtr filename);
T_Status    FileSys_Delete_File(T_Glyph_CPtr filename,T_Boolean is_force);
bool        FileSys_Verify_Write_Privileges(T_Glyph_Ptr folder);
T_Glyph_Ptr FileSys_Get_Current_Directory(T_Glyph_Ptr buffer);
T_Boolean   FileSys_Is_Wildcard_Match(T_Glyph_Ptr filename,T_Glyph_Ptr wildspec);
T_Status    FileSys_Get_File_Attributes(T_Glyph_CPtr filename,
                                        T_File_Attribute * attribute);
bool        FileSys_Does_Folder_Exist(T_Glyph_CPtr filename);


/* Unique id functions, in uniqueid.cpp
*/
void            UniqueId_Initialize(void);
T_Unique        UniqueId_From_Text(const T_Glyph * text);
T_Glyph_Ptr     UniqueId_To_Text(T_Unique unique,const T_Glyph * text = NULL);
T_Boolean       UniqueId_Is_Valid(const T_Glyph * text);
T_Boolean       UniqueId_Is_Valid_Char(T_Glyph ch);


/* Encoding functions, in encode.cpp
*/
void            Text_Encode(T_Byte_Ptr data,T_Int32U in_len,T_Glyph_Ptr encode);
void            Text_Decode(T_Glyph_Ptr encode,T_Byte_Ptr data,T_Int32U * out_len);
T_Status        Text_Encode_To_File(T_Glyph_Ptr filename,T_Glyph_Ptr text);
T_Glyph_Ptr     Text_Decode_From_File(T_Glyph_Ptr filename);


/* Functions to grab stuff from the internet, in www*.cpp
*/
typedef struct T_WWW_ *             T_WWW;

T_Status        WWW_Retrieve_URL(T_WWW internet,T_Glyph_Ptr url,
                                    T_Glyph_Ptr * contents,T_Void_Ptr context);
T_Status        WWW_HTTP_Open(T_WWW * internet,T_Glyph_Ptr url,
                                    T_Glyph_Ptr proxy,T_Glyph_Ptr user,T_Glyph_Ptr password);
T_Status        WWW_HTTP_Post(T_WWW internet,T_Glyph_Ptr url,
                                    T_Int32U count,T_Glyph_Ptr * names,
                                    T_Glyph_Ptr * values,
                                    T_Glyph_Ptr * contents);
T_Status        WWW_Close_Server(T_WWW internet);


/* Text-processing functions - found in text*.cpp
*/
void        Strip_Bad_Characters(T_Glyph_Ptr buffer);
T_Status    Parse_Description_Text(T_Glyph_Ptr * final, T_Glyph_Ptr text, C_Pool * pool, bool is_close_p_newlines = false,
                                    T_Glyph_Ptr find_end = NULL);
T_Glyph_Ptr Extract_Text(T_Glyph_Ptr * final, C_Pool * pool, T_Glyph_Ptr text,
                            T_Glyph_Ptr checkpoint, T_Glyph_Ptr find, T_Glyph_Ptr find_end);
T_Glyph_Ptr Extract_Text(T_Glyph_Ptr * final, C_Pool * pool, T_Glyph_Ptr text,
                            T_Glyph_Ptr checkpoint, T_Glyph_Ptr find_sequence[],
                            T_Int32U find_count, T_Glyph_Ptr end_options[], T_Int32U end_count);
T_Glyph_Ptr Extract_Number(T_Glyph_Ptr * final, C_Pool * pool, T_Glyph_Ptr text,
                            T_Glyph_Ptr checkpoint, T_Glyph_Ptr find);
T_Glyph_Ptr Find_Text(T_Glyph_Ptr haystack, T_Glyph_Ptr needle, T_Glyph_Ptr checkpoint,
                        bool is_move_past, bool is_find_checkpoint_ok);
T_Int32S    stricmpright(T_Glyph_Ptr target, T_Glyph_Ptr compare);
void        Text_Trim_Trailing_Space(T_Glyph_Ptr src);
void        Text_Trim_Leading_Space(T_Glyph_Ptr src);
T_Int32U    Text_Find_Replace(T_Glyph_Ptr dest,T_Glyph_Ptr src,T_Glyph_Ptr find,T_Glyph_Ptr replace);
void        Text_Find_Replace(T_Glyph_Ptr dest,T_Glyph_Ptr src,T_Int32U length,
                                T_Int32U count,T_Glyph_Ptr * find,T_Glyph_Ptr * replace);

/* define some functions that are standard on windows but not OS X
*/
#ifdef _OSX
char* strlwr(char*);
char* strupr(char*);
#endif


/* Regex stuff - found in regex*.cpp
*/
bool        Is_Regex_Match(T_Glyph_Ptr regexp, T_Glyph_Ptr text,
                            T_Glyph_Ptr capture1 = NULL, T_Glyph_Ptr capture2 = NULL);
T_Void_Ptr  Regex_Create(T_Glyph_Ptr pattern);
string      Regex_Find_Match_Captures(T_Void_Ptr rx, T_Glyph_Ptr search_string, const vector<int> & captures);
string      Regex_Next_Match_Capture(T_Void_Ptr rx);
void        Regex_Destroy(T_Void_Ptr rx);


/* Misc functions
 */
void            Pause_Execution(T_Int32U milliseconds);
T_Glyph         Get_Character(void);
T_Power_Info *  Get_Power(T_Int32U index);
bool            Is_Log(void);
