/**
 * \defgroup local_ui Local UI Library
 * 
 * \brief Library providing a folder-based user interface through linked lists.
 * 
 * The key type used in this library is the ::local_ui_folder_tree which contains one or more
 * ::local_ui_folder structs. If you imagine a linked list of ::local_ui_folder structs with each having 
 * zero or more subfolders, the ::local_ui_folder_tree tracks the root of the tree, and the
 * current folder. A ::local_ui_folder_tree is built using the ::local_ui_add_subfolder function
 * which connects a parent folder to a new child.
 * 
 * Each ::local_ui_folder can have one or more subfolders and/or a ::folder_action callback. If a
 * ::folder_action is assigned, then calling ::local_ui_enter_subfolder when the folder is active
 * calls the callback. Else, the indicated subfolder is entered. Note that any subfolders are
 * ignored if the ::folder_action is set. 
 * 
 * 
 * \ingroup machine_logic
 * \{
 * 
 * \file local_ui.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Local UI header
 * \version 0.1
 * \date 2022-11-15
*/
#ifndef LOCAL_UI_H
#define LOCAL_UI_H
#include "pico/stdlib.h" // typedefs

/** \brief The maximum number of subfolders that any folder can have. */
#define LOCAL_UI_MAX_SUBFOLDER_NUM 3
/** \brief The maximum number of chars in a folder name. */
#define LOCAL_UI_MAX_FOLDER_NAME_LN 35

/** \brief Unique, structured ID of folder. Assigned when added to tree. */
typedef uint32_t folder_id;

/** \brief Data field that can be passed to a ::folder_action callback. */
typedef int folder_action_data;
/** \brief Action function assigned to folder. If true is returned, tree returns to root. */
typedef bool (*folder_action)(folder_id, uint8_t, folder_action_data);

/** \brief A single folder. 
 * 
 * The folder structure is basically a linked list with some extra functionality. 
 * Therefore, the folder can be thought of as a node in a linked list.
*/
typedef struct local_ui_folder_ {
    uint32_t id;                            /**< \brief A unique ID assigned to a folder */
    uint16_t rel_id;                        /**< \brief An unstructured ID based on parent rel_id and subfolder index.*/
    struct local_ui_folder_ * parent;       /**< \brief Parent folder. NULL if root. */
    char name[LOCAL_UI_MAX_FOLDER_NAME_LN+1]; /**< \brief The folder's name as a null-terminated string*/
    folder_action action;                   /**< \brief An optional folder action callback*/
    folder_action_data data;                /**< \brief An integer value used to pass data to action folders */
    uint8_t num_subfolders;                 /**< \brief The number of subfolders under folder */
    struct local_ui_folder_ * subfolders [LOCAL_UI_MAX_SUBFOLDER_NUM];  /**< \brief A list of pointers to a folder's subfolders */
} local_ui_folder;

/** \brief Object representing an entire folder structure. */
typedef struct {
    local_ui_folder * root;       /**< \brief Pointer to the root of the tree */
    local_ui_folder * cur_folder; /**< \brief Pointer to the tree's active folder*/
} local_ui_folder_tree;

/**
 * \brief Sets up a directory tree.
 * 
 * \param tree Tree structure that will track the directory.
 * \param root Folder that acts as the root of the tree.
 * \param root_name Human readable name of root folder.
 */
void local_ui_folder_tree_init(local_ui_folder_tree * tree, 
                               local_ui_folder * root, 
                               const char * root_name);

/**
 * \brief Adds a subfolder with the given name and action (optional) to the indicated folder.
 * 
 * \param folder Pointer to the folder which the subfolder is added to.
 * \param subfolder Pointer to the subfolder struct.
 * \param subfolder_name Human readable name for the subfolder.
 * \param subfolder_action Pointer to an action callback. Null if no action.
 * \param subfolder_action_data Data that will be passed to the subfolder_action callback.
 */
void local_ui_add_subfolder(local_ui_folder * folder, 
                            local_ui_folder * subfolder, 
                            const char * subfolder_name, 
                            folder_action subfolder_action, 
                            folder_action_data subfolder_action_data);

/**
 * \brief Go up a level in the folders
 * \param tree Pointer to tree that should be ascended.
 * \returns True if success. Else, false (e.g. already at top level).
 */
bool local_ui_go_up(local_ui_folder_tree * tree);

/**
 * \brief Returns to the root of the tree.
 * \param tree Pointer to tree that should be ascended.
 * \returns True if success. Else, false (e.g. already at top level).
 */
bool local_ui_go_to_root(local_ui_folder_tree * tree);

/**
 * \brief Enter a subfolder of the active folder in the tree.
 * 
 * If the index is out of range, nothing is done and the function returns.
 * If the subfolder is an action folder, the action callback is triggered and the subfolder_idx
 * is passed as a parameter along with the folder's action data.
 * 
 * \param tree Full tree structure
 * \param subfolder_idx Index of subfolder to enter or parameter passed to callback.
 * 
 * \returns True if another folder was entered. Else, returns false (e.g. an action was called).
 */
bool local_ui_enter_subfolder(local_ui_folder_tree * tree, uint8_t subfolder_idx);

/**
 * \brief Indicates if passed in folder is an action folder
 * 
 * \param folder Pointer to folder to examine
 * \return true if folder action is not NULL. Else false. 
 */
bool local_ui_is_action_folder(local_ui_folder * folder);

/**
 * \brief Checks if an ID belongs to a tree rooted at f.
 * 
 * \param f Folder at subtree root.
 * \param id ID to look for in subtree.
 * 
 * \returns True if f subtree would contain ID. False else.
 */
bool local_ui_id_in_subtree(local_ui_folder * f, uint32_t id);

#endif

/** \} */