#pragma once

#define Status int
#define Success 0

#define EPERM 1
#define ErrNotPermitted 1
#define ENOENT 2
#define ErrNotFound 2
#define ErrNull 3
#define EINVAL 4
#define ErrInvalidArgument 4
#define EIO 5
#define ErrFull 6
#define ErrUnknown 7
#define ENOEXEC 8
#define ErrExecFormat 8
#define EBADF 9
#define ErrBadFileno 9
#define ErrNotExist 10
#define EAGAIN 11
#define ErrTryAgain 11
#define ENOMEM 12
#define ErrOutOfMemory 12
#define EACCES 13
#define ErrPermission 13
#define EFAULT 14
#define ErrBadAddress 14
#define ENOTBLK 15
#define EBUSY 16
#define ErrBusy 16
#define EEXIST 17
#define ENOIMPL 18
#define ErrNotImplemented 18
#define ENODEV 19
#define ENOTDIR 20
#define ErrNotDirectory 20
#define EISDIR 21
#define ErrIsDirectory 21
#define EBADSEK 22
#define ErrBadSeek 22
#define ErrVmareaOverlap 160
#define ErrNoSuchVmarea 161
#define ErrProcReady 162

void set_errno(u16 errno);
u16 get_errno();
