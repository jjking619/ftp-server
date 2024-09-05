#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <libgen.h>
enum CMD_NO  //命令
{
  FTP_CMD_LS = 1024,
  FTP_CMD_GET,
  FTP_CMD_PUT,
  FTP_CMD_BYE,
  FTP_CMD_UNKNOW = 0x1111
};
extern int confd;
#endif