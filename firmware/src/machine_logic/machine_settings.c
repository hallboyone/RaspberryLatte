/**
 * @ingroup machine_settings
 * @{
 * 
 * \file machine_settings.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Machine Settings source
 * \version 1.0
 * \date 2023-07-01
 */

#include "machine_logic/machine_settings.h"

#include <stdio.h>

#include "machine_logic/local_ui.h"
#include "utils/value_flasher.h"
#include "utils/macros.h"


/** Printing the folder structure takes at lease 4 lines */
static const uint8_t local_ui_num_ln = 4;

/**
 * \brief Clear and reprint the corresponding line of the machine settings display
 * 
 * \param ln_num The line to reprint
 */
static void _machine_settings_print_ln(uint ln_num);

/** \brief Starting address in mb85 FRAM chip where setting data is stored */
static const reg_addr MACHINE_SETTINGS_START_ADDR = 0x0000;

/** \brief The size, in bytes, of a single settings profile */
static const uint16_t MACHINE_SETTINGS_MEMORY_SIZE = NUM_SETTINGS * sizeof(machine_setting);

/** \brief Pointer to FRAM memory IC object where settings are stored */
static mb85_fram _mem = NULL;

/** \brief Flasher object to display a setting when it's getting modified. */
static value_flasher _setting_flasher;
/** \brief Bitfield used to store output of ::_setting_flasher. */
static uint8_t _ui_mask;

/** \brief Internal settings array holding the current settings */
static machine_setting _ms [NUM_SETTINGS];

/**
 * \brief The scale, bounds, and default for a machine setting.
 * Also contains the line of the console display that contains the setting.
 */
typedef struct{
    machine_setting scale; /**<\brief The amount the base unit is scaled. Larger means more precise but slower to adjust. 0 is for enumerated values. */
    machine_setting max;   /**<\brief The maximum value of the setting after scaling. */
    machine_setting min;   /**<\brief The minimum value of the setting after scaling. */
    machine_setting std;   /**<\brief The default value of the setting after scaling. */
    uint8_t ln_idx;        /**<\brief The setting's line in the settings console display*/
} machine_setting_specs;

/** \brief Minimum settings array */
static const machine_setting_specs _specs [NUM_SETTINGS] = {
    {.scale = 10,  .min = 0,   .max = 1400, .std = 900 , .ln_idx = 1},// MS_TEMP_BREW_10C = 0
    {.scale = 10,  .min = 0,   .max = 1400, .std = 1000, .ln_idx = 2},// MS_TEMP_HOT_10C
    {.scale = 10,  .min = 0,   .max = 1400, .std = 1400, .ln_idx = 3},// MS_TEMP_STEAM_10C
    {.scale = 10,  .min = 0,   .max = 300,  .std = 150 , .ln_idx = 4},// MS_WEIGHT_DOSE_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 300 , .ln_idx = 5},// MS_WEIGHT_YIELD_10g
    {.scale = 1,   .min = 0,   .max = 100,  .std = 100 , .ln_idx = 6},// MS_POWER_BREW_PER,   
    {.scale = 1,   .min = 0,   .max = 100,  .std = 20  , .ln_idx = 7},// MS_POWER_HOT_PER
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 13},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 13},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 13},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 13},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 13},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 13},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 13},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 14},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 14},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 14},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 14},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 14},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 14},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 14},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 15},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 15},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 15},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 15},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 15},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 15},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 15},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 16},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 16},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 16},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 16},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 16},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 16},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 16},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 17},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 17},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 17},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 17},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 17},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 17},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 17},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 18},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 18},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 18},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 18},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 18},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 18},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 18},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 19},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 19},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 19},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 19},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 19},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 19},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 19},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 20},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 20},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 20},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 20},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 20},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 20},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 20},// MS_A1_TIMEOUT_s
    {.scale = 0,   .min = 0,   .max = 2,    .std = 0   , .ln_idx = 21},// MS_A1_REF_STYLE_ENM
    {.scale = 10,  .min = 0,   .max = 300,  .std = 25  , .ln_idx = 21},// MS_A1_REF_START_per_100mlps_10bar
    {.scale = 1,   .min = 0,   .max = 300,  .std = 25  , .ln_idx = 21},// MS_A1_REF_END_per_100mlps_10bar
    {.scale = 100, .min =-300, .max = 300,  .std = 0   , .ln_idx = 21},// MS_A1_TRGR_FLOW_100mlps
    {.scale = 10,  .min =-150, .max = 150,  .std = 0   , .ln_idx = 21},// MS_A1_TRGR_PRSR_10bar
    {.scale = 10,  .min =-300, .max = 300,  .std = 0   , .ln_idx = 21},// MS_A1_TRGR_MASS_10g
    {.scale = 10,  .min = 0,   .max = 600,  .std = 0   , .ln_idx = 21},// MS_A1_TIMEOUT_s
    };

static local_ui_folder_tree settings_modifier; /**< \brief Local UI folder tree for updating machine settings*/
static local_ui_folder  f_; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder      f_set; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_set_temp; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_set_temp_brew; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_set_temp_hot; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_set_temp_steam; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_set_weight; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_set_weight_yield; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_set_weight_dose;  /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_set_power; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_set_power_brew; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_set_power_hot; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder      f_ab; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_ab_ab13; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab13_ab1; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab1_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab1_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab1_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab1_ref_end; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab1_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab1_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab1_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab1_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab1_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab13_ab2; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab2_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab2_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab2_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab2_ref_end; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab2_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab2_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab2_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab2_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab2_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab13_ab3; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab3_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab3_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab3_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab3_ref_end; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab3_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab3_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab3_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab13_ab3_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab13_ab3_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_ab_ab46; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab46_ab4; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab4_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab4_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab4_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab4_ref_end; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab4_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab4_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab4_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab4_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab4_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab46_ab5; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab5_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab5_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab5_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab5_ref_end;  /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab5_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab5_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab5_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab5_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab5_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab46_ab6; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab6_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab6_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab6_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab6_ref_end; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab6_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab6_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab6_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab46_ab6_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab46_ab6_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_ab_ab79; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab79_ab7; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab7_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab7_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab7_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab7_ref_end; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab7_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab7_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab7_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab7_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab7_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab79_ab8; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab8_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab8_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab8_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab8_ref_end;  /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab8_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab8_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab8_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab8_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab8_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_ab_ab79_ab9; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab9_ref; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab9_ref_style; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab9_ref_start; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab9_ref_end; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab9_trgr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab9_trgr_flow; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab9_trgr_prsr; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                      f_ab_ab79_ab9_trgr_mass; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder                  f_ab_ab79_ab9_timeout; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder      f_presets; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_presets_profile_a; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_a_0; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_a_1; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_a_2; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_presets_profile_b; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_b_0; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_b_1; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_b_2; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder          f_presets_profile_c; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_c_0; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_c_1; /**< \brief Folder in tree used to update machine settings. */
static local_ui_folder              f_presets_profile_c_2; /**< \brief Folder in tree used to update machine settings. */

/**
 * \brief Returns the starting address of the given settings profile
 * \param id Profile number
 * \return Started memory address where profile is stored 
 */
static inline reg_addr _machine_settings_id_to_addr(uint8_t id){
    return MACHINE_SETTINGS_START_ADDR + (1+id)*MACHINE_SETTINGS_MEMORY_SIZE;
}

/**
 * \brief Restores settings to default state if any invalid values are found
 * 
 * \return true Invalid values found.
 * \return false No invalid values found.
 */
static bool _machine_settings_verify(){
    for(uint8_t p_id = 0; p_id < NUM_SETTINGS; p_id++){
        if(_ms[p_id] > _specs[p_id].max || _ms[p_id] < _specs[p_id].min){
            // Invalid setting found. Copy in defaults and return true
            for(uint8_t p_id_2 = 0; p_id_2 < NUM_SETTINGS; p_id_2++){
                _ms[p_id_2] = _specs[p_id_2].std;
            }
            return true;
        }
    }
    return false;
}

/**
 * \brief Save the current settings array, \p _ms to the MB85 FRAM, \p _mem.
 * 
 * \param profile_id The number to save the profile under, 0 <= profile_id <= 8
 * \return PICO_ERROR_GENERIC if library not setup. Else PICO_ERROR_GENERIC.
 */
static int _machine_settings_save_profile(uint8_t profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile save address
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, _machine_settings_id_to_addr(profile_id), MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);

    // Break link with save address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

/**
 * \brief Load the indicated profile into the settings array, \p _ms from the MB85 FRAM, \p _mem.
 * 
 * \param profile_id The profile number to load, 0 <= profile_id <= 8
 * \return PICO_ERROR_GENERIC if library not setup. Else PICO_ERROR_GENERIC.
 */
static int _machine_settings_load_profile(uint8_t profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile load address
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, _machine_settings_id_to_addr(profile_id), MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_FRAM);

    // Verify that loaded address is valid. If it wasn't, save back into profile.
    if(_machine_settings_verify()) mb85_fram_save(_mem, &_ms);

    // Break link with load address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

/**
 * \brief Callback function used by setting action folders. Increments corresponding
 * setting or calls preset load/save function
 * 
 * \param id ID of calling folder
 * \param val Value of increment index. 0 = -10, 1 = +1, and 2 = +10
 * \param ms_id The ID of the calling folder's machine setting.
 * \returns True if value was greater than 2. False otherwise
 */
static bool _ms_f_cb(folder_id id, uint8_t val, folder_action_data ms_id){
    if (val > 2) return true;
    if (local_ui_id_in_subtree(&f_set, id) || local_ui_id_in_subtree(&f_ab, id) ){
        // Get step size (special case if base step size is 0)
        const int16_t deltas [3] = {-10, 1, 10};
        const int16_t step = (_specs[ms_id].scale==0) ? val-1 : deltas[val];

        // Increment and clamp setting
        _ms[ms_id] = CLAMP(_ms[ms_id] + step, _specs[ms_id].min, _specs[ms_id].max);

        // Print results and save
        _machine_settings_print_ln(_specs[ms_id].ln_idx);
        mb85_fram_save(_mem, _ms);
    } else if (local_ui_id_in_subtree(&f_presets, id)){
        // Save or load presets
        if (val == 0){
            _machine_settings_save_profile(ms_id);
        } else if (val == 1){
            _machine_settings_load_profile(ms_id);
            machine_settings_print();
            machine_settings_print_local_ui();
        }
    }
    return false;
}

/** \brief Setup the local UI file structure. */
static void _machine_settings_setup_local_ui(){
    local_ui_folder_tree_init(&settings_modifier, &f_, "RaspberryLatte");
    local_ui_add_subfolder(&f_,                 &f_set,                   "Settings",                     NULL, 0);
    local_ui_add_subfolder(&f_set,              &f_set_temp,              "Temperatures",                 NULL, 0);
    local_ui_add_subfolder(&f_set_temp,         &f_set_temp_brew,         "Brew (-1, 0.1, 1)",            &_ms_f_cb, MS_TEMP_BREW_10C);
    local_ui_add_subfolder(&f_set_temp,         &f_set_temp_hot,          "Hot (-1, 0.1, 1)",             &_ms_f_cb, MS_TEMP_HOT_10C);
    local_ui_add_subfolder(&f_set_temp,         &f_set_temp_steam,        "Steam (-1, 0.1, 1)",           &_ms_f_cb, MS_TEMP_STEAM_10C);
    local_ui_add_subfolder(&f_set,              &f_set_weight,            "Weights",                      NULL, 0);
    local_ui_add_subfolder(&f_set_weight,       &f_set_weight_dose,       "Dose (-1, 0.1, 1)",            &_ms_f_cb, MS_WEIGHT_DOSE_10g);
    local_ui_add_subfolder(&f_set_weight,       &f_set_weight_yield,      "Yield (-1, 0.1, 1)",           &_ms_f_cb, MS_WEIGHT_YIELD_10g);
    local_ui_add_subfolder(&f_set,              &f_set_power,             "Power",                        NULL, 0);
    local_ui_add_subfolder(&f_set_power,        &f_set_power_brew,        "Brew (-10, 1, 10)",            &_ms_f_cb, MS_POWER_BREW_PER);
    local_ui_add_subfolder(&f_set_power,        &f_set_power_hot,         "Hot (-10, 1, 10)",             &_ms_f_cb, MS_POWER_HOT_PER);

    local_ui_add_subfolder(&f_,                 &f_ab,                    "Autobrew",                     NULL, 0);
    local_ui_add_subfolder(&f_ab,               &f_ab_ab13,               "Autobrew Legs 1-3",            NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13,          &f_ab_ab13_ab1,           "Autobrew Leg 1",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab1,      &f_ab_ab13_ab1_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab1_ref,  &f_ab_ab13_ab1_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A1_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab13_ab1_ref,  &f_ab_ab13_ab1_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A1_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab1_ref,  &f_ab_ab13_ab1_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A1_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab1,      &f_ab_ab13_ab1_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab1_trgr, &f_ab_ab13_ab1_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A1_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab13_ab1_trgr, &f_ab_ab13_ab1_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A1_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab1_trgr, &f_ab_ab13_ab1_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A1_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab13_ab1,      &f_ab_ab13_ab1_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A1_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab_ab13,          &f_ab_ab13_ab2,           "Autobrew Leg 2",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab2,      &f_ab_ab13_ab2_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab2_ref,  &f_ab_ab13_ab2_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A2_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab13_ab2_ref,  &f_ab_ab13_ab2_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A2_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab2_ref,  &f_ab_ab13_ab2_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A2_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab2,      &f_ab_ab13_ab2_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab2_trgr, &f_ab_ab13_ab2_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A2_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab13_ab2_trgr, &f_ab_ab13_ab2_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A2_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab2_trgr, &f_ab_ab13_ab2_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A2_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab13_ab2,      &f_ab_ab13_ab2_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A2_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab_ab13,          &f_ab_ab13_ab3,           "Autobrew Leg 3",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab3,      &f_ab_ab13_ab3_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab3_ref,  &f_ab_ab13_ab3_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A3_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab13_ab3_ref,  &f_ab_ab13_ab3_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A3_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab3_ref,  &f_ab_ab13_ab3_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A3_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab3,      &f_ab_ab13_ab3_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab3_trgr, &f_ab_ab13_ab3_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A3_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab13_ab3_trgr, &f_ab_ab13_ab3_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A3_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab13_ab3_trgr, &f_ab_ab13_ab3_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A3_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab13_ab3,      &f_ab_ab13_ab3_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A3_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab,               &f_ab_ab46,               "Autobrew Legs 4-6",            NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46,          &f_ab_ab46_ab4,           "Autobrew Leg 4",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab4,      &f_ab_ab46_ab4_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab4_ref,  &f_ab_ab46_ab4_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A4_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab46_ab4_ref,  &f_ab_ab46_ab4_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A4_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab4_ref,  &f_ab_ab46_ab4_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A4_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab4,      &f_ab_ab46_ab4_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab4_trgr, &f_ab_ab46_ab4_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A4_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab46_ab4_trgr, &f_ab_ab46_ab4_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A4_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab4_trgr, &f_ab_ab46_ab4_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A4_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab46_ab4,      &f_ab_ab46_ab4_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A4_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab_ab46,          &f_ab_ab46_ab5,           "Autobrew Leg 5",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab5,      &f_ab_ab46_ab5_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab5_ref,  &f_ab_ab46_ab5_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A5_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab46_ab5_ref,  &f_ab_ab46_ab5_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A5_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab5_ref,  &f_ab_ab46_ab5_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A5_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab5,      &f_ab_ab46_ab5_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab5_trgr, &f_ab_ab46_ab5_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A5_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab46_ab5_trgr, &f_ab_ab46_ab5_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A5_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab5_trgr, &f_ab_ab46_ab5_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A5_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab46_ab5,      &f_ab_ab46_ab5_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A5_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab_ab46,          &f_ab_ab46_ab6,           "Autobrew Leg 6",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab6,      &f_ab_ab46_ab6_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab6_ref,  &f_ab_ab46_ab6_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A6_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab46_ab6_ref,  &f_ab_ab46_ab6_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A6_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab6_ref,  &f_ab_ab46_ab6_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A6_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab6,      &f_ab_ab46_ab6_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab6_trgr, &f_ab_ab46_ab6_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A6_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab46_ab6_trgr, &f_ab_ab46_ab6_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A6_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab46_ab6_trgr, &f_ab_ab46_ab6_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A6_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab46_ab6,      &f_ab_ab46_ab6_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A6_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab,               &f_ab_ab79,               "Autobrew Legs 7-9",            NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79,          &f_ab_ab79_ab7,           "Autobrew Leg 7",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab7,      &f_ab_ab79_ab7_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab7_ref,  &f_ab_ab79_ab7_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A7_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab79_ab7_ref,  &f_ab_ab79_ab7_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A7_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab7_ref,  &f_ab_ab79_ab7_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A7_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab7,      &f_ab_ab79_ab7_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab7_trgr, &f_ab_ab79_ab7_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A7_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab79_ab7_trgr, &f_ab_ab79_ab7_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A7_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab7_trgr, &f_ab_ab79_ab7_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A7_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab79_ab7,      &f_ab_ab79_ab7_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A7_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab_ab79,          &f_ab_ab79_ab8,           "Autobrew Leg 8",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab8,      &f_ab_ab79_ab8_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab8_ref,  &f_ab_ab79_ab8_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A8_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab79_ab8_ref,  &f_ab_ab79_ab8_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A8_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab8_ref,  &f_ab_ab79_ab8_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A8_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab8,      &f_ab_ab79_ab8_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab8_trgr, &f_ab_ab79_ab8_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A8_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab79_ab8_trgr, &f_ab_ab79_ab8_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A8_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab8_trgr, &f_ab_ab79_ab8_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A8_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab79_ab8,      &f_ab_ab79_ab8_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A8_TIMEOUT_10s);
    local_ui_add_subfolder(&f_ab_ab79,          &f_ab_ab79_ab9,           "Autobrew Leg 9",               NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab9,      &f_ab_ab79_ab9_ref,       "Setpoint",                     NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab9_ref,  &f_ab_ab79_ab9_ref_style, "Style (Pwr, Flow, Prsr)",      _ms_f_cb, MS_A9_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab79_ab9_ref,  &f_ab_ab79_ab9_ref_start, "Starting Setpoint",            _ms_f_cb, MS_A9_REF_START_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab9_ref,  &f_ab_ab79_ab9_ref_end,   "Ending Setpoint",              _ms_f_cb, MS_A9_REF_END_per_100mlps_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab9,      &f_ab_ab79_ab9_trgr,      "Trigger",                      NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab9_trgr, &f_ab_ab79_ab9_trgr_flow, "Flow (ml/s, -0.1, 0.01, 0.1)", _ms_f_cb, MS_A9_TRGR_FLOW_100mlps);
    local_ui_add_subfolder(&f_ab_ab79_ab9_trgr, &f_ab_ab79_ab9_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",       _ms_f_cb, MS_A9_TRGR_PRSR_10bar);
    local_ui_add_subfolder(&f_ab_ab79_ab9_trgr, &f_ab_ab79_ab9_trgr_mass, "Mass (g, -1, 0.1, 1)",         _ms_f_cb, MS_A9_TRGR_MASS_10g);
    local_ui_add_subfolder(&f_ab_ab79_ab9,      &f_ab_ab79_ab9_timeout,   "Timeout (-1, 0.1, 1)",         _ms_f_cb, MS_A9_TIMEOUT_10s);

    local_ui_add_subfolder(&f_,                 &f_presets,               "Presets",                      NULL, 0);
    local_ui_add_subfolder(&f_presets,          &f_presets_profile_a,     "Presets 1-3",                  NULL, 0);
    local_ui_add_subfolder(&f_presets_profile_a,&f_presets_profile_a_0,   "Preset 1 (1-save, 2-load)",        &_ms_f_cb, 0);
    local_ui_add_subfolder(&f_presets_profile_a,&f_presets_profile_a_1,   "Preset 2 (1-save, 2-load)",        &_ms_f_cb, 1);
    local_ui_add_subfolder(&f_presets_profile_a,&f_presets_profile_a_2,   "Preset 3 (1-save, 2-load)",        &_ms_f_cb, 2);
    local_ui_add_subfolder(&f_presets,          &f_presets_profile_b,     "Presets 4-6",                  NULL, 0);
    local_ui_add_subfolder(&f_presets_profile_b,&f_presets_profile_b_0,   "Preset 4 (1-save, 2-load)",        &_ms_f_cb, 3);
    local_ui_add_subfolder(&f_presets_profile_b,&f_presets_profile_b_1,   "Preset 5 (1-save, 2-load)",        &_ms_f_cb, 4);
    local_ui_add_subfolder(&f_presets_profile_b,&f_presets_profile_b_2,   "Preset 6 (1-save, 2-load)",        &_ms_f_cb, 5);
    local_ui_add_subfolder(&f_presets,          &f_presets_profile_c,     "Presets 7-9",                  NULL, 0);
    local_ui_add_subfolder(&f_presets_profile_c,&f_presets_profile_c_0,   "Preset 7 (1-save, 2-load)",        &_ms_f_cb, 6);
    local_ui_add_subfolder(&f_presets_profile_c,&f_presets_profile_c_1,   "Preset 8 (1-save, 2-load)",        &_ms_f_cb, 7);
    local_ui_add_subfolder(&f_presets_profile_c,&f_presets_profile_c_2,   "Preset 9 (1-save, 2-load)",        &_ms_f_cb, 8);
}

void machine_settings_setup(mb85_fram mem){
    if(_mem == NULL){
        _mem = mem;
        if(mb85_fram_link_var(_mem, &_ms, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_FRAM)){
            _mem = NULL;
            return;
        }
        if(_machine_settings_verify()){
            // If settings had to be reset to defaults, save new values.
            mb85_fram_save(_mem, &_ms);
        }
        _machine_settings_setup_local_ui();

        // Create value_flasher object
        _setting_flasher = value_flasher_setup(0, 750, &_ui_mask);

        // Clear screen
        printf("\033[2J");
    }
}

machine_setting machine_settings_get(setting_id id){
    assert(id < NUM_SETTINGS || id == MS_UI_MASK);
    if(_mem == NULL) return 0; // nothing setup yet :(
    else if (id < NUM_SETTINGS) return _ms[id];
    else return _ui_mask;
}

/**
 * \brief Read any available char from stdin.
 * \returns The passed in command or MS_CMD_NONE if none found.
*/
static setting_command _get_ssh_command(){
    int cmd = getchar_timeout_us(0);
    if(cmd < 0) cmd = MS_CMD_NONE;
    return cmd;
}

/** 
 * \brief Start and stop the value flasher depending on state of folder tree.
 * If not in an action folder, then the ::_ui_mask is set to the current folder's
 * relative ID.
*/
static void _update_value_flasher(){
    const folder_id id = settings_modifier.cur_folder->id;
    if(local_ui_is_action_folder(settings_modifier.cur_folder) &&
        (local_ui_id_in_subtree(&f_set, id) || local_ui_id_in_subtree(&f_ab, id))){
        // If entered action settings folder, start value flasher
        value_flasher_update(_setting_flasher, _ms[settings_modifier.cur_folder->data]);
        value_flasher_start(_setting_flasher);
    } else {
        // else in nav folder or root. Display id.
        value_flasher_end(_setting_flasher);
        _ui_mask = settings_modifier.cur_folder->rel_id;
    }
}

int machine_settings_update(setting_command cmd){
    if(cmd == MS_CMD_NONE) cmd = _get_ssh_command();
    bool changed_folder = false;
    switch(cmd){
        case MS_CMD_SUBFOLDER_1: 
        case MS_CMD_SUBFOLDER_2:
        case MS_CMD_SUBFOLDER_3:
        changed_folder = local_ui_enter_subfolder(&settings_modifier, cmd - MS_CMD_SUBFOLDER_1);
        break;

        case MS_CMD_ROOT:
        changed_folder = local_ui_go_to_root(&settings_modifier);
        break;

        case MS_CMD_UP:
        changed_folder = local_ui_go_up(&settings_modifier);
        break;

        case MS_CMD_PRINT:
        machine_settings_print();
        machine_settings_print_local_ui();
        break;

        case MS_CMD_NONE:
        break;

        default:
        return PICO_ERROR_INVALID_ARG;
    }

    if(changed_folder){
        machine_settings_print_local_ui();
        _update_value_flasher();
    }
    return PICO_ERROR_NONE;
}

/** \brief Enumerated list of all lines in setting's display */
enum {
    LN_TOP_BUFF = 0,    // 
    LN_BREW_TEMP,       // Brew Temp   : %5.1fC
    LN_HOT_TEMP,        // Hot Temp    : %5.1fC
    LN_STEAM_TEMP,      // Steam Temp  : %5.1fC
    LN_DOSE,            // Dose        : %5.1fC
    LN_YIELD,           // Yield       : %5.1fC
    LN_BREW_POWER,      // Brew Power  : %5.1fC
    LN_HOT_POWER,       // Hot Power   : %5.1fC  
    LN_MID_BUFF,        //      
    LN_AB_TOP_BOUNDARY, // |=|=========================|========================|=========|
    LN_AB_H1,           // | |        Setpoint         |         Target         | Timeout |
    LN_AB_H2,           // |#|  Style  : Start :  End  | Flow : Pressure : Mass |         |
    LN_AB_TOP_DIVIDE,   // |-|---------:-------:-------|------:----------:------|---------|
    LN_AB_LEG_1,        // |1|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_2,        // |2|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_3,        // |3|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_4,        // |4|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_5,        // |5|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_6,        // |6|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_7,        // |7|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_8,        // |8|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LEG_9,        // |9|   %s    : %5.1f : %5.1f | %4.2f:   %4.1f  : %4.1f|  %4.1f  |
    LN_AB_LOW_BOUNDARY, // |-|---------:-------:-------|------:----------:------|---------|
    LN_LOW_BUFF,        //
    LN_COUNT
};

static void _machine_settings_print_ln(uint ln_num){
    const uint flat_ln_num = ((ln_num >= LN_AB_LEG_1 && ln_num <= LN_AB_LEG_9) ? LN_AB_LEG_1 : ln_num);
    switch (flat_ln_num) {
    case LN_TOP_BUFF:
        printf("\033[%d;1H\033[2K\n",
        LN_TOP_BUFF);
        break;
    case LN_BREW_TEMP:
        printf("\033[%d;1H\033[2KBrew Temp   : %5.1f C\n",
        LN_BREW_TEMP+1, _ms[MS_TEMP_BREW_10C]/10.);
        break;
    case LN_HOT_TEMP:
        printf("\033[%d;1H\033[2KHot Temp    : %5.1f C\n",
        LN_HOT_TEMP+1, _ms[MS_TEMP_HOT_10C]/10.);
        break;
    case LN_STEAM_TEMP:
        printf("\033[%d;1H\033[2KSteam Temp  : %5.1f C\n",
        LN_STEAM_TEMP+1, _ms[MS_TEMP_STEAM_10C]/10.);
        break;
    case LN_DOSE:
        printf("\033[%d;1H\033[2KDose        : %5.1f g\n",
        LN_DOSE+1, _ms[MS_WEIGHT_DOSE_10g]/10.);
        break;
    case LN_YIELD:
        printf("\033[%d;1H\033[2KYield       : %5.1f g\n",
        LN_YIELD+1, _ms[MS_WEIGHT_YIELD_10g]/10.);
        break;
    case LN_BREW_POWER:
        printf("\033[%d;1H\033[2KBrew Power  :   %3d %%\n",
        LN_BREW_POWER+1, _ms[MS_POWER_BREW_PER]);
        break;
    case LN_HOT_POWER:
        printf("\033[%d;1H\033[2KHot Power   :   %3d %%\n",
        LN_HOT_POWER+1, _ms[MS_POWER_HOT_PER]);
        break;
    case LN_MID_BUFF:
        printf("\033[%d;1H\033[2K\n",
        LN_MID_BUFF + 1);
        break;
    case LN_AB_TOP_BOUNDARY:
        printf("\033[%d;1H\033[2K|=|=========================|========================|=========|\n",
        LN_AB_TOP_BOUNDARY+1);
        break;
    case LN_AB_H1:
        printf("\033[%d;1H\033[2K| |        Setpoint         |         Target         | Timeout |\n",
        LN_AB_H1+1);
        break;
    case LN_AB_H2:
        printf("\033[%d;1H\033[2K|#|  Style  : Start :  End  | Flow : Pressure : Mass |         |\n",
        LN_AB_H2+1);
        break;
    case LN_AB_TOP_DIVIDE:
        printf("\033[%d;1H\033[2K|-|---------:-------:-------|------:----------:------|---------|\n",
        LN_AB_TOP_DIVIDE+1);
        break;
    case LN_AB_LEG_1:
        {
            const uint8_t offset = (ln_num - LN_AB_LEG_1)*NUM_AUTOBREW_PARAMS_PER_LEG;
            printf("\033[%d;1H\033[2K|%d|", ln_num + 1, ln_num - LN_AB_LEG_1);
            switch (_ms[offset + MS_A1_REF_STYLE_ENM]){
                case AUTOBREW_REF_STYLE_PWR:
                printf("  Power  : %5d : %5d ",
                    _ms[offset + MS_A1_REF_START_per_100mlps_10bar],
                    _ms[offset + MS_A1_REF_END_per_100mlps_10bar]);
                    break;
                case AUTOBREW_REF_STYLE_FLOW:
                printf("  Flow   : %5.2f : %5.2f ",
                    _ms[offset + MS_A1_REF_START_per_100mlps_10bar]/100.,
                    _ms[offset + MS_A1_REF_END_per_100mlps_10bar]/100.);
                    break;
                case AUTOBREW_REF_STYLE_PRSR:
                printf(" Pressure: %5.1f : %5.1f ",
                    _ms[offset + MS_A1_REF_START_per_100mlps_10bar]/10.,
                    _ms[offset + MS_A1_REF_END_per_100mlps_10bar]/10.);
                    break;
            }
            printf("| %4.2f :   %4.1f   : %4.1f |  %4.1f   |\n",
                _ms[offset + MS_A1_TRGR_FLOW_100mlps]/100.,
                _ms[offset + MS_A1_TRGR_PRSR_10bar]/10.,
                _ms[offset + MS_A1_TRGR_MASS_10g]/10.,
                _ms[offset + MS_A1_TIMEOUT_10s]/10.);        
            break;
        }
    case LN_AB_LOW_BOUNDARY:
        printf("\033[%d;1H\033[2K|=|=========================|========================|=========|\n",
        LN_AB_LOW_BOUNDARY + 1);
        break;
    case LN_LOW_BUFF:
        printf("\033[%d;1H\033[2K\n",
        LN_LOW_BUFF + 1);
        break;
    default:
        break;
    }
    printf("\033[%d;1H", LN_COUNT + local_ui_num_ln + 2);
}

int machine_settings_print(){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    printf("\033[2J");
    for(uint8_t ln_num = 0; ln_num < LN_COUNT; ln_num++){
        _machine_settings_print_ln(ln_num);
    }
    return PICO_ERROR_NONE;
}

int machine_settings_print_local_ui(){
    if(_mem == NULL) return PICO_ERROR_GENERIC;

    // Go below settings and delete all content in the lines below
    printf("\033[%d;1H", LN_COUNT + 1 + local_ui_num_ln);
    for(uint8_t i = 0; i < local_ui_num_ln; i++) printf("\033[A\033[2K");
    
    // Print the correct output for the type of folder.
    const bool is_action = local_ui_is_action_folder(settings_modifier.cur_folder);
    if(is_action){
        printf("Adjusting: %s\n", settings_modifier.cur_folder->name);
        for(uint8_t i = 1; i < local_ui_num_ln; i++) printf("\n");
    } else {
        printf("\033[4m%s\033[24m\n", settings_modifier.cur_folder->name);

        // Never print more than local_ui_num_ln-1 folders. End with "..." if some have been cut off.
        uint8_t n_ln = MIN(settings_modifier.cur_folder->num_subfolders, local_ui_num_ln - 1);
        bool overflows = (settings_modifier.cur_folder->num_subfolders > local_ui_num_ln - 1);
        for(uint8_t i = 0; i < n_ln; i++){
            printf(" (%d) %s%s\n", i+1, 
            settings_modifier.cur_folder->subfolders[i]->name,
            (i == n_ln-1 && overflows) ? "..." : "");
        }
    }
    printf("\033[%d;1H", LN_COUNT + local_ui_num_ln + 2);
    return PICO_ERROR_NONE;
}
/** @} */
