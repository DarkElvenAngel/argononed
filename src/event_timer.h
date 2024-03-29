#ifndef EVENT_TIME_H
#define EVENT_TIME_H
#include <stdlib.h>
#include <time.h>

typedef enum
{
TIMER_SINGLE_SHOT = 0,
TIMER_PERIODIC
} tmr_types;

typedef void (*time_handler)(size_t timer_id, void * user_data);

/**
 * @brief Initialize timers
 * 
 * @return 0 on success
 */
int     initialize_timers();
/**
 * \brief Start a new timer
 * 
 * \param interval how often to run in seconds
 * \param handler Function to run
 * \param type Set to single shot or periodic
 * \param user_data pointer to data to pass to function
 * \return 0 on fault or timer id
 */
size_t  start_timer_long(time_t interval, time_handler handler, tmr_types type, void * user_data);
/**
 * \brief Start a new timer
 * 
 * \param interval how often to run in miliseconds
 * \param handler Function to run
 * \param type Set to single shot or periodic
 * \param user_data pointer to data to pass to function
 * \return 0 on fault or timer id
 */
size_t  start_timer(time_t interval, time_handler handler, tmr_types type, void * user_data);
/**
 * \brief Stop a running timer
 * 
 * \param timer_id the timer id returned by start timer function
 * \return none
 */
void    stop_timer(size_t timer_id);
/**
 * \brief Stop all timers 
 * 
 * \return none
 */
void    close_timers();

#endif
