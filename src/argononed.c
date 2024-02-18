/*
MIT License

Copyright (c) 2024 DarkElvenAngel

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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "argononed.common.h"
#include "identapi.h"
#include "event_timer.h"
#include "argonone_shm.h"

struct SHM_Data* ptr = NULL;            // Global Shared Memory Pointer
Daemon_Conf Configuration = { 0 };      // Global Daemon Configuration

void TMR_Get_temp(size_t timer_id, void *user_data);
void Set_FanSpeed(uint8_t fan_speed);

/**
 * \brief Prepare daemon for shutdown.
 * 
 * \return none
 */ 
void Clean_Exit(int status)
{
    log_message(LOG_INFO, "Begin Daemon Clean up");
    uint8_t cmd = 1;
    TMR_Get_temp(0,&cmd);
    Set_FanSpeed(0);
    Set_FanSpeed(0xFF);
    log_message(LOG_DEBUG,"  Clean_Exit close_timers()"); close_timers();
    log_message(LOG_DEBUG,"  Clean_Exit shm_unlink(SHM_FILE) return %d",shm_unlink(SHM_FILE));
    log_message(LOG_DEBUG,"  Clean_Exit unlink(LOCK_FILE) return %d", unlink(LOCK_FILE));
    log_message(LOG_INFO, "Daemon ready for shutdown");
    log_message(status == 0 ? LOG_INFO : LOG_ERROR + LOG_BOLD, "Daemon Exiting Status %d", status);
    exit(status);
}

/**
 * \brief Signal handler
 * 
 * \attention This function should not be called directly
 * 
 * \param sig Signal received 
 * \return none
 */ 
void signal_handler(int sig){
    switch(sig){
        case SIGHUP:
            log_message(LOG_INFO + LOG_BOLD,"Received SIGHUP Hang up");
#ifndef DISABLE_LEGACY_IPC
            reload_config_from_shm();
#endif
            break;
        case SIGTERM:
            log_message(LOG_INFO + LOG_BOLD,"Received SIGTERM Terminate");
            Clean_Exit(0);
            break;
        case SIGINT:
            log_message(LOG_INFO + LOG_BOLD,"Received SIGINT Interupt");
            Clean_Exit(0);
            break;
        default:
            log_message(LOG_INFO + LOG_BOLD,"Received Signal %s", strsignal(sig));
    }
}

/**
 * \brief Alarm signal handler
 * 
 * \attention This function shouldn't be called directly
 *  
 * \param sig ignored
 */
void Alarm_handler(int sig __attribute__((unused)))
{
    log_message(LOG_DEBUG + LOG_BOLD,"Received Signal ALARM");
}
// Write to an I2C slave device's register:
int i2c_write(int fd, uint8_t slave_addr, uint8_t reg, uint8_t data) {
    int retval;
    uint8_t outbuf[2];

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgset[1];

    outbuf[0] = reg;
    outbuf[1] = data;

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = outbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &msgset) < 0) {
        return 0;
    }

    return 1;
}
// Read the given I2C slave device's register and return the read value in `*result`:
int i2c_read(int fd, uint8_t slave_addr, uint8_t reg, uint8_t *result) {
    int retval;
    uint8_t outbuf[1], inbuf[1];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = outbuf;

    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msgs[1].len = 1;
    msgs[1].buf = inbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    outbuf[0] = reg;

    inbuf[0] = 0;

    *result = 0;
    if (ioctl(fd, I2C_RDWR, &msgset) < 0) {
        return -1;
    }

    *result = inbuf[0];
    return 0;
}
/**
 * \brief Send fan speed request to the argon micro controller
 * 
 * This function will attempt connect to the configured i2c bus.
 * On the event of a bus error the connection is closed and reset
 * 
 * \attention THERE IS NO ERROR FEEDBACK
 * 
 * \todo Add Error feedback and i2c bus scanning
 * 
 * \param[in] fan_speed 0-100 to set fanspeed or 0xFF to close I2C interface
 * \return none
 */
void Set_FanSpeed(uint8_t fan_speed)
{
    static int file_i2c = 0;        // i2c file descripter
    static uint8_t speed = 1;       // Current fan speed 
    static bool ctrl_reg = false;   // This is a V3+ case? 
    unsigned long functions = 0;
	if (file_i2c == 0)
    {
        char filename[14]; // = (char*)"/dev/i2c-1  ";
        snprintf(filename,14,"/dev/i2c-%d", Configuration.extra.bus);
        log_message(LOG_INFO,"Attempt to open the i2c bus at %s", filename);
        if ((file_i2c = open(filename, O_RDWR)) < 0)
        {
            log_message(LOG_CRITICAL,"Failed to open the i2c bus");
            file_i2c = 0;  // Reset to zero this will allow the daemon to retry the connection
            return;
        }
        if (ioctl(file_i2c, I2C_FUNCS, &functions) < 0) {
            log_message(LOG_WARN, "Could not get the adapter functionality matrix: %s", strerror(errno));
        }
        int addr = 0x1a;
        if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
        {
            if (errno == EBUSY) log_message(LOG_WARN, "Device address is busy");
            else log_message(LOG_CRITICAL,"Failed to acquire bus access");
            close(file_i2c);
            file_i2c = 0; // Reset so the i2c can reconnect if needed
            return;
        }
        if ((functions & I2C_FUNC_SMBUS_QUICK))
        {
            struct i2c_smbus_ioctl_data args;
            args.read_write = I2C_SMBUS_WRITE;
            args.command = 0;
            args.size = 0;
            args.data = NULL;

            if (ioctl(file_i2c, I2C_SMBUS, &args) < 0)
            {
                log_message(LOG_WARN, "Unable to detect Argon fan controller");
                close(file_i2c);
                file_i2c = 0; // Reset so the i2c can reconnect on a different bus if requested
                return;
            }
            else
                log_message(LOG_DEBUG, "Argon fan controller found");
        }
        // Scan for V3 controller
        {
            
        }
        log_message(LOG_INFO,"I2C Initialized");
    }
    if (fan_speed <= 100 && fan_speed != speed)
    {
        // Pi 5 Version only
        if (i2c_write(file_i2c, 0x1a, 80, fan_speed) != 1)
        {
            log_message(LOG_CRITICAL,"Failed to write to the i2c bus.");
        }
        log_message(LOG_INFO, "Set fan to %d%%",fan_speed);
        speed = fan_speed;
        ptr->fanspeed = fan_speed;
    } else if (fan_speed == 0xFF)
    {
        close(file_i2c);
        file_i2c = 0; // Reset so the i2c can reconnect if needed
        log_message(LOG_INFO,"i2c closed");
    }
}
/**
 * \brief Read the CPU temperature
 * 
 * Fetch the CPU temperature, this will use the configured interface
 * if the temperature cannot be read then return -1 and do not update
 * CPU_Temp
 * 
 * \param[OUT] CPU_Temperature* pointer to hold CPU temperature
 * \param[IN] command 0 get temperature, 1 close fd if open
 * \return 0 if CPU_Temp is valid 
 */
int Get_CPU_Temp(uint32_t *CPU_Temperature, uint8_t command)
{
    uint32_t CPU_Temp = 0;
    static int32_t fdtemp = 0;
    int return_val = 0;
    FILE* fptemp = 0;
    uint32_t property[10] =
    {
        0x00000000,
        0x00000000,
        0x00030006,
        0x00000008,
        0x00000004,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000
    };
    if (Configuration.extra.flags.USE_SYSFS)
    {
        char filename[36];
        snprintf(filename,36,"/sys/class/hwmon/hwmon%d/temp1_input", Configuration.extra.flags.SET_HWMON_NUM);
        log_message(LOG_DEBUG,"Open %s for temperature ",filename);
        fptemp = fopen(filename, "r");
        if (fptemp)
        {
            fscanf(fptemp, "%d", &CPU_Temp);
            fclose(fptemp);
        } else {
            return_val = -1;
            log_message(LOG_CRITICAL, "Temperature can not be monitored!!");
        }
        CPU_Temp = CPU_Temp / 1000;
    } else {
        property[0] = 10 * sizeof(property[0]);
        if (command == 0)
        {
            if (fdtemp == 0)
            {
                fdtemp = open("/dev/vcio", 0);
                if (fdtemp == -1)
                {
                    log_message(LOG_CRITICAL, "Cannot access VideoCore I/O!");
                    return_val = -1;
                    log_message(LOG_CRITICAL, "Temperature can not be monitored!!");
                }
            }
            if (ioctl(fdtemp, _IOWR(100, 0, char *), property) == -1)
            {
                log_message(LOG_CRITICAL, "Cannot get CPU Temp!");
                return_val = -1;
                log_message(LOG_CRITICAL, "Temperature can not be monitored!!");
            }
        CPU_Temp = property[6] / 1000;
        } else {
            close(fdtemp);
            log_message(LOG_INFO, "Successfully closed temperature sensor");
        }
    }
    if (return_val != 0)
    {
        close(fdtemp);
        fdtemp = 0;
    } else *CPU_Temperature = CPU_Temp;
    return return_val;
}

/**
 * \brief Read temperature and process temperature data
 * 
 * \note This is meant to be called with a Timer.
 * \bug  If the temperature cannot be monitored then fan control isn't possible.
 * \param timer_id calling timer id
 * \param user_data pointer to argument data
 * \return none
 */
void TMR_Get_temp(size_t timer_id, void *user_data)
{
    uint32_t CPU_Temp = 0;
	static uint8_t fanspeed = 0;
    static uint8_t temp_error = 0;
    uint8_t command = (user_data == NULL ? 0 :*(uint8_t*)user_data);
#if 0
    static int32_t fdtemp = 0;
    FILE* fptemp = 0;
    uint32_t property[10] =
    {
        0x00000000,
        0x00000000,
        0x00030006,
        0x00000008,
        0x00000004,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000
    };
    if (Configuration.extra.flags.USE_SYSFS)
    {
        char filename[36];
        snprintf(filename,36,"/sys/class/hwmon/hwmon%d/temp1_input", Configuration.extra.flags.SET_HWMON_NUM);
        log_message(LOG_DEBUG,"Open %s for temperature ",filename);
        fptemp = fopen(filename, "r");
        if (fptemp)
        {
            fscanf(fptemp, "%d", &CPU_Temp);
            fclose(fptemp);
        } else {
            stop_timer(timer_id);
            log_message(LOG_CRITICAL, "Temperature can not be monitored!!");
        }
        CPU_Temp = CPU_Temp / 1000;
    } else {
        property[0] = 10 * sizeof(property[0]);
        if (user_data == NULL)
        {
            if (fdtemp == 0)
            {
                fdtemp = open("/dev/vcio", 0);
                if (fdtemp == -1)
                {
                    log_message(LOG_CRITICAL, "Cannot access VideoCore I/O!");
                    stop_timer(timer_id);
                    log_message(LOG_CRITICAL, "Temperature can not be monitored!!");
                } else { // this will flood the logs!
                    // log_message(LOG_INFO, "Successfully opened /dev/vcio for temperature sensor");
                }
            }
            if (ioctl(fdtemp, _IOWR(100, 0, char *), property) == -1)
            {
                log_message(LOG_CRITICAL, "Cannot get CPU Temp!");
                stop_timer(timer_id);
                log_message(LOG_CRITICAL, "Temperature can not be monitored!!");
            }
            CPU_Temp = property[6] / 1000;
        } else {
            close(fdtemp);
            log_message(LOG_INFO, "Successfully closed temperature sensor");
            return;
        }
    } 
#endif
    switch (command)
    {
        case 0:
            if (temp_error == 0)
                if (Get_CPU_Temp(&CPU_Temp, 0) != 0)
                {
                    temp_error = 1;
                    return;
                }
            break;
        default:
            Get_CPU_Temp(&CPU_Temp, command);
    }

    #if 1 // SKIP FAN SWITCHING
    switch (Configuration.runstate)
    {
        case 0: //AUTO
        switch (fanspeed)
        {
            case 0:
            if (CPU_Temp >= Configuration.configuration.thresholds[0]) fanspeed = 1;
            Set_FanSpeed(0);
            break;
            case 1:
            if (CPU_Temp >= Configuration.configuration.thresholds[1]) fanspeed = 2;
            if (CPU_Temp <= (uint8_t)(Configuration.configuration.thresholds[0] - Configuration.configuration.hysteresis)) fanspeed = 0;
            Set_FanSpeed(Configuration.configuration.fanstages[0]);
            break;
            case 2:
            if (CPU_Temp >= Configuration.configuration.thresholds[2]) fanspeed = 3;
            if (CPU_Temp <= (uint8_t)(Configuration.configuration.thresholds[1] - Configuration.configuration.hysteresis)) fanspeed = 1;
            Set_FanSpeed(Configuration.configuration.fanstages[1]);
            break;
            case 3:
            if (CPU_Temp <= (uint8_t)(Configuration.configuration.thresholds[2] - Configuration.configuration.hysteresis)) fanspeed = 2;
            Set_FanSpeed(Configuration.configuration.fanstages[2]);
            break;
        }
        break;
        case 1: Set_FanSpeed(0); break; // OFF
        case 2: // MANUAL OVERRIDE
        Set_FanSpeed(Configuration.fanspeed_Overide);
        break;
        case 3: // COOLDOWN
        if (CPU_Temp <= Configuration.temperature_target)
        {
            log_message(LOG_INFO, "Cool down complete. switch to AUTO mode"); 
            Set_FanSpeed(0);
            Configuration.runstate = 0;
            ptr->fanmode = 0;
        } else {
            Set_FanSpeed(Configuration.fanspeed_Overide);
        }
        break;
    }
    #endif
    ptr->temperature = (uint8_t)CPU_Temp;
    if (CPU_Temp > ptr->stat.max_temperature) ptr->stat.max_temperature = (uint8_t)CPU_Temp;
    if ( (ptr->stat.min_temperature == 0) || (CPU_Temp < ptr->stat.min_temperature)) ptr->stat.min_temperature = (uint8_t)CPU_Temp;
}
/**
 * \brief This Function is used to watch for the power button events.
 * 
 * \note Call is Blocking
 * \param[out] Pulse_Time_ms pointer used to hold the pulse time in ms 
 * \return 0 when Pule_Time_ms is valid or error code
 */
int32_t monitor_device(uint32_t *Pulse_Time_ms)
{
    static int32_t E_Flag = 0;
	struct gpioevent_request req;
	int fd;
	int ret = 0;
    if (E_Flag == -1 || Configuration.extra.flags.PB_DISABLE) { // E_Flag -1 disable attempts to open /dev/gpiochip0 
        while (1) {
            usleep(10000);
        }
    }
    *Pulse_Time_ms = 0; // Initialize to zero
	fd = open("/dev/gpiochip0", 0);
	if (fd == -1) {
        log_message(LOG_CRITICAL, "Unable to open /dev/gpiochip0 : %s", strerror(errno));
		ret = errno;
        if (ret == ENOENT) // /dev/gpiochip0 doesn't exists
        {
            E_Flag = -1;
            log_message(LOG_FATAL, "Powerbutton monitoring has failed!");
        }
		goto exit_close_error;
	}
	req.lineoffset = 4;
	req.handleflags = GPIOHANDLE_REQUEST_INPUT;
	req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
	strcpy(req.consumer_label, "argonone-powerbutton");
	ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &req);
	if (ret == -1) {
        if (E_Flag == 0)
        {
            log_message(LOG_CRITICAL, "Unable to get GPIO Line Event : %s", strerror(errno));
            E_Flag = errno;
        }
		ret = errno;
		goto exit_close_error;
	}
    if (E_Flag != 0 && ret == 0)
    {
        log_message(LOG_INFO, "GPIO Line Event Cleared ");
        E_Flag = 0;
    }
	log_message(LOG_INFO, "Monitoring line 4 on /dev/gpiochip0");
    uint32_t Rtime = 0;
	while (1) {
		struct gpioevent_data event;
		ret = read(req.fd, &event, sizeof(event));
		if (ret == -1) {
            if (E_Flag == 0)
            {
                if (errno == EAGAIN) {
                    continue;  // No Data Retry
                }
                if (errno == EBUSY) { // BLOCK ERROR "Device or resource busy" It seems to be a false alarm
                    usleep(10000);
                    continue;
                } else {
                    log_message(LOG_ERROR, "Unable to read GPIO event : %s", strerror(errno));
                    ret = errno;
                    break;
                }
                E_Flag = errno;
            }
		}
		if (ret != sizeof(event)) {
            log_message(LOG_ERROR, "GPIO Event malformed Reply");
			ret = EIO;
			break;
		}
		if (event.id == GPIOEVENT_EVENT_RISING_EDGE)
        {
            log_message(LOG_DEBUG, "GPIOEVENT_EVENT_RISING_EDGE @ %d",event.timestamp / 1000000);
        	Rtime = (uint32_t)(event.timestamp / 1000000);
        }
		if (event.id == GPIOEVENT_EVENT_FALLING_EDGE)
        {
            log_message(LOG_DEBUG, "GPIOEVENT_EVENT_FALLING_EDGE @ %d",event.timestamp / 1000000);
            if ( ((event.timestamp / 1000000) - Rtime) > 0 )
            {
                *Pulse_Time_ms = (uint32_t)(event.timestamp / 1000000) - Rtime;
                ret = 0;
                break;
            }
            log_message(LOG_ERROR, "Negative pulse time");
            // negative pulse time is invalid and should be ignored
            // TODO: Detect underlining cause and report
		}
	}
exit_close_error:
    // This will flood the log with useless information.
    // if (ret != 0) log_message(LOG_DEBUG, "FUNC [monitor_device] Exit with error %d : %s", ret, strerror(ret));
	close(fd);
	close(req.fd); // FIXES file descripter leak
	return ret;
}
/**
 * \brief Fork into a daemon
 * \note only call once
 * 
 * \param *conf Current configuration
 * 
 * \return none
 */
int daemonize(struct DTBO_Data *conf){
    int lfp;
    char str[10];
    if (!conf->extra.flags.FOREGROUND_MODE)
    {
        int i = fork();
        if(i < 0)
            exit(1);
        if(i > 0)
            exit(0);
        setsid();
        for(i = getdtablesize(); i >= 0; --i)
            close(i);
        i = open("/dev/null",O_RDWR);
        dup(i);
        dup(i);
        log_message(LOG_INFO,"Now running as a daemon");
    }
    umask(0);
    chdir(RUNNING_DIR);
    lfp = open(LOCK_FILE,O_RDWR|O_CREAT,0640);
    if(lfp < 0)
    {
        log_message(LOG_FATAL,"Lock file can't be created");
        return 1;
    }
    if(lockf(lfp,F_TLOCK,0) < 0)
    {
        log_message(LOG_FATAL,"Lock file cannot be locked");
        return 1;
    }
    sprintf(str,"%d\n",getpid());
    if (write(lfp,str,strlen(str)) > 0) 
    {
        log_message(LOG_INFO,"Lock file created");
    } else {
        log_message(LOG_FATAL,"cannot write to lock file");
        return 1;
    }
    close(lfp);
    signal(SIGCHLD,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);
    signal(SIGALRM,Alarm_handler);
    signal(SIGCHLD,signal_handler);
    signal(SIGUSR1,signal_handler);
    signal(SIGUSR2,signal_handler);
    signal(SIGURG,signal_handler);
    signal(SIGHUP,signal_handler);
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);
    return 0;
}

int main(int argc,char **argv)
{
    Init_Configuration(&Configuration);
    struct DTBO_Data Arguments = { 0 };
    Parse_Command_Line_Arguments(argc, argv, &Arguments, &Configuration);
    Configuration.extra.flags.value = Arguments.extra.flags.value;
    Configuration.Log_Level = Arguments.Log_Level;
    Configuration.colour = Arguments.colour;
#if 1 // bypass privilege user checks
    // Check we are running as root
    if (getuid() != 0) {
        fprintf(stderr, "ERROR:  Permissions error, must be run as root\n");
        exit(1);
    }
    // check for unclean exit
    if (access(LOCK_FILE, F_OK) != -1)
    {
        fprintf(stderr, "WARNING:  argononed Lock file found\n");
        FILE* file = fopen (LOCK_FILE, "r");
        int d_pid = 0;
        fscanf (file, "%d", &d_pid);
        fclose (file);
        if (kill(d_pid, 0) == 0)
        {
          fprintf(stderr, "ERROR:  argononed ALREADY RUNNING\n");
          exit (1);
        }
        log_message(LOG_WARN + LOG_BOLD, "Unclean exit detected");
        unlink (LOCK_FILE);
        shm_unlink(SHM_FILE);
        log_message(LOG_INFO, "Clean up complete");
    }
#endif
    log_message(LOG_INFO,"Startup ArgonOne Daemon version %s", DAEMON_VERSION);
    log_message(LOG_INFO + LOG_BOLD,"Checking Board Revision");
    struct identapi_struct Pirev;
    Pirev.RAW = IDENTAPI_GET_Revision();
    if (Pirev.RAW == 1)
    {
        log_message(LOG_INFO,"Unable to read valid revision code");  // Since GPIO isn't in use this isn't a fatal and only needed for logging
        //return 1;
    } else {

        float frev = 1.0f + (Pirev.REVISION / 10.0f);
        char memstr[11];
        if (IDENTAPI_GET_int(Pirev, IDENTAPI_MEM) > 512) sprintf(memstr,"%dGB",IDENTAPI_GET_int(Pirev, IDENTAPI_MEM) / 1024);
        else sprintf(memstr,"%dMB",IDENTAPI_GET_int(Pirev, IDENTAPI_MEM));
        log_message(LOG_INFO, "Detected RPI MODEL %s %s rev %1.1f", IDENTAPI_GET_str(Pirev, IDENTAPI_TYPE), memstr, frev);
    }
    umask(0);
    log_message(LOG_INFO + LOG_BOLD, "Begin Initalizing shared memory");
	int shm_fd = shm_open(SHM_FILE, O_CREAT | O_RDWR, 0666);
	ftruncate(shm_fd, SHM_SIZE);
    ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		log_message(LOG_FATAL, "Shared memory map error");
		Clean_Exit(1);
	}
    log_message(LOG_INFO + LOG_BOLD,"Shared memory initialized");
    log_message(LOG_INFO + LOG_BOLD,"Loading Configurations");
    int ret = Read_DeviceTree_Data(&Configuration);
    log_message(LOG_DEBUG,"Read_DeviceTree_Data return %d", ret);
    if (Arguments.filename)
        ret = Read_Configuration_File(Arguments.filename, &Configuration);
    else
        ret = Read_Configuration_File("/etc/argononed.conf", &Configuration);
    log_message(LOG_DEBUG,"Read_Configuration_File return %d", ret);
    Check_Configuration(&Configuration,Arguments.configuration, 1);

    log_message(LOG_INFO + LOG_BOLD,"Configuration Complete");
    Configuration_log(&Configuration);
    if (daemonize(&Configuration) != 0) {
        Clean_Exit(1);
        log_message(LOG_ERROR + LOG_BOLD,"Daemon Exiting Status 1");
        exit(1);
    }
    initialize_timers();
    argonon_shm_start();
    size_t timer1 __attribute__((unused)) = start_timer_long(2, TMR_Get_temp,TIMER_PERIODIC,NULL);

    log_message(LOG_INFO + LOG_BOLD,"Enter Main loop");
    if (!Configuration.extra.flags.PB_DISABLE)
    {
        log_message(LOG_INFO + LOG_BOLD,"Now waiting for button press");
        uint32_t count = 0;
        do
        {
            if (monitor_device(&count) == 0)
            {
                log_message(LOG_DEBUG, "Pulse received %dms", count);
                if ((count >= 19 && count <= 21) || (count >= 39 && count <= 41)) break;
                sleep(1);
            }
            // monitor_device has produced and error
            usleep(10000);  // Shouldn't be reached but prevent overloading CPU
        } while (1);
        if (count >= 19 && count <= 21)
        {
            log_message(LOG_DEBUG, "EXEC REBOOT");
            sync();
            system("/sbin/reboot");
        }
        if (count >= 39 && count <= 41)
        {
            log_message(LOG_DEBUG, "EXEC SHUTDOWN");
            sync();
            system("/sbin/poweroff");
        }
    } else {
        log_message(LOG_INFO + LOG_BOLD,"Daemon Ready");
        for(;;)
        {
            sleep(1); // Main loop to sleep 
        }
    }

    Clean_Exit(0);
}