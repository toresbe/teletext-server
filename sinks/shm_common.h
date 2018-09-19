#ifndef __SHM_COMMON_H
#define __SHM_COMMON_H
#ifndef _WIN32
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#define LINES_PER_FIELD 10 
#define FIELDS_PER_BUFFER 1000
#define LINES_PER_BUFFER LINES_PER_FIELD * FIELDS_PER_BUFFER

typedef int timecode_t; // not thought out yet, placeholder type, assume it's fields since epoch or something

typedef struct { uint8_t data[40]; } ttx_line_t;

typedef struct {
    ttx_line_t  line[LINES_PER_FIELD];
    int         is_page_boundrary;      // can we make a clean buffer flip after having sent this field?
} ttx_field_t;

typedef struct { 
    timecode_t  starting_time;
    sem_t               buf_sem;
    ttx_field_t field[FIELDS_PER_BUFFER];
} ttx_shm_buffer_t;

typedef struct {
    pid_t   obe_pid;
    pid_t   ttxd_pid;
    int     ttx_lines_per_field; // number of lines per field of video
    int     ttx_fields_per_buffer; // number of fields stored in each buffer
} ttx_shm_header_t;

typedef struct {
    ttx_shm_header_t    header;
    ttx_shm_buffer_t    *front_buffer; // pointer to the buffer most recently rendered
    sem_t               flip_sem;
    int                 page_bound_encountered; // ttxd sets this to 0 after swapping buffers,
                                                // obe sets this to 1 after encountering a 
                                                // cleanly switchable point.
    ttx_shm_buffer_t    buffer1;
    ttx_shm_buffer_t    buffer2;
} ttx_shm_buffer_file;

ttx_shm_buffer_file * open_shm_file(char * service_name);
void shm_flip_buffers(ttx_shm_buffer_file * buf);
#endif
#endif