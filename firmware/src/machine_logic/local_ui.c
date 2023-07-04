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
#include <stdlib.h>
#include <stdio.h>

#define DEBUG_LOCAL_UI

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

/**
 * \brief Copy the string literal \p name into the name field of folder f.
 */
static void _local_ui_init_folder_name(local_ui_folder * f, const char * name){
    const size_t name_len = strlen(name);
    f->name = (char*)malloc((name_len+1)*sizeof(char));
    strncpy(f->name, name, name_len);
    f->name[name_len] = '\0';    
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
    _local_ui_init_folder_name(root, root_name);
    root->id = 0;
    root->num_subfolders = 0;
    root->parent = NULL;
    root->action = NULL;
    root->subfolders = NULL;
    root->_subfolder_buf_size = 0;
    root->rel_id = 0;
}

void local_ui_add_subfolder(local_ui_folder * folder, local_ui_folder * subfolder, const char * subfolder_name, folder_action subfolder_action, folder_action_data subfolder_action_data){
    if(folder->num_subfolders == folder->_subfolder_buf_size){
        local_ui_folder ** new_buf = (local_ui_folder**)malloc(2*folder->_subfolder_buf_size*sizeof(local_ui_folder *));
        memcpy(folder->subfolders, new_buf, folder->num_subfolders*sizeof(local_ui_folder *));
        free(folder->subfolders);
        folder->subfolders = new_buf;
        folder->_subfolder_buf_size *= 2;
    }

    folder->subfolders[folder->num_subfolders] = subfolder;
    folder->num_subfolders += 1;

    _local_ui_init_folder_name(subfolder, subfolder_name);
    _local_ui_init_subfolder_id(folder, subfolder);
    subfolder->parent = folder;
    subfolder->action = subfolder_action;
    subfolder->data = subfolder_action_data;
    subfolder->num_subfolders = 0;
    subfolder->subfolders = NULL;
    subfolder->_subfolder_buf_size = 0;
    subfolder->rel_id = folder->rel_id + folder->num_subfolders;
}

void local_ui_go_up(local_ui_folder_tree * tree){
    if(tree->cur_folder->parent != NULL){
        tree->cur_folder = tree->cur_folder->parent;
    }
    #ifdef DEBUG_LOCAL_UI
    printf("Entered [%s] with subfolders:\n", tree->cur_folder->name);
    for(uint8_t i = 0; i < tree->cur_folder->num_subfolders; i++){
        printf(" (%d) [%s]\n", i+1, tree->cur_folder->subfolders[i]->name);
    }
    #endif
}

void local_ui_go_to_root(local_ui_folder_tree * tree){
    #ifdef DEBUG_LOCAL_UI
    if(tree->cur_folder != tree->root){
        printf("Returned to root with subfolders:\n");
        for(uint8_t i = 0; i < tree->root->num_subfolders; i++){
            printf(" (%d) [%s]\n", i+1, tree->root->subfolders[i]->name);
        }
    }
    #endif
    tree->cur_folder = tree->root;
}

void local_ui_enter_subfolder(local_ui_folder_tree * tree, uint8_t subfolder_idx){
    if(tree->cur_folder->action != NULL){
        // If in action folder, just call action instead of entering subfolder
        if(tree->cur_folder->action(tree->cur_folder->id, subfolder_idx, tree->cur_folder->data)){
            #ifdef DEBUG_LOCAL_UI
            printf("Calling action on [%s] with value %d\n", tree->cur_folder->name, subfolder_idx);
            #endif
            local_ui_go_to_root(tree);
        }
    } else if(subfolder_idx < tree->cur_folder->num_subfolders){
        // Not in action folder. Enter subfolder if valid index
        tree->cur_folder = tree->cur_folder->subfolders[subfolder_idx];
        #ifdef DEBUG_LOCAL_UI
        if(tree->cur_folder->action != NULL){
            printf("Entered action folder [%s].\n", tree->cur_folder->name);
        } else {
            printf("Entered [%s] with subfolders:\n", tree->cur_folder->name);
            for(uint8_t i = 0; i < tree->cur_folder->num_subfolders; i++){
                printf(" (%d) [%s]\n", i+1, tree->cur_folder->subfolders[i]->name);
            }
        }
        #endif
    }
}

bool local_ui_is_action_folder(local_ui_folder * folder){
    return folder->action != NULL;
}

bool local_ui_id_in_subtree(local_ui_folder * f, uint32_t id){
    const uint8_t level = _local_ui_folder_level(f);
    const folder_id id_mask = ~(~((folder_id)0)<<(4*level));
    return f->id == (id & id_mask);
}