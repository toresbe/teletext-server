#include "shm_common.h"
ttx_shm_buffer_file * shm_buffer;

void foo() {
    // todo: read this from config file
    shm_buffer = open_shm_file((char *)"frikanalen");
}
