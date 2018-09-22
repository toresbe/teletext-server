#ifndef _WIN32
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>

#include "shm_common.h"
#include "ttxdata/ttxdata.hpp"

ttx_shm_buffer_file * open_shm_file(char * service_name) {
    char shm_filename[64] = "obe_ttx_";
    strncat(shm_filename, service_name, 63 - strlen(shm_filename));

    int shm_file = shm_open(shm_filename, O_RDWR | O_CREAT, 0666);

    if (shm_file == -1) {
        printf("Failed to open shm file '%s'; error: '%s'!\n", shm_filename, strerror(errno)); return 0;
    };

    if (ftruncate(shm_file, sizeof(ttx_shm_buffer_file)) == -1) {
        printf("Failed to set shm file '%s' size: '%s'\n", shm_filename, strerror(errno));
        return 0;
    };

    ttx_shm_buffer_file * buffer_pointer = (ttx_shm_buffer_file *) \
                                           mmap(
                                                   NULL,
                                                   sizeof(ttx_shm_buffer_file),
                                                   PROT_READ | PROT_WRITE,
                                                   MAP_SHARED,
                                                   shm_file,
                                                   0
                                               );
    if (buffer_pointer == MAP_FAILED) {
        printf("Failed to map shared memory file; error '%s'!\n", strerror(errno));
        return 0;
    }

    return buffer_pointer;
}

void shm_flip_buffers(ttx_shm_buffer_file * buf) {
    sem_wait(&buf->flip_sem);
    buf->front_buffer = (buf->front_buffer == &buf->buffer1) ? &buf->buffer2 : &buf->buffer1;
    msync(buf, sizeof(buf->header), MS_SYNC);
    sem_post(&buf->flip_sem);
}
#endif
