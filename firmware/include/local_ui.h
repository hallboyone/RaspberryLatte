#ifndef LOCAL_UI_H
#define LOCAL_UI_H
#include "pico/stdlib.h"

/** \brief Unique, structured ID of folder. Assigned when added to tree. */
typedef uint32_t folder_id;

/** \brief Action function asigned to folder. If true is returned, tree returns to root. */
typedef bool (*folder_action)(folder_id, uint8_t);

/** \brief A single folder. The folder structure is basically a linked
 * list with some extra functionality. Therefore, the folder can be thought
 * of as a node in a linked list.
*/
typedef struct local_ui_folder_ {
    uint32_t id;                            /**< A unique ID assigned to a folder */
    char * name;                            /**< The folder's name as a null-terminated string*/
    folder_action action;                   /**< An optional folder action callback*/
    uint8_t num_subfolders;                 /**< The number of subfolders under folder */
    struct local_ui_folder_ ** subfolders;  /**< A list of pointers to a folder's subfolders */
    uint8_t _subfolder_buf_size;            /**< Internal var used to allocate space for subfolder pointers*/
} local_ui_folder;

/** \brief Object representing an entire folder structure. */
typedef struct local_ui_folder_tree_{
    local_ui_folder * root;       /**< Pointer to the root of the tree */
    local_ui_folder * cur_folder; /**< Pointer to the tree's active folder*/
} local_ui_folder_tree;

/**
 * \brief Checks if an ID belongs to a tree rooted at f.
 * 
 * \param f Folder at subtree root.
 * \param id ID to look for in subtree.
 * 
 * \returns True if f subtree would contain ID. False else.
 */
bool local_ui_id_in_subtree(local_ui_folder * f, uint32_t id);

/**
 * \brief Sets up a directory tree.
 * 
 * \param tree Tree structure that will track the directory.
 * \param root Folder that acts as the root of the tree.
 * \param root_name Human readable name of root folder.
 */
void local_ui_folder_tree_init(local_ui_folder_tree * tree, local_ui_folder * root, const char * root_name);

/**
 * \brief Adds a subfolder with the given name and action (opt) to the indicated folder.
 * 
 * \param folder Pointer to the folder which the subfolder is added to.
 * \param subfolder Pointer to the subfolder struct.
 * \param subfolder_name Human readable name for the subfolder.
 * \param action Pointer to an action callback. Null if no action.
 */
void local_ui_add_subfolder(local_ui_folder * folder, local_ui_folder * subfolder, const char * subfolder_name, folder_action subfolder_action);

/**
 * \brief Returns to the root of the tree
 */
void local_ui_go_to_root(local_ui_folder_tree * tree);

/**
 * \brief Enter a subfolder of the active folder in the tree.
 * 
 * If the index is out of range, nothing is done and the function returns.
 * If the subfolder is an action folder, the action callback is triggered and the subfolder_idx
 * is passed as a parameter.
 * 
 * \param tree Full tree structure
 * \param subfolder_idx Index of subfolder to enter or parameter passed to callback.
 */
void local_ui_enter_subfolder(local_ui_folder_tree * tree, uint8_t subfolder_idx);
#endif