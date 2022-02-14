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

#ifndef ARGONONE_SHM_H
#define ARGONONE_SHM_H
#include <stdint.h>
#include <string.h>
#include "argononed.common.h"

int argonon_shm_start();
/**
 * Reset Shared Memory to match correct values
 * 
 * \return none
 */
void reset_shm();
/**
 * Reload the configuration from shared memory
 * 
 * \return 0 on success
 */
int reload_config_from_shm();
/**
 * Reset Shared Memory
 * 
 * \note This is meant to be called with a Timer.
 * \param timer_id calling timer id
 * \param user_data pointer to argument data
 * \return none
 */
void TMR_SHM_Reset(size_t timer_id, void *user_data __attribute__((unused)));
/**
 * Monitor Shared Memory for commands
 * 
 * \note This is meant to be called with a Timer.
 * \param timer_id calling timer id
 * \param user_data pointer to argument data
 * \return none
 */
void TMR_SHM_Interface(size_t timer_id __attribute__((unused)), void*);
/**
 * Monitor Shared Memory for commands Legacy
 * 
 * \note This is meant to be called with a Timer.
 * \param timer_id calling timer id
 * \param user_data pointer to argument data
 * \return none
 */
void TMR_Legacy_Interface(size_t timer_id __attribute__((unused)), void *user_data __attribute__((unused)));
#endif