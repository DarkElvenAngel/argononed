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

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <string.h>

/**
 * @brief Write to an I2C slave device's register:
 * 
 * @param[in] fd i2c file descriptor 
 * @param[in] slave_addr address of device
 * @param[in] reg register to read from
 * @param[in] data byte to write
 * @return int 
 */
int i2c_write(int fd, unsigned char slave_addr, unsigned char reg, unsigned char data) {
    //int retval;
    unsigned char outbuf[2];

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
/**
 * @brief  Read the given I2C slave device's register
 * 
 * @param[in] fd i2c file descriptor 
 * @param[in] slave_addr address of device
 * @param[in] reg register to read from
 * @param[out] result byte value read 
 * @return int 
 */
int i2c_read(int fd, unsigned char slave_addr, unsigned char reg, unsigned char *result) {
    // int retval;
    unsigned char outbuf[1], inbuf[1];
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

int main(int argc, char **argv)
{
    if (argc == 1) return 0;
    unsigned char test_data, return_data;
    if (strstr(" poweroff halt ", argv[1]) != NULL) 
    {
        int file_i2c = 0;
        char cmd_shutdown = 0x00;  // Turn off Fan 
        char *filename = (char*)"/dev/i2c-1";
        if ((file_i2c = open(filename, O_RDWR)) < 0)
        {
            return -1;
        }
        int addr = 0x1a;
        if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
        {
            return -1;
        }
        i2c_read(file_i2c, 0x1a, 0x80, &test_data);
        return_data = test_data + 1;
        i2c_write(file_i2c, 0x1a, 0x80, return_data);
        i2c_read(file_i2c, 0x1a, 0x80, &return_data);
        if (return_data != test_data)
        {
            // V3 turn off fan
            if (i2c_write(file_i2c,0x1a,0x80,0) != 1)
            {
                return -1;
            }
            // V3 Shutdown Command
            if (i2c_write(file_i2c,0x1a,0x86,1) != 1)
            {
                return -1;
            }
        } else {
            if (write(file_i2c, &cmd_shutdown, 1) != 1)
            {
                return -1;
            }
            cmd_shutdown = 0xff;        // Shutdown Command
            if (write(file_i2c, &cmd_shutdown, 1) != 1)
            {
                return -1;
            }
        }
        close(file_i2c);
    }
    return 0;
}
