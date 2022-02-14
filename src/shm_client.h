/*
MIT License

Copyright (c) 2021 DarkElvenAngel

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

#ifndef SHM_CLIENT_H
#define SHM_CLIENT_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <dirent.h>
#include <poll.h>
#include <inttypes.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "argononed.common.h"

struct _SHM_IPC_CLIENT_D {
    int daemon_pid;
    int shm_fd;
    struct SHM_Data* memory;
} typedef ArgonMem;

typedef enum {
    AR_MODE_AUTO = 0,
    AR_MODE_OFF  = 1,
    AR_MODE_MAN  = 2, 
    AR_MODE_COOL = 3
} ArgonModes;

ArgonMem* New_ArgonMem();
int Open_ArgonMem(ArgonMem* ar_ptr);
void Close_ArgonMem(ArgonMem* ar_ptr);

int Get_Config(ArgonMem* ar_ptr, struct DTBO_Config*);
int Get_Statistics(ArgonMem* ar_ptr, struct SHM_DAEMON_STATS*);
int Get_Current_Temperature(ArgonMem* ar_ptr, uint8_t *temperature);
int Get_Current_FanSpeed(ArgonMem* ar_ptr, uint8_t *speed);

int Set_FanMode(ArgonMem* ar_ptr, ArgonModes mode_select);
int Set_FanSpeed(ArgonMem* ar_ptr, uint8_t speed);
int Set_TargetTemperature(ArgonMem* ar_ptr, uint8_t temperature);
int Set_Schedule(ArgonMem* ar_ptr, struct DTBO_Config Schedule);

int Set_CoolDown(ArgonMem* ar_ptr, uint8_t temperature, uint8_t speed);
int Set_ManualFan(ArgonMem* ar_ptr, uint8_t speed);

int Send_Request(ArgonMem* ar_ptr);
int Send_Reset(ArgonMem* ar_ptr);

#endif