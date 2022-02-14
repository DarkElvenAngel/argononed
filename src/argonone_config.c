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

#include "argononed.common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>

#define MAX_LEN 256

/**
 * Initialize the configuration
 * 
 * \param struct DTBO_Data* configuration
 * \return int
 */
int Init_Configuration(struct DTBO_Data* conf)
{
  struct DTBO_Config defaults = { 
      .fanstages = { 10, 55, 100 },
      .thresholds = { 55, 60, 65 },
      .hysteresis = 3
  };
  memcpy(&conf->configuration, &defaults, sizeof(struct DTBO_Config));

  conf->Log_Level = LOG_LEVEL;
  #ifdef RUN_IN_FOREGROUND
  conf->extra.flags.FOREGROUND_MODE = 1; // always true until it's not.
  #endif
  #ifdef USE_SYSFS_TEMP
  conf->extra.flags.USE_SYSFS = 1;
  #endif
  #ifdef DISABLE_POWER_BUTTON_SUPPORT
  conf->extra.flags.PB_DISABLE = 1;
  #endif
  return 0;
}
/**
 * Check the configuration
 * 
 * \param struct DTBO_Data* configuration
 * \param struct DTBO_Data* src)
 * \param int Full_check
 * \return int
 */
int Check_Configuration(struct DTBO_Data* conf, struct DTBO_Config src, int Full_check)
{
    if (src.hysteresis <= 10) conf->configuration.hysteresis = src.hysteresis;
    for (int i = 0; i < 3; i++)
    {
        if (i > 0)
        {
            if (src.fanstages[i] > src.fanstages[i - 1])
                src.fanstages[i] = conf->configuration.fanstages[i] = src.fanstages[i] <= 100 ? src.fanstages[i] : conf->configuration.fanstages[i];
            if (src.thresholds[i] > src.thresholds[i - 1])
                src.thresholds[i] = conf->configuration.thresholds[i] = src.thresholds[i] <= 80 ? src.thresholds[i] : conf->configuration.thresholds[i];
        } else {
            src.fanstages[i]  = conf->configuration.fanstages[i]  = src.fanstages[i] <= 100 ? src.fanstages[i]  : conf->configuration.fanstages[i];
            src.thresholds[i] = conf->configuration.thresholds[i] = src.thresholds[i] <= 80 ? src.thresholds[i] : conf->configuration.thresholds[i];
        }
    }
    if (Full_check == 0) return 0;
    log_message(LOG_INFO + LOG_BOLD,"Checking configuration");
    char devi2c[13];
    snprintf(devi2c, 13, "/dev/i2c-%d", conf->extra.bus);
    if (access(devi2c, F_OK) == -1) log_message(LOG_CRITICAL, "i2c bus at %s inaccessible", devi2c);
    if (!conf->extra.flags.USE_SYSFS && access("/dev/vcio", W_OK + R_OK) == -1) {
        log_message(LOG_WARN, "Temperature sensor at /dev/vcio inaccessible");
        log_message(LOG_INFO + LOG_BOLD, "  Set flag USE_SYSFS as fallback!");
        conf->extra.flags.USE_SYSFS = 1;
    }
    if (conf->extra.flags.USE_SYSFS && access("/sys/class/hwmon/hwmon0/temp1_input", R_OK) == -1)
    {
        log_message(LOG_CRITICAL, "Temperature sensor at /sys/class/hwmon/hwmon0/temp1_input inaccessible");
    }
    if (!conf->extra.flags.PB_DISABLE && access("/dev/gpiochip0", W_OK + R_OK) == -1) {
        log_message(LOG_CRITICAL, "GPIO device /dev/gpiochip0 inaccessible");
        log_message(LOG_INFO + LOG_BOLD, "  Disabling Power Button!");
        conf->extra.flags.PB_DISABLE = 1;
    }
    return 0;
}

/**
 * @brief Send Configuration output to log
 * 
 * \param struct DTBO_Data* configuration
 */
void Configuration_log(struct DTBO_Data* conf)
{  
    log_message(LOG_INFO + LOG_BOLD,"Configuration Data");
    log_message(LOG_INFO,"Device-Tree Overlay version %d.%d.%d",conf->version[0], conf->version[1], conf->version[2]);
    log_message(LOG_INFO,"Hysteresis set to %d",conf->configuration.hysteresis);
    log_message(LOG_INFO,"Fan Speeds set to %d%% %d%% %d%%",conf->configuration.fanstages[0],conf->configuration.fanstages[1],conf->configuration.fanstages[2]);
    log_message(LOG_INFO,"Fan Temps set to %d %d %d",conf->configuration.thresholds[0],conf->configuration.thresholds[1],conf->configuration.thresholds[2]);
    log_message(LOG_INFO,"i2c bus set to /dev/i2c-%d",conf->extra.bus);
    log_message(LOG_INFO,"Flags set to 0x%02X", conf->extra.flags.value); 
    log_message(LOG_DEBUG,"  FLAG Disable Powerbutton %s SET", conf->extra.flags.PB_DISABLE ? "IS" : "NOT");
    log_message(LOG_DEBUG,"  FLAG Forground mode %s SET", conf->extra.flags.FOREGROUND_MODE ? "IS" : "NOT");
    log_message(LOG_DEBUG,"  FLAG Use sysfs for temperature %s SET", conf->extra.flags.USE_SYSFS ? "IS" : "NOT");
    log_message(LOG_DEBUG,"  FLAG Hardware monitor address %x", conf->extra.flags.SET_HWMON_NUM);
    if ( conf->extra.flags.USE_SYSFS)
    {
        log_message(LOG_INFO,"Using /sys/class/hwmon/hwmon%d/temp1_input", conf->extra.flags.SET_HWMON_NUM);
    } else {
        log_message(LOG_INFO,"Using /dev/vcio for temperature");
    }
}

/**
 * Read Device Tree Data
 * 
 * \param struct DTBO_Data* configuration
 * \return int
 */
int Read_DeviceTree_Data(struct DTBO_Data* conf)
{
    FILE *fp = NULL;
    uint32_t ret = 0;
    struct DTBO_Config datain = {0};
    log_message(LOG_INFO,"Reading values from device-tree");
    fp = fopen("/proc/device-tree/argonone/argonone-ver","rb");
    if (fp == NULL)
    {
        conf->version[0] = 0;
        conf->version[1] = 0;
        conf->version[2] = 1;
        conf->extra.bus  = 1;
        log_message(LOG_WARN,"Overlay version 0.0.1 Detected [please update]");
        #ifndef DISABLE_POWERBUTTON
        conf->extra.flags.PB_DISABLE = 1;
        log_message(LOG_INFO,"Disabling Power Button.");
        #endif
    } else {
        ret = fread(&conf->version,sizeof(uint8_t),3,fp);
        if (ret <= 0)
        {
            printf("ERROR : Unable to read device-tree version data\n");
        }
        fclose(fp);
    }
    fp = fopen("/proc/device-tree/argonone/argonone-cfg","rb");
    if (fp == NULL)
    {
        log_message(LOG_WARN,"Unable to open device-tree data");
        return -1;
    } else {
        ret = fread(&datain,sizeof(struct DTBO_Config),1,fp);
        if (ret <= 0)
        {
            log_message(LOG_ERROR,"Unable to read device-tree data");
            return -1;
        } else {
            Check_Configuration(conf, datain, 0);
            if (conf->version[1] > 0)
            {
                fclose(fp);
                fp = fopen("/proc/device-tree/argonone/argonone-extra","rb");
                if (fp == NULL)
                {
                    log_message(LOG_WARN, "Unable to open device-tree data argonone-extra");
                    return 1;
                } else {
                    struct DTO_EXTRA extra;
                    ret = fread(&extra,sizeof(struct DTO_EXTRA),1,fp);
                    conf->extra.bus = extra.bus;
                    conf->extra.flags.value |= extra.flags.value;
                }
            }
        }
        fclose(fp);
    }
    if (access("/proc/device-tree/hat/product", R_OK) == 0)
    {
        fp = fopen("/proc/device-tree/hat/product","rb");
        if (fp)
        {
            char buffer[MAX_LEN];
            fgets(buffer, MAX_LEN, fp);
            if (strcmp(buffer, "Argon Forty Controllable Fan Hat") == 0)
            {
                log_message(LOG_INFO, "Argon Artik Hat Detected");
            } else {
                log_message(LOG_DEBUG, "Hat detected %s", buffer);
            }
            fclose(fp);
        }
    }
    return 0;
}
/** Configuration Files
 */

typedef enum { CFG_FANS, CFG_FAN0, CFG_FAN1, CFG_FAN2,
    CFG_TEMPS, CFG_TEMP0, CFG_TEMP1, CFG_TEMP2,
    CFG_HYSTERESIS, CFG_FLAGS, CFG_BUS, CFG_LOGLEVEL } eConf;

typedef struct Conf_id {
    char    *text;
    eConf   ec;
} Conf_id;
 
Conf_id conf_map[] = {
    { "fans", CFG_FANS },
    { "fan0", CFG_FAN0 },
    { "fan1", CFG_FAN1 }, 
    { "fan2", CFG_FAN2 },
    { "temps", CFG_TEMPS },
    { "temp0", CFG_TEMP0 },
    { "temp1", CFG_TEMP1 },
    { "temp2", CFG_TEMP2 },
    { "hysteresis", CFG_HYSTERESIS },
    { "flags", CFG_FLAGS },
    { "i2cbus", CFG_BUS },
    { "loglevel", CFG_LOGLEVEL },
};

static void strclean(char* str)
{
    char* d = str;
    while((*(d+=!isspace(*str++)) = *str));
}

static int findit(const char text[], int offset) {
    for (size_t i = 0; i < sizeof(conf_map) / sizeof(conf_map[0]); i++) {
        if (strcmp(conf_map[i].text, text) == 0) return conf_map[i].ec;
    }
    log_message(LOG_WARN,"CONF ERROR Unknown property \"%s\" in line %d", text, offset);
    return -1;
}

static int get_vals(char text[], unsigned char* val, int max_elements, int offset)
{ 
    int count = 0;
    char *token = 0;
    token = strtok(text, ",");
    while (token)
    {
        count++;
        if (0 && offset >= 0) printf("ERROR:  Bad value %s line %d\n", token, offset);
        val[count - 1] = (unsigned char)atoi(token);
        token = strtok(NULL, ",");
        if (count >= max_elements) return count;
    }
    return count;
}

/**
 * Read Configuration File
 * 
 * \param filename configuration file's name
 * \param conf pointer to Configuration structer
 * 
 * \return 0 on success
 */
int Read_Configuration_File(const char* filename, struct DTBO_Data* conf)
{
    log_message(LOG_INFO, "Reading %s", filename);
    int ret = 0;
    if ((ret = access(filename, F_OK)))
    {
        log_message(LOG_INFO, "Configuration file [%s] was not found",filename);
        return 0;
    }
    FILE *fp = NULL;
    fp = fopen(filename,"r");
    if (!fp)
    {
        log_message(LOG_ERROR, "Unable to access %s",filename);
        return -1;
    }  
    struct DTBO_Config conf_in = { 0 };
    char buffer[MAX_LEN];
    uint16_t line = 0;
    while (fgets(buffer, MAX_LEN, fp))
    {
        int property;
        line++;
        char key[MAX_LEN], value[MAX_LEN];
        // Remove trailing newline
        buffer[strcspn(buffer, "\n")] = 0;
        // Remove comments
        buffer[strcspn(buffer, "#")] = 0;
        if (strlen(buffer) == 0) continue;
        if (strcspn(buffer, "=") == strlen(buffer))
        {
            log_message(LOG_WARN,"CONF SYNTAX ERROR in line %d", line);
            continue;
        }
        strclean(buffer);
        buffer[strcspn(buffer, "=")] = ' ';
        sscanf(buffer,"%s %s",key, value);
        if ((property = findit(key,line)) == -1) continue;

        switch (property)
        {
            case CFG_FANS:
                get_vals(value, (uint8_t *)&conf_in.fanstages, 3, line);
                break;
            case CFG_FAN0:
                conf_in.fanstages[0] = (uint8_t)atoi(value);
                break;
            case CFG_FAN1:
                conf_in.fanstages[1] = (uint8_t)atoi(value);
                break;
            case CFG_FAN2:
                conf_in.fanstages[2] = (uint8_t)atoi(value);
                break;
            case CFG_TEMPS:
                get_vals(value, (uint8_t *)&conf_in.thresholds, 3, line);
                break;
            case CFG_TEMP0:
                conf_in.thresholds[0] = (uint8_t)atoi(value);
                break;
            case CFG_TEMP1:
                conf_in.thresholds[1] = (uint8_t)atoi(value);
                break;
            case CFG_TEMP2:
                conf_in.thresholds[2] = (uint8_t)atoi(value);
                break;
            case CFG_HYSTERESIS:
                conf_in.hysteresis = (uint8_t)atoi(value);
                break;
            case CFG_FLAGS:
                conf->extra.flags.value |= (uint8_t)strtol(value, NULL, 16);
                break;
            case CFG_BUS:
                conf->extra.bus = (uint8_t)atoi(value);
                break;
            case CFG_LOGLEVEL:
                conf->Log_Level = (uint8_t)atoi(value);
                break;
            default: continue;
        }
    }    
    fclose(fp);
    Check_Configuration(conf, conf_in, 0);
    return 0;
}

/**
 *  Command line Arguments
 */

const char *argp_program_version = "ArgonOne Daemon version " DAEMON_VERSION;
const char *argp_program_bug_address = "<gitlab.com/darkelvenangel/argononed.git>";
static char doc[] = "ArgonOne Daemon";
// static char args_doc[] = "";
static struct argp_option options[] = {
  { "fans",       11,  "VALUE",    0, "Set Fan values"                  ,0 },
  { "fan0",       1,   "VALUE",    0, "Set Fan1 value"                  ,0 },
  { "fan1",       2,   "VALUE",    0, "Set Fan2 value"                  ,0 },
  { "fan2",       3,   "VALUE",    0, "Set Fan3 value"                  ,0 },
  { "temps",      12,  "VALUE",    0, "Set Temperature values"          ,0 },
  { "temp0",      4,   "VALUE",    0, "Set Temperature1 value"          ,0 },
  { "temp1",      5,   "VALUE",    0, "Set Temperature2 value"          ,0 },
  { "temp2",      6,   "VALUE",    0, "Set Temperature3 value"          ,0 },
  { "hysteresis", 7,   "VALUE",    0, "Set Hysteresis"                  ,0 },
  { "conf",       8,   "FILENAME", 0, "load config"                     ,1 },
  { "forground",  'F', 0,          0, "Run in Forground"                ,1 },
  { "loglevel",   'l', "VALUE",    0, "Set Log level"                   ,1 },
  { "forceflag",  9,   "VALUE",    0, "Force flags to VALUE"            ,1 },
  { "dumpconf",   10,  0,          0, "Dump build config"               ,2 },
  { "colour",     'c', 0,          0, "Run in Forground with colour"    ,2 },
  { 0 }
};

extern const char* LOG_LEVEL_STR[7];
/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct DTBO_Data *config = state->input;
  switch (key)
  {
    case 'c':
      config->colour = 1;
      config->extra.flags.FOREGROUND_MODE = 1;
      break;
    case 'F':
      config->extra.flags.FOREGROUND_MODE = 1;
      break;
    case 'l':
      config->Log_Level = (uint8_t)atoi(arg);
      printf("Loglevel set to %s\n", LOG_LEVEL_STR[config->Log_Level]);
      break;
    case 1:
      config->configuration.fanstages[0] = (uint8_t)atoi(arg);
      break; 
    case 2:
      config->configuration.fanstages[1] = (uint8_t)atoi(arg);
      break; 
    case 3:
      config->configuration.fanstages[2] = (uint8_t)atoi(arg);
      break; 
    case 4:
      config->configuration.thresholds[0] = (uint8_t)atoi(arg);
      break;
    case 5:
      config->configuration.thresholds[1] = (uint8_t)atoi(arg);
      break;
    case 6:
      config->configuration.thresholds[2] = (uint8_t)atoi(arg);
      break;
    case 7:
      config->configuration.hysteresis = (uint8_t)atoi(arg);
      break;
    case 8:
      config->filename = arg;
      break;
    case 9:
      config->extra.flags.value = (uint8_t)atoi(arg);
      break;
    case 10:
    {
      struct DTBO_Data conf = {0};
      printf ("%s\n config\n",argp_program_version);
      Init_Configuration(&conf);
      printf ("LOG_LEVEL = %d [ %s ] ", LOG_LEVEL, LOG_LEVEL_STR[LOG_LEVEL]);
      if (conf.extra.flags.PB_DISABLE) printf ("DISABLE_POWERBUTTON ");
      if (conf.extra.flags.FOREGROUND_MODE) printf ("RUN_IN_FOREGROUND ");
      if (conf.extra.flags.USE_SYSFS) printf ("USE_SYSFS_TEMP ");
      printf ("\n");    
      exit (0);
      break;
    }
    case 11:
        get_vals(arg, (uint8_t *)&config->configuration.fanstages, 3, 0);
        break;
    case 12:
        get_vals(arg, (uint8_t *)&config->configuration.thresholds, 3, 0);
        break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 2)
      {
        fprintf(stderr, "ERROR:  Bad Argument");
        /* Too many arguments. */
        argp_usage (state);
      }
      break;
    case ARGP_KEY_END: break;
    default: return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
/* Our argp parser. */
static struct argp argp = { options, parse_opt, NULL, doc, 0, 0, 0 };


/**
 * Parse Command Line Arguments
 * 
 * \param argc
 * \param argv
 * \param conf pointer to Configuration structer
 * 
 * \return 0 on success
 */
int Parse_Command_Line_Arguments(int argc, char **argv, struct DTBO_Data* args, struct DTBO_Data* conf)
{
    memcpy(args, conf, sizeof(struct DTBO_Data));
    memset(&args->configuration, 255, sizeof(struct DTBO_Config));
    if (argc == 1) return 0;
    return argp_parse (&argp, argc, argv, 0, 0, args);
}
