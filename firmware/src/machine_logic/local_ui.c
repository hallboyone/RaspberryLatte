/**
 * \ingroup local_ui
 * 
 * \file local_ui.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Local UI source
 * \version 0.1
 * \date 2022-11-15
*/

#include "machine_logic/local_ui.h"

#include <string.h>
#include <stdio.h>

//#define DEBUG_LOCAL_UI

#ifdef DEBUG_LOCAL_UI
static int _num_lines = 0;

static void _delete_chars(){
    for(int i = 0; i < _num_lines; i++){
        printf("\033[1A\033[2K");
    }
    _num_lines = 0;
}
#endif

/**
 * \brief Returns the portion of the ID at the indicated level.
 * 
 * \param id A \ref folder_id to split
 * \param level The level of \p id to return
 * 
 * \return Bits of \p id ranging from (\p level*4) to (\p level*4+3)
*/
static uint8_t _local_ui_id_splitter(const folder_id id, const uint8_t level){
    if(level >= 8) return 0;
    return (id & (0xF<<(4*level)))>>(4*level);
}

/**
 * \brief Returns the level of the passed in folder based on its ID.
 * 
 * Each level of folders are identified by the bits in their IDs from level*4 to level*4+3. To determine
 * the level of a folder, this function looks for the first group of bits that are all zero.
 */
static uint8_t _local_ui_folder_level(local_ui_folder * f){
    uint8_t level = 0;
    while(_local_ui_id_splitter(f->id, level)){
        level += 1;
    }
    return level;
}

/** \brief Compute the ID for a subfolder. Value is based on parent's ID and the order
 * in which subfolder was added to parent.
*/
static void _local_ui_init_subfolder_id(local_ui_folder * parent, local_ui_folder * child){
    const uint32_t child_level_id = parent->num_subfolders;
    const uint32_t parent_level = _local_ui_folder_level(parent);
    child->id = (parent->id | child_level_id<<(4*parent_level));
}

void local_ui_folder_tree_init(local_ui_folder_tree * tree, local_ui_folder * root, const char * root_name){
    tree->root = root;
    tree->cur_folder = root;
    assert(strlen(root_name) <= LOCAL_UI_MAX_FOLDER_NAME_LN);
    strncpy(root->name, root_name, LOCAL_UI_MAX_FOLDER_NAME_LN);
    root->id = 0;
    root->num_subfolders = 0;
    root->parent = NULL;
    root->action = NULL;
    root->rel_id = 0;
}

void local_ui_add_subfolder(local_ui_folder * folder,
                            local_ui_folder * subfolder, 
                            const char * subfolder_name, 
                            folder_action subfolder_action, 
                            folder_action_data subfolder_action_data){
    folder->subfolders[folder->num_subfolders] = subfolder;
    folder->num_subfolders += 1;

    assert(strlen(subfolder_name) <= LOCAL_UI_MAX_FOLDER_NAME_LN);
    strncpy(subfolder->name, subfolder_name, LOCAL_UI_MAX_FOLDER_NAME_LN);
    _local_ui_init_subfolder_id(folder, subfolder);
    subfolder->parent = folder;
    subfolder->action = subfolder_action;
    subfolder->data = subfolder_action_data;
    subfolder->num_subfolders = 0;
    subfolder->rel_id = folder->rel_id + folder->num_subfolders;
}

bool local_ui_go_up(local_ui_folder_tree * tree){
    if(tree->cur_folder->parent != NULL){
        tree->cur_folder = tree->cur_folder->parent;
        #ifdef DEBUG_LOCAL_UI
        _delete_chars();
        _num_lines = 1 + tree->cur_folder->num_subfolders;
        printf("Entered [%s] with subfolders:\n", tree->cur_folder->name);
        for(uint8_t i = 0; i < tree->cur_folder->num_subfolders; i++){
            printf(" (%d) [%s]\n", i+1, tree->cur_folder->subfolders[i]->name);
        }
        #endif
        return true;
    } else {
        return false;
    }
}

bool local_ui_go_to_root(local_ui_folder_tree * tree){
    if(tree->cur_folder != tree->root){
        #ifdef DEBUG_LOCAL_UI
        _delete_chars();
        _num_lines = 1 + tree->root->num_subfolders;
        printf("Returned to root with subfolders:\n");
        for(uint8_t i = 0; i < tree->root->num_subfolders; i++){
            printf(" (%d) [%s]\n", i+1, tree->root->subfolders[i]->name);
        }
        #endif
        tree->cur_folder = tree->root;
        return true;
    } else {
        return false;
    }
}

bool local_ui_enter_subfolder(local_ui_folder_tree * tree, uint8_t subfolder_idx){
    if(tree->cur_folder->action != NULL){
        // If in action folder, just call action instead of entering subfolder
        if(tree->cur_folder->action(tree->cur_folder->id, subfolder_idx, tree->cur_folder->data)){
            #ifdef DEBUG_LOCAL_UI
            _delete_chars();
            _num_lines = 1;
            printf("Calling action on [%s] and returning to root\n", tree->cur_folder->name);
            #endif
            local_ui_go_to_root(tree);
            return true;
        } else {
            #ifdef DEBUG_LOCAL_UI
            _delete_chars();
            _num_lines = 1;
            printf("Calling action on [%s]\n", tree->cur_folder->name);
            #endif
            return false;
        }
    } else if(subfolder_idx < tree->cur_folder->num_subfolders){
        // Not in action folder. Enter subfolder if valid index
        if(subfolder_idx < tree->cur_folder->num_subfolders){
            tree->cur_folder = tree->cur_folder->subfolders[subfolder_idx];
            #ifdef DEBUG_LOCAL_UI
            _delete_chars();
            if(tree->cur_folder->action != NULL){
                _num_lines = 1;
                printf("Entered action folder [%s].\n", tree->cur_folder->name);
            } else {
                _num_lines = 1 + tree->cur_folder->num_subfolders;
                printf("Entered [%s] with subfolders:\n", tree->cur_folder->name);
                for(uint8_t i = 0; i < tree->cur_folder->num_subfolders; i++){
                    printf(" (%d) [%s]\n", i+1, tree->cur_folder->subfolders[i]->name);
                }
            }
            #endif
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool local_ui_is_action_folder(local_ui_folder * folder){
    return folder->action != NULL;
}

bool local_ui_id_in_subtree(local_ui_folder * f, uint32_t id){
    const uint8_t level = _local_ui_folder_level(f);
    const folder_id id_mask = ~(~((folder_id)0)<<(4*level));
    return f->id == (id & id_mask);
}