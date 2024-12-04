#pragma once

enum W25_State
{
    Read,
    Write
};

enum State {
    ENABLE_RESET,
    DEVICE_RESET,
    SEND_READ_JEDEC,
    ACCEPT_JEDEC_ID,
    ACCEPT_ID2,
    ACCEPT_ID1,
    FREE,
    READ_DATA,
    SEND_ADDRESS,
    ACCEPT_DATA,
    SEND_WRITE_ENABLE,
    SEND_ERASE,
    ERASED,
    SEND_PROGRAM,
    SEND_DATA
};

#define BUFFER_SIZE 1024
#define PAGE_SIZE 256
