#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "local_ui.h"

uint8_t local_ui_id_splitter(const uint32_t id, const uint8_t level){
    if(level >= 8) return 0;
    return (id & (0xF<<(4*level)))>>(4*level);
}

/**
 * \brief Returns the level of the passed in folder based on its ID.
 * 
 * Each level of folders are identified by the bits in their IDs from level*4 to level*4+3. To determine
 * the level of a folder, this function looks for the first group of bits that are all zero.
 */
uint8_t local_ui_folder_level(local_ui_folder * f){
    uint8_t level = 0;
    while(local_ui_id_splitter(f->id, level)){
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

static void _local_ui_init_subfolder_id(local_ui_folder * parent, local_ui_folder * child){
    const uint32_t child_level_id = parent->num_subfolders;
    const uint32_t parent_level = local_ui_folder_level(parent);
    child->id = (parent->id | child_level_id<<(4*parent_level));
}

void local_ui_folder_tree_init(local_ui_folder_tree * tree, local_ui_folder * root, const char * root_name){
    tree->root = root;
    tree->cur_folder = root;
    _local_ui_init_folder_name(root, root_name);
    root->id = 0;
    root->num_subfolders = 0;
    root->action = NULL;
    root->subfolders = NULL;
    root->_subfolder_buf_size = 0;
}

void local_ui_add_subfolder(local_ui_folder * folder, local_ui_folder * subfolder, const char * subfolder_name, folder_action subfolder_action){
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
    subfolder->action = subfolder_action;
    subfolder->num_subfolders = 0;
    subfolder->subfolders = NULL;
    subfolder->_subfolder_buf_size = 0;
}

void local_ui_go_to_root(local_ui_folder_tree * tree){
    tree->cur_folder = tree->root;
}

void local_ui_enter_subfolder(local_ui_folder_tree * tree, uint8_t subfolder_idx){
    if(subfolder_idx < tree->cur_folder->num_subfolders){
        // If entering action folder, just call action instead of entering
        if(tree->cur_folder->subfolders[subfolder_idx]->action != NULL){
            tree->cur_folder->subfolders[subfolder_idx]->action(tree->cur_folder->subfolders[subfolder_idx]->id);
        } else {
            tree->cur_folder = tree->cur_folder->subfolders[subfolder_idx];
        }
    }
}