#ifndef __FTP_SERVER_H__
#define __FTP_SERVER_H__
int get_filename(); // 获取本目录下所有文件名
int down_file(char *arg, int arg_len);
void parse_cmd(unsigned char *recvbuf); // 拆包
void cmd_ls(unsigned char *recvbuf, int *resp_length);
void cmd_get(unsigned char *recvbuf, int *resp_length);
void cmd_put(unsigned char *recvbuf, int *resp_length);
void cmd_bye(unsigned char *recvbuf, int *resp_length);

#endif