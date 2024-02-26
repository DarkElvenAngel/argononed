/*
MIT License

Copyright (c) 2020 DarkElvenAngel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#define ARGONONED
#include <stdint.h>
#include "argononed.common.h"
#include "argonone_shm.h"
#include "event_timer.h"

extern struct SHM_Data* ptr;
extern Daemon_Conf Configuration;
const char* RUN_STATE_STR[4] = {"AUTO", "OFF", "MANUAL", "COOLDOWN"};

struct SHM_Reset{
    size_t  timer;
    uint8_t msg_index;
    uint8_t *status;
};
// #define DISABLE_LEGACY_IPC
int argonon_shm_start()
{
    static int index[3]= {0,1,2};
    log_message(LOG_INFO + LOG_BOLD, "Initalizing IPC Channels");
#ifdef DISABLE_LEGACY_IPC
    start_timer(100,reset_shm,TIMER_PERIODIC,NULL);
#else
    reset_shm();
#endif
    start_timer(100,TMR_SHM_Interface,TIMER_PERIODIC,&index[0]);
    start_timer(100,TMR_SHM_Interface,TIMER_PERIODIC,&index[1]);
    start_timer(100,TMR_SHM_Interface,TIMER_PERIODIC,&index[2]);
#ifdef DISABLE_LEGACY_IPC
    ptr->status = REQ_HOLD;
#else
    start_timer(100,TMR_Legacy_Interface,TIMER_PERIODIC,NULL);
#endif
    return 0;
}

/**
 * Reset Shared Memory to match correct values
 * 
 * \return none
 */
void reset_shm()
{
    memcpy(ptr->config.fanstages, 
        &Configuration.configuration.fanstages, 
        sizeof(Configuration.configuration.fanstages)
        );
    memcpy(ptr->config.thresholds,
        &Configuration.configuration.thresholds,
        sizeof(Configuration.configuration.thresholds)
        );
    ptr->config.hysteresis = Configuration.configuration.hysteresis;
    ptr->fanmode = Configuration.runstate;
}

/**
 * Reload the configuration from shared memory
 * 
 * \return 0 on success
 */
int reload_config_from_shm()
{
    for (int i = 0; i < 3; i++)
    {
        int lastval = 30;
        if (ptr->config.thresholds[i] < lastval)
        {
            log_message(LOG_WARN,"Shared Memory contains bad value at threshold %d ABORTING reload", i);
            reset_shm();
            return -1;
        }
    }
    for (int i = 0; i < 3; i++)
    {
        if (ptr->config.fanstages[i] > 100 )
        {
            log_message(LOG_WARN,"Shared Memory contains bad value at fanstage %d ABORTING reload", i);
            reset_shm();
            return -1;
        }
    }
    memcpy(&Configuration.configuration.fanstages,
        ptr->config.fanstages,
        sizeof(Configuration.configuration.fanstages)
        );
    memcpy(&Configuration.configuration.thresholds,
        ptr->config.thresholds,
        sizeof(Configuration.configuration.thresholds)
        );
    if (ptr->config.hysteresis > 10)
    {
        log_message(LOG_WARN,"Shared Memory contains bad value at hysteresis FORCING to 10");
        ptr->config.hysteresis = 10;
    }
    Configuration.configuration.hysteresis = ptr->config.hysteresis;
    if (ptr->fanmode > 3) 
    {
        log_message(LOG_WARN,"Shared Memory contains bad value at fanmode FORCING to AUTO");
        ptr->fanmode = 0;
    }
    Configuration.runstate = ptr->fanmode;
    Configuration.temperature_target = ptr->temperature_target >= 30 ? ptr->temperature_target : 30;
    Configuration.fanspeed_Overide   = ptr->fanspeed_Overide <= 100 ? ptr->fanspeed_Overide : 0;
    Configuration_log(&Configuration);
    //log_message(LOG_INFO,"Hysteresis set to %d",hysteresis);
    //log_message(LOG_INFO,"Fan Speeds set to %d%% %d%% %d%%",fanstage[0],fanstage[1],fanstage[2]);
    //log_message(LOG_INFO,"Fan Temps set to %d %d %d",threshold[0],threshold[1],threshold[2]);
    log_message(LOG_INFO,"Fan Mode [ %s ] ", RUN_STATE_STR[Configuration.runstate]);
    log_message(LOG_INFO,"Fan Speed Override %3d ", Configuration.fanspeed_Overide);
    log_message(LOG_INFO,"Target Temperature %d ", Configuration.temperature_target);   
    return 0;
}
/**
 * Load new schedule from IPC Message
 * 
 * \param config settings to apply
 * \return 0 on success
 */
int load_schedule(Schedule config)
{
    log_message(LOG_INFO, "load schedule"); 
    for (int i = 0; i < 3; i++)
    {
        int lastval = 30;
        if (config.thresholds[i] < lastval)
        {
            log_message(LOG_WARN,"Shared Memory contains bad value at threshold %d ABORTING reload", i);
            return -1;
        }
    }
    for (int i = 0; i < 3; i++)
    {
        if (config.fanstages[i] > 100 )
        {
            log_message(LOG_WARN,"Shared Memory contains bad value at fanstage %d ABORTING reload", i);
            return -1;
        }
    }
    memcpy(&Configuration.configuration.fanstages, 
        config.fanstages, 
        sizeof(Configuration.configuration.fanstages)
        );
    memcpy(&Configuration.configuration.thresholds, 
        config.thresholds, 
        sizeof(Configuration.configuration.thresholds)
        );
    if (Configuration.configuration.hysteresis > 10)
    {
        log_message(LOG_WARN,"Shared Memory contains bad value at hysteresis FORCING to 10");
        config.hysteresis = 10;
    }
    Configuration.configuration.hysteresis = config.hysteresis;
    //log_message(LOG_INFO,"Hysteresis set to %d",hysteresis);
    //log_message(LOG_INFO,"Fan Speeds set to %d%% %d%% %d%%",fanstage[0],fanstage[1],fanstage[2]);
    //log_message(LOG_INFO,"Fan Temps set to %d %d %d",threshold[0],threshold[1],threshold[2]);
    reset_shm();
    return 0;
}
/**
 * Change Mode
 * 
 * \param fanmode
 * \param temperature_target
 * \param fanspeed_Overide
 * \return 0 on success
 */
int Change_mode(uint8_t fanmode, uint8_t temperature_target, uint8_t fanspeed_Overide)
{
    if (fanmode > 3) 
    {
        log_message(LOG_WARN,"Shared Memory contains bad value at fanmode FORCING to AUTO");
        return 1;
    }
    if (fanmode == 3 && temperature_target >= ptr->temperature )
    {
        log_message(LOG_WARN,"Cannot execute cool down when target temperature is great than CPU Temperature");
        return 1;
    }
    Configuration.temperature_target = temperature_target >= 30 ? temperature_target : 30;
    Configuration.fanspeed_Overide   = fanspeed_Overide <= 100 ? fanspeed_Overide : 100;
    Configuration.runstate = fanmode;
    log_message(LOG_INFO,"Fan Mode [ %s ] ", RUN_STATE_STR[Configuration.runstate]);
    log_message(LOG_INFO,"Fan Speed Override %3d ", Configuration.fanspeed_Overide);
    log_message(LOG_INFO,"Target Temperature %d ", Configuration.temperature_target);
    reset_shm();
    return 0;
}

/**
 * Reset Shared Memory
 * 
 * \note This is meant to be called with a Timer.
 * \param timer_id calling timer id
 * \param user_data pointer to argument data
 * \return none
 */
void TMR_SHM_Reset(size_t timer_id, void *user_data)
{
    struct SHM_Reset *timer_rst = (struct SHM_Reset*)user_data;
    log_message(LOG_INFO,"IPC [%d] Reset error condition", timer_rst->msg_index);
    *timer_rst->status = REQ_WAIT;
    stop_timer(timer_id);
    timer_rst->timer = 0;
}
/**
 * Monitor Shared Memory for commands
 * 
 * \note This is meant to be called with a Timer.
 * \param timer_id calling timer id
 * \param user_data pointer to argument data
 * \return none
 */
void TMR_SHM_Interface(size_t timer_id __attribute__((unused)), void *message_index)
{
    static struct SHM_Reset timer_rst[3] = {0};
    static uint8_t last_state[3] = { 0 };
    int index = *(int*)message_index;
    struct SHM_REQ_MSG *message = &ptr->msg_app[index];
    uint8_t *status = &message->status;
    uint8_t *req_flags = &message->req_flags;
    if (*status == REQ_ERR && timer_rst[index].timer == 0)
    {
        log_message(LOG_DEBUG,"IPC Start timer to reset error flag");
        timer_rst[index].msg_index = index;
        timer_rst[index].status = status;
        timer_rst[index].timer = start_timer_long(1, TMR_SHM_Reset, TIMER_SINGLE_SHOT, &timer_rst[index]);
        return;
    }
    if (*status != REQ_WAIT)
        switch (*status)
        {
            case REQ_ERR: // Last command was error wait for reset
                if (last_state[index] == *status) break;
                last_state[index] = *status;
                log_message(LOG_DEBUG,"IPC [%d] ERROR",index);
                break;
            case REQ_RDY: // A shared memory command is ready for processing
                *status = REQ_PEND;
                log_message (LOG_INFO, "IPC message request [%d:%02X]",index, message->req_flags);
                //if (reload_config_from_shm() == 0)
                //{
                if (*req_flags & REQ_FLAG_CONF)
                {
                    if (load_schedule(message->Schedules) == 0)
                    {
                        memset(message, 0, sizeof(struct SHM_REQ_MSG));
                        *status = REQ_WAIT;
                        return;
                    }
                }
                if (*req_flags & REQ_FLAG_MODE)
                {
                    log_message(LOG_DEBUG,"IPC [%d] CHANGE MODE %s FAN %d TEMP %d",
                        index,
                        RUN_STATE_STR[message->fanmode],
                        message->fanspeed_Overide,
                        message->temperature_target
                    );
                    if (Change_mode(message->fanmode,message->temperature_target,message->fanspeed_Overide) == 0)
                    {
                        memset(message, 0, sizeof(struct SHM_REQ_MSG));
                        *status = REQ_WAIT;
                        return;
                    }
                }
                *status = REQ_ERR;
                break;
            case REQ_CLR: // The request area and reset shared memory
                log_message (LOG_DEBUG, "IPC [%d] Reset message", index);
                memset(message, 0, sizeof(struct SHM_REQ_MSG));
                *status = REQ_WAIT;
                return;
            default:
                if (last_state[index] == *status) return;
                last_state[index] = *status;
                log_message (LOG_DEBUG, "IPC [%d] Unknown Status %20X", index, *status);
        }
}

/**
 * Monitor Shared Memory for commands
 * 
 * \note This is meant to be called with a Timer.
 * \param timer_id calling timer id
 * \param user_data pointer to argument data
 * \return none
 */
void TMR_Legacy_Interface(size_t timer_id __attribute__((unused)), void *user_data __attribute__((unused)))
{
    static struct SHM_Reset timer_rst = {0};
    static uint8_t last_state = 0;
    if (ptr->status == REQ_ERR && timer_rst.timer == 0)
    {
        log_message(LOG_DEBUG,"IPC [Legacy] Error");
        timer_rst.msg_index = 4;
        timer_rst.status = &ptr->status;
        timer_rst.timer = start_timer_long(1, TMR_SHM_Reset, TIMER_SINGLE_SHOT, &timer_rst);
        return;
    }
    if (ptr->status != REQ_WAIT)
        switch (ptr->status)
        {
            case REQ_ERR: // Last command was error wait for reset
                break;
            case REQ_RDY: // A shared memory command is ready for processing
#if 1
                ptr->status = REQ_PEND;
                log_message (LOG_INFO + LOG_BOLD, "IPC [ Legacy ] Request reload of config");
                if (reload_config_from_shm() == 0)
                {
                    ptr->status = REQ_WAIT;
                    return;
                }
#endif
                ptr->status = REQ_ERR;
                break;
            case REQ_CLR: // The request area and reset shared memory
                reset_shm();
                ptr->status = REQ_WAIT;
                return;
            default:
                if (last_state == ptr->status) return;
                last_state = ptr->status;
                log_message (LOG_DEBUG, "SHM Unknown Status %20X", ptr->status);
        }
}