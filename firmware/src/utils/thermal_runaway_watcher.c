/**
 * \file thermal_runaway_watcher.c
 * \ingroup thermal_runaway_watcher
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Thermal Runaway Watcher source
 * \version 0.1
 * \date 2023-04-28
 */

#include <stdlib.h>
#include "pico/time.h"

#include "utils/thermal_runaway_watcher.h"
#include "config/raspberry_latte_config.h"

typedef struct thermal_runaway_watcher_s {
    thermal_runaway_state state;   /**< \brief The state of the watcher. */
    int16_t setpoint;             /**< \brief The latest setpoint of the system. */
    int16_t temp;                 /**< \brief The latest temp of the system. */
    int16_t temp_max_change;      /**< \brief The maximum the temp can change in a single timestep. */
    int16_t temp_convergence_tol; /**< \brief Range around setpoint that counts as converged. */
    int16_t temp_divergence_limit;/**< \brief Range around setpoint that must be respected while converged. */
    int16_t min_temp_change_heat; /**< \brief The minimum increase in temp during heating within duration. */
    int16_t min_temp_change_cool; /**< \brief The minimum decrease in temp during cooling within duration. */
    uint32_t min_temp_change_time; /**< \brief The duration for the temp to satisfy it's min-response constraint. */
    int16_t temp_change_target;   /**< \brief The target the temperature is trying to cross. */       
    absolute_time_t temp_change_timer_end; /**< \brief End of temp-change window*/  
} thermal_runaway_watcher_;

static inline bool _temp_change_timer_expired(thermal_runaway_watcher trw){
    return absolute_time_diff_us(get_absolute_time(), trw->temp_change_timer_end) <= 0;
}

thermal_runaway_watcher thermal_runaway_watcher_setup(uint16_t temp_max_change,
    uint16_t temp_convergence_tol, uint16_t temp_divergence_limit, uint16_t min_temp_change_heat, 
    uint16_t min_temp_change_cool, uint32_t min_temp_change_time_ms){
    thermal_runaway_watcher trw = malloc(sizeof(thermal_runaway_watcher_));
    trw->state = TRS_OFF;
    trw->temp_max_change       = temp_max_change;
    trw->temp_convergence_tol  = temp_convergence_tol;
    trw->temp_divergence_limit = temp_divergence_limit;
    trw->min_temp_change_heat  = min_temp_change_heat;
    trw->min_temp_change_cool  = min_temp_change_cool;
    trw->min_temp_change_time  = min_temp_change_time_ms;
    return trw;
}

thermal_runaway_state thermal_runaway_watcher_tick(thermal_runaway_watcher trw, uint16_t setpoint, int16_t temp){
    if (setpoint == 0){
        trw->state = TRS_OFF;
    } 
    // If not in error state
    else if (trw->state >= 0){
        if (trw->setpoint == 0){
            // When machine is switched on, go ahead and manually set temp
            trw->temp = temp;
        }
        if ((trw->state == TRS_HEATING || trw->state == TRS_COOLING) && _temp_change_timer_expired(trw)){
            // Passed timeout while heating or cooling
            trw->state = TRS_ERROR_FAILED_TO_CONVERGE;
        }
        else if ((trw->temp < temp ? temp - trw->temp : trw->temp - temp) > trw->temp_max_change){
            // Large jump in temp. May indicate an sensor error
            trw->state = TRS_ERROR_LARGE_TEMP_JUMP;
        }
        else if (  (trw->setpoint != setpoint)
                 ||(trw->state == TRS_HEATING && temp >= trw->temp_change_target)
                 ||(trw->state == TRS_COOLING && temp <= trw->temp_change_target)){
            if (trw->setpoint != setpoint){
                trw->state = (temp < setpoint ? TRS_HEATING : TRS_COOLING);
            }
            if (trw->state == TRS_HEATING){
                trw->temp_change_target = temp + trw->min_temp_change_heat;
            } else if (trw->state == TRS_COOLING){
                trw->temp_change_target = temp - trw->min_temp_change_cool;
            }
            trw->temp_change_timer_end = make_timeout_time_ms(trw->min_temp_change_time);
        }
        if (trw->state == TRS_HEATING && temp >= setpoint - trw->temp_convergence_tol){
            // Heated up to setpoint
            trw->state = TRS_CONVERGED;
        } 
        else if (trw->state == TRS_COOLING && temp <= setpoint + trw->temp_convergence_tol){
            // Cooled down to setpoint
            trw->state = TRS_CONVERGED;
        } 
        else if (trw->state == TRS_CONVERGED){
            // If in converged state
            if (temp < setpoint - trw->temp_divergence_limit || temp > setpoint + trw->temp_divergence_limit){
                trw->state = TRS_ERROR_DIVERGED;
            }
        }
    }

    trw->setpoint = setpoint;
    trw->temp = temp;
    return trw->state;
}

thermal_runaway_state thermal_runaway_watcher_state(thermal_runaway_watcher trw){
    return trw->state;
}

void thermal_runaway_watcher_deinit(thermal_runaway_watcher trw){
    free(trw);
}

#ifdef THERMAL_RUNAWAY_WATCHER_TESTS
#include <stdio.h>
#define NUM_TEST_POINTS 32

typedef struct {
    int16_t temp;
    uint16_t setpoint;
    uint32_t delay;
    thermal_runaway_state state;
} thermal_runaway_watcher_test_point;

void thermal_runaway_watcher_test(){
    thermal_runaway_watcher trw = thermal_runaway_watcher_setup(10, 2, 10, 4, 2, 1000);
    thermal_runaway_watcher_test_point tp [NUM_TEST_POINTS] = {
/*1*/   {.delay =   0, .setpoint =  0, .temp = 23, .state = TRS_OFF},
/*2*/   {.delay =   0, .setpoint =  0, .temp = 25, .state = TRS_OFF},
/*3*/   {.delay =   0, .setpoint =  0, .temp = 65, .state = TRS_OFF},
/*4*/   {.delay =   0, .setpoint =  0, .temp =  0, .state = TRS_OFF},
/*5*/   {.delay =   0, .setpoint =  0, .temp = 23, .state = TRS_OFF},
/*6*/   {.delay =   0, .setpoint = 95, .temp = 23, .state = TRS_HEATING},
/*7*/   {.delay = 100, .setpoint = 95, .temp = 25, .state = TRS_HEATING},
/*8*/   {.delay = 950, .setpoint = 95, .temp = 25, .state = TRS_ERROR_FAILED_TO_CONVERGE},
/*9*/   {.delay =   0, .setpoint = 95, .temp = 30, .state = TRS_ERROR_FAILED_TO_CONVERGE},
/*10*/  {.delay =   0, .setpoint =  0, .temp = 30, .state = TRS_OFF},
/*11*/  {.delay =   0, .setpoint = 95, .temp = 23, .state = TRS_HEATING},
/*12*/  {.delay = 500, .setpoint = 95, .temp = 25, .state = TRS_HEATING},
/*13*/  {.delay = 450, .setpoint = 95, .temp = 27, .state = TRS_HEATING},
/*14*/  {.delay = 100, .setpoint = 95, .temp = 25, .state = TRS_HEATING},
/*15*/  {.delay = 100, .setpoint = 95, .temp = 94, .state = TRS_ERROR_LARGE_TEMP_JUMP},
/*16*/  {.delay =   0, .setpoint =  0, .temp = 94, .state = TRS_OFF},
/*17*/  {.delay =   0, .setpoint = 95, .temp = 94, .state = TRS_CONVERGED},
/*18*/  {.delay =1100, .setpoint = 95, .temp = 94, .state = TRS_CONVERGED},
/*19*/  {.delay =   0, .setpoint = 95, .temp = 96, .state = TRS_CONVERGED},
/*20*/  {.delay =   0, .setpoint = 95, .temp = 96, .state = TRS_CONVERGED},
/*21*/  {.delay =   0, .setpoint = 95, .temp = 90, .state = TRS_CONVERGED},
/*22*/  {.delay =   0, .setpoint = 95, .temp = 84, .state = TRS_ERROR_DIVERGED},
/*23*/  {.delay =   0, .setpoint =  0, .temp = 84, .state = TRS_OFF},
/*24*/  {.delay =   0, .setpoint = 85, .temp = 84, .state = TRS_CONVERGED},
/*25*/  {.delay =   0, .setpoint = 75, .temp = 84, .state = TRS_COOLING},
/*26*/  {.delay =   0, .setpoint = 75, .temp = 83, .state = TRS_COOLING},
/*27*/  {.delay =1010, .setpoint = 75, .temp = 83, .state = TRS_ERROR_FAILED_TO_CONVERGE},
/*28*/  {.delay =   0, .setpoint =  0, .temp = 83, .state = TRS_OFF},
/*29*/  {.delay =   0, .setpoint = 75, .temp = 83, .state = TRS_COOLING},
/*30*/  {.delay = 900, .setpoint = 75, .temp = 80, .state = TRS_COOLING},
/*31*/  {.delay = 150, .setpoint = 75, .temp = 80, .state = TRS_COOLING},
/*32*/  {.delay =   0, .setpoint = 75, .temp = 77, .state = TRS_CONVERGED}
    };

    for(uint8_t i = 0; i < NUM_TEST_POINTS; i++){
        sleep_ms(tp[i].delay);
        thermal_runaway_watcher_tick(trw, tp[i].setpoint, tp[i].temp);
        if(trw->state == tp[i].state){
            printf("Test %d:  PASS\n", i+1);
        } else {
            printf("Test %d:  FAIL (%d != %d)\n", i+1, trw->state, tp[i].state);
        }
    }
}
#endif