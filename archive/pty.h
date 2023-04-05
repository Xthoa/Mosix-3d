#pragma once

#include "vfs.h"
#include "msglist.h"

typedef struct s_Pty{
    Node* slave;
    int slaveno;
    FifoBuffer in;
    FifoBuffer out;
} Pty;
