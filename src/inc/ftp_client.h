#ifndef __FTP_CLIENT_H__
#define __FTP_CLIENT_H__

void judge_cmd(unsigned char *s);                          // 判断命令
void parse_data(unsigned char *recvbuf, int *resp_length); // 解析数据包
void cmd_get(unsigned char *arg, unsigned char *cmd);      // get命令
void cmd_ls(unsigned char *cmd);                           // ls命令
void cmd_put(unsigned char *arg, unsigned char *cmd);      // put命令
void cmd_bye(unsigned char *cmd);                          // put命令
void parse_get(int resp_len, unsigned char *recvbuf);
int put_file(char *arg, int arg_len);
#endif