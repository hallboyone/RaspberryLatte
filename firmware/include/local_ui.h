#ifndef LOCAL_UI_H
#define LOCAL_UI_H
#include "pico/stdlib.h"

typedef void (*folder_action)(uint32_t);

typedef struct local_ui_folder_ {
    uint32_t id;
    char * name;
    folder_action action;
    uint8_t num_subfolders;
    struct local_ui_folder_ ** subfolders;
    uint8_t _subfolder_buf_size;
} local_ui_folder;

typedef struct local_ui_folder_tree_{
    local_ui_folder * root;
    local_ui_folder * cur_folder;
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
 * If the subfolder is an action folder, the action callback is triggered and the 
 * current folder doesn't change.
 */
void local_ui_enter_subfolder(local_ui_folder_tree * tree, uint8_t subfolder_idx);
#endif