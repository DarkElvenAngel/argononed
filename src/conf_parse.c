#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "argononed.common.h"

struct DTBO_Data conf, Configuration;
struct SHM_Data* ptr;
#define MAX_LEN 256
 
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

void strclean(char* str)
{
    char* d = str;
    while(*(d+=!isspace(*str++)) = *str);
}

int findit(const char text[], int offset) {
    for (size_t i = 0; i < sizeof(conf_map) / sizeof(conf_map[0]); i++) {
        if (strcmp(conf_map[i].text, text) == 0) return conf_map[i].ec;
    }
    printf("WARNING:  CONFIG ERROR Unknown property %s line %d\n", text, offset);
    return -1;
}

int get_vals(char text[], unsigned char* val, int max_elements, int offset)
{ 
    int count = 0;
    char *token = 0;
    token = strtok(text, ",");
    while (token)
    {
        count++;
        //if ( ) printf("ERROR:  Bad value %s line %d\n", token, offset);
        val[count - 1] = (unsigned char)atoi(token);
        token = strtok(NULL, ",");
        if (count >= max_elements) return count;
    }
    return count;
}

int main(void)
{
    FILE* fp;
    fp = fopen("argononed.conf", "r");
    if (fp == NULL) {
      perror("Failed: ");
      return 1;
    }
    Init_Configuration(&conf);
    struct DTBO_Config conf_in = { 0 };
    char buffer[MAX_LEN];
    int line = 0;
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
            printf("WARNING:  CONFIG SYNTAX ERROR:  line %d\n", line);
            continue;
        }
        strclean(buffer);
        buffer[strcspn(buffer, "=")] = ' ';
        sscanf(buffer,"%s %s",key, value);
        if ((property = findit(key,line)) == -1) continue;

        switch (property)
        {
            case CFG_FANS:
                get_vals(value, (unsigned char *)&conf_in.fanstages, 3, line);
                // printf("%s",value);
                break;
            case CFG_FAN0:
                conf_in.fanstages[0] = atoi(value);
                break;
            case CFG_FAN1:
                conf_in.fanstages[1] = atoi(value);
                break;
            case CFG_FAN2:
                conf_in.fanstages[2] = atoi(value);
                break;
            case CFG_TEMPS:
                get_vals(value, (unsigned char *)&conf_in.thresholds, 3, line);
                break;
            case CFG_TEMP0:
                conf_in.thresholds[0] = atoi(value);
                break;
            case CFG_TEMP1:
                conf_in.thresholds[1] = atoi(value);
                break;
            case CFG_TEMP2:
                conf_in.thresholds[2] = atoi(value);
                break;
            case CFG_HYSTERESIS:
                conf_in.hysteresis = atoi(value);
                break;
            case CFG_FLAGS:
                conf.extra.flags.value |= strtol(value, NULL, 16);
                break;
            case CFG_BUS:
                conf.extra.bus = atoi(value);
                break;
            case CFG_LOGLEVEL:
                conf.Log_Level = atoi(value);
                break;
            default: continue;
        }
    }
    fclose(fp);

    if (conf_in.hysteresis <= 10) conf.configuration.hysteresis = conf_in.hysteresis;
    for (int i = 0; i < 3; i++)
    {
        if (i > 0)
        {
            if (conf_in.fanstages[i] > conf_in.fanstages[i - 1])
                conf_in.fanstages[i] = conf.configuration.fanstages[i] = conf_in.fanstages[i] <= 100 ? conf_in.fanstages[i] : conf.configuration.fanstages[i];
            if (conf_in.thresholds[i] > conf_in.thresholds[i - 1])
                conf_in.thresholds[i] = conf.configuration.thresholds[i] = conf_in.thresholds[i] <= 80 ? conf_in.thresholds[i] : conf.configuration.thresholds[i];
        } else {
            conf_in.fanstages[i]  = conf.configuration.fanstages[i]  = conf_in.fanstages[i] <= 100 ? conf_in.fanstages[i]  : conf.configuration.fanstages[i];
            conf_in.thresholds[i] = conf.configuration.thresholds[i] = conf_in.thresholds[i] <= 80 ? conf_in.thresholds[i] : conf.configuration.thresholds[i];
        }
    }

    printf("\n");
    printf("Hysteresis set to %d\n",conf.configuration.hysteresis);
    printf("Fan Speeds set to %d%% %d%% %d%%\n",conf.configuration.fanstages[0],conf.configuration.fanstages[1],conf.configuration.fanstages[2]);
    printf("Fan Temps set to %d %d %d\n",conf.configuration.thresholds[0],conf.configuration.thresholds[1],conf.configuration.thresholds[2]);
    printf("i2c bus set to /dev/i2c-%d\n",conf.extra.bus);
    printf("Flags set to 0x%02X\n", conf.extra.flags.value); 
    printf(" |==> FLAG Disable Powerbutton %s SET\n", conf.extra.flags.PB_DISABLE ? "IS" : "NOT");
    printf(" |==> FLAG Forground mode %s SET\n", conf.extra.flags.FOREGROUND_MODE ? "IS" : "NOT");
    printf(" |==> FLAG Use sysfs for temperature %s SET\n", conf.extra.flags.USE_SYSFS ? "IS" : "NOT");
                
    return 0;
}
