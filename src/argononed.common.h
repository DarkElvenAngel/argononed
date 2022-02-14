/*
MIT License

Copyright (c) 2022 DarkElvenAngel

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

#ifndef ARGONONED_H
#define ARGONONED_H

#include <stdint.h>

#define DAEMON_VERSION "0.4.0"

#define RUNNING_DIR "/tmp"

#ifndef LOG_FILE 
#define LOG_FILE "/var/log/argononed.log"
#endif

#ifndef LOCK_FILE
#define LOCK_FILE "/run/argononed.pid"
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL 6
#endif

#ifndef SHM_FILE
#define SHM_FILE "argonone"
#endif
#define SHM_SIZE 512

typedef enum {
    LOG_NONE = 0,
    LOG_FATAL,
    LOG_CRITICAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_BOLD = 8,
    LOG_INVERT = 16
} Log_Level;

typedef struct DTBO_Config {
    uint8_t fanstages[3];
    uint8_t thresholds[3];
    uint8_t hysteresis;
} Schedule;

struct SHM_DAEMON_STATS {
    uint8_t max_temperature;
    uint8_t min_temperature;
    uint8_t EF_Warning;
    uint8_t EF_Error;
    uint8_t EF_Critical;
};

struct DTO_FLAGS{
    union {
        struct {
            uint8_t PB_DISABLE      : 1;
            uint8_t FOREGROUND_MODE : 1;
            uint8_t USE_SYSFS       : 1;
            uint8_t SET_HWMON_NUM   : 1;
        };
        uint8_t value;
    };
};

struct DTO_EXTRA{
    uint8_t bus;
    struct DTO_FLAGS flags;
}; 

typedef struct DTBO_Data{
    uint8_t             version[3];
    uint8_t             Log_Level;
    Schedule            configuration;
    struct DTO_EXTRA    extra;
    char*               filename;
    uint8_t             colour;
    uint8_t             runstate;
    uint8_t             temperature_target;
    uint8_t             fanspeed_Overide;
} Daemon_Conf;

struct SHM_REQ_MSG {
    uint8_t req_flags;
    Schedule Schedules;
    uint8_t fanmode;
    uint8_t temperature_target;
    uint8_t fanspeed_Overide;
    uint8_t status;
};

#define REQ_WAIT 0              // Waiting for request 
#define REQ_RDY  1              // Request is ready for processing
#define REQ_PEND 2              // Request pending
#define REQ_ERR  3              // Error in last Request
#define REQ_SYNC 4              // Request Status to sync
#define REQ_CLR  5              // Clear request
#define REQ_RST  6              // Request Daemon to reset 
#define REQ_HOLD 7              // Hold Requests
#define REQ_OFF  8              // Request Daemon to shutdown
#define REQ_SIG  9              // Request Commit Signal

#define REQ_FLAG_MODE   0x01    // Request Mode change
#define REQ_FLAG_CONF   0x02    // Request Config change
#define REQ_FLAG_CMD    0x04    // Request Command
#define REQ_FLAG_STAT   0x08    // Request Statistics Reset *REQ_CLR only 

struct SHM_Data {               //  DAEMON  |   CLIENT
    uint8_t fanspeed;           //      WO  |   RO
    uint8_t temperature;        //      WO  |   RO
    struct DTBO_Config config;  //      RW  |   RW
    uint8_t fanmode;            //      RW  |   RW
    uint8_t temperature_target; //      RW  |   RW
    uint8_t fanspeed_Overide;   //      RO  |   RW
    uint8_t status;             //      RW  |   RW
    // uint8_t req_flags;          //      RW  |   WO
    struct SHM_DAEMON_STATS stat;
    //struct SHM_REQ_MSG msg; // Special Message for CLI client only
    struct SHM_REQ_MSG msg_app[3]; // Normal Application Messages **Not Yet Enabled**
}; // current size - 14 bytes

/**
 * Write formatted output to Log file.
 * 
 * \param level Message's Log level
 * \param message formatted text to output
 * \return none
 */
void log_message(Log_Level level, const char *message, ...);
/**
 * Initialize the configuration
 * 
 * \param struct DTBO_Data* configuration
 * \return int
 */
int Init_Configuration(struct DTBO_Data* conf);
/**
 * @brief Send Configuration output to log
 * 
 * \param struct DTBO_Data* configuration
 */
void Configuration_log(struct DTBO_Data* conf);
/**
 * Check the configuration
 * 
 * \param struct DTBO_Data* configuration
 * \param struct DTBO_Data* src)
 * \param int Full_check
 * \return int
 */
int Check_Configuration(struct DTBO_Data* conf, struct DTBO_Config src, int Full_check);
/**
 * Read Device Tree Data
 * 
 * \param struct DTBO_Data* configuration
 * \return int
 */
int Read_DeviceTree_Data(struct DTBO_Data* conf);
/**
 * Read Configuration File
 * 
 * \param filename configuration file's name
 * \param conf pointer to Configuration structer
 * 
 * \return 0 on success
 */
int Read_Configuration_File(const char* filename, struct DTBO_Data* conf);
/**
 * Parse Command Line Arguments
 * 
 * \param argc
 * \param argv
 * \param conf pointer to Configuration structer
 * 
 * \return 0 on success
 */
int Parse_Command_Line_Arguments(int argc, char **argv, struct DTBO_Data* args, struct DTBO_Data* conf);
#endif
