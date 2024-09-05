#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "common.h"
#include "ftp_client.h"

int sockfd;
int confd;
int flag = 1; //判断客户端是否退出
int filecount = 0;  //要上传或者下载的文件数
struct sockaddr_in client;
unsigned char recvbuf[10240] = {0}; // 接收包 
int resp_length = 0;             // 接收数据的长度
unsigned char cmd[1024] = {0};  //命令的数组
unsigned char filenames[4096] = {0};  //存放要上传的文件数据
unsigned char arg[1024] = {0};   // 保存文件名的数组
unsigned char filelist[4096][100] = {0}; //保存文件名
enum CMD_NO cmdnum;
int main(int argc, char *argv[]) //./tcp_server IP PORT
{
  // 1.创建一个套接字
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sockfd)
  {
    perror("socket failed");
    return -1;
  }
  // 2.绑定服务器的地址
  client.sin_family = AF_INET;
  client.sin_port = htons(atoi("7777"));
  client.sin_addr.s_addr = inet_addr("172.70.0.154");
  //端口地址复用
  int s = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &s,4);
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &s,4);
  // 连接
  int ret = connect(sockfd, (struct sockaddr *)&client, sizeof(client));
  if (-1 == ret)
  {
    perror("connect failed");
    close(sockfd);
    return -1;
  }
  printf("connect %s[port:%d] success!!\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port)); // 打印服务端的信息
  while (1)
  {
    //发送
    unsigned char sendbuf[1024] = {0};
    fgets((char*)sendbuf, sizeof(sendbuf)-1, stdin);
    judge_cmd(sendbuf);
    //接收
    memset(recvbuf, 0, sizeof(recvbuf));
    recv(sockfd,(char *)recvbuf,sizeof(recvbuf)-1, 0);
    parse_data(recvbuf,&resp_length);
  }
}
//判断命令
void judge_cmd(unsigned char *s)
{
  if (strncmp("ls", s,2) == 0)
  {
     cmdnum = FTP_CMD_LS;
     cmd_ls(cmd);
  }
  else if (strncmp("get", s,3) == 0)
  {
     cmdnum = FTP_CMD_GET;
     cmd_get(arg, cmd);
  }
  else if (strncmp("put",s,3) == 0)
  {
     cmdnum = FTP_CMD_PUT;
     cmd_put(arg, cmd);
  }
  else if (strncmp("bye",s,3) == 0)
  {
     cmdnum = FTP_CMD_BYE;
     cmd_bye(cmd);
  }
  else{
     cmdnum = 0x1111;
  }
}
//执行ls命令
void cmd_ls(unsigned char *cmd)
{
  int i = 0;
  int pkg_len = 8;           // 数据包的总长度
  cmd[i++] = 0xC0;           // 包头
  cmd[i++] = pkg_len & 0xff; // 小端模式存放，先存低字节
  cmd[i++] = (pkg_len >> 8) & 0xff;
  cmd[i++] = (pkg_len >> 16) & 0xff;
  cmd[i++] = (pkg_len >> 24) & 0xff;
  cmd[i++] = cmdnum & 0xff; // 小端模式存放，先存低字节 命令号
  cmd[i++] = (cmdnum >> 8) & 0xff;
  cmd[i++] = (cmdnum >> 16) & 0xff;
  cmd[i++] = (cmdnum >> 24) & 0xff;
  cmd[i++] = 0xF0;
  resp_length = i;
  send(sockfd, cmd, resp_length, 0);
}
//执行get命令 arg--要下载的文件名
void cmd_get(unsigned char *arg, unsigned char *cmd)
{
  printf("请输入你要下载的文件数: ");
  scanf("%d", &filecount);
  printf("请输入你要下载的文件名: \n");
  for (int i = 0; i < filecount; i++)
    {
      scanf("%s", filelist[i]);
    }
    int i = 0;
    cmd[i++] = 0xC0;             // 包头
    int total_pkg_len_index = i; // 数据包的下标
    i += 4;
    // 命令
    cmd[i++] = cmdnum & 0xff; // 小端模式存放，先存低字节 命令号
    cmd[i++] = (cmdnum >> 8) & 0xff;
    cmd[i++] = (cmdnum >> 16) & 0xff;
    cmd[i++] = (cmdnum >> 24) & 0xff;
    // 文件数
    cmd[i++] = filecount & 0xff;
    cmd[i++] = (filecount >> 8) & 0xff;
    cmd[i++] = (filecount >> 16) & 0xff;
    cmd[i++] = (filecount >> 24) & 0xff;
    for (int f = 0; f < filecount; f++)
    {
      arg = filelist[f];
      int arg_len = strlen(arg);
      if (arg[arg_len - 1] == '\n')
      {
        arg[arg_len - 1] = '\0';
        arg_len--;
      }
      cmd[i++] = arg_len & 0xff; // 小端模式存放，先存低字节 文件名长度
      cmd[i++] = (arg_len >> 8) & 0xff;
      cmd[i++] = (arg_len >> 16) & 0xff;
      cmd[i++] = (arg_len >> 24) & 0xff;
      strncpy(cmd + i, (char *)arg, arg_len); // 字符串拷贝
      i += arg_len;
    }
    cmd[i++] = 0xF0;
    // 数据包的总长度
    int total_pkg_len = i;
    cmd[total_pkg_len_index] = total_pkg_len & 0xff;
    cmd[total_pkg_len_index + 1] = (total_pkg_len >> 8) & 0xff;
    cmd[total_pkg_len_index + 2] = (total_pkg_len >> 16) & 0xff;
    cmd[total_pkg_len_index + 3] = (total_pkg_len >> 24) & 0xff;
    resp_length = i;
    send(sockfd, cmd, total_pkg_len, 0);
}
// put命令,上传文件至服务器
void cmd_put(unsigned char *arg, unsigned char *cmd)
{
  printf("请输入你要上传的文件数: ");
  scanf("%d", &filecount);
  printf("请输入你要上传的文件名: \n");
  for (int i = 0; i < filecount; i++)
  {
    scanf("%s", filelist[i]);
  }
  memset(filenames, 0, sizeof(filenames));
  int i = 0;
  char result = 1;
  cmd[i++] = 0xC0; // 包头
  int index = i; //记录数据包的下标
  i += 4;
  //命令
  cmd[i++] = cmdnum & 0xff; // 小端模式存放，先存低字节 命令号
  cmd[i++] = (cmdnum >> 8) & 0xff;
  cmd[i++] = (cmdnum >> 16) & 0xff;
  cmd[i++] = (cmdnum >> 24) & 0xff;
 //文件数
  cmd[i++] = filecount & 0xff;
  cmd[i++] = (filecount >> 8) & 0xff;
  cmd[i++] = (filecount >> 16) & 0xff;
  cmd[i++] = (filecount >> 24) & 0xff;
  for (int f = 0; f < filecount; f++)
  {
    // 要上传的文件名长度
    arg = filelist[f];
    int arg_len = strlen(arg);
    if (arg[arg_len - 1] == '\n')
    {
      arg[arg_len - 1] = '\0';
      arg_len--;
    }
    cmd[i++] = arg_len & 0xff; // 小端模式存放，先存低字节 命令号
    cmd[i++] = (arg_len >> 8) & 0xff;
    cmd[i++] = (arg_len >> 16) & 0xff;
    cmd[i++] = (arg_len >> 24) & 0xff;
    strncpy(cmd + i, (char *)arg, arg_len);
    i += arg_len;
    arg[arg_len] = '\0';
    put_file(arg, arg_len);
    int x = strlen(filenames);
    cmd[i++] =x & 0xff; // 小端模式存放，先存低字节 命令号
    cmd[i++] = (x >> 8) & 0xff;
    cmd[i++] = (x >> 16) & 0xff;
    cmd[i++] = (x >> 24) & 0xff;
    strncpy(cmd + i, (char *)filenames, x);
    i += x;
  }
  cmd[i++] = result;
  cmd[i++] = 0xF0;
  int total_pkg_len = i;
  cmd[index] = total_pkg_len & 0xff;
  cmd[index + 1] = (total_pkg_len >> 8) & 0xff;
  cmd[index + 2] = (total_pkg_len >> 16) & 0xff;
  cmd[index + 3] = (total_pkg_len >> 24) & 0xff;
  resp_length = i;
  send(sockfd, cmd, resp_length, 0);

  }
//bye命令
void cmd_bye( unsigned char *cmd)
{
  int i = 0;
  int pkg_len = 8;           // 数据包的总长度
  cmd[i++] = 0xC0;           // 包头
  cmd[i++] = pkg_len & 0xff; // 小端模式存放，先存低字节
  cmd[i++] = (pkg_len >> 8) & 0xff;
  cmd[i++] = (pkg_len >> 16) & 0xff;
  cmd[i++] = (pkg_len >> 24) & 0xff;
  cmd[i++] = cmdnum & 0xff; // 小端模式存放，先存低字节 命令号
  cmd[i++] = (cmdnum >> 8) & 0xff;
  cmd[i++] = (cmdnum >> 16) & 0xff;
  cmd[i++] = (cmdnum >> 24) & 0xff;
  cmd[i++] = 0xF0;
  resp_length = i;
  send(sockfd, cmd, resp_length, 0);
}

//解析数据包
void parse_data(unsigned char *recvbuf,int *resp_length)
{
  int i = 1;
  // 解析数据包长度
  int pkg_len = recvbuf[i++];
  pkg_len |= recvbuf[i++] << 8;
  pkg_len |= recvbuf[i++] << 16;
  pkg_len |= recvbuf[i++] << 24;
  //命令
  int cmd_no = recvbuf[i++];
  cmd_no |= recvbuf[i++] << 8;
  cmd_no |= recvbuf[i++] << 16;
  cmd_no |= recvbuf[i++] << 24;
  //数据长度
  int resp_len = recvbuf[i++];
  resp_len |= (recvbuf[i++] << 8);
  resp_len |= (recvbuf[i++] << 16);
  resp_len |= (recvbuf[i++] << 24);
  //发送是否成功
  unsigned char result = recvbuf[i++];
  switch (cmd_no)
  {
  case FTP_CMD_LS:
    printf("服务器下面的文件为:\n");
    for (int j = 0; j<resp_len-1; j++)
    {
      printf("%c", recvbuf[j+i]);
    }
    printf("\n");
    break;
  case FTP_CMD_GET:
    //数据长度
    parse_get(resp_len,recvbuf);
    printf("\n");
    break;
  case FTP_CMD_PUT:
    break;
  case FTP_CMD_BYE:
    if (strncmp((char *)&recvbuf[i], "close client socket!!!", resp_len - 1) == 0)
    {
      exit(0);
      printf("client disconnect!!!!\n");
    }
    break;
  default:
    printf("Unknown command!\n");
    break;
  }
  if(resp_len >0)
  {
    send(sockfd,recvbuf, sizeof(recvbuf) - 1, 0);
  }
  memset(recvbuf, 0, sizeof(recvbuf));
}
//处理服务器回复的数据
void parse_get(int resp_len, unsigned char *recvbuf)
{
  int j = 0, i = 1;
  memset(filenames, 0, sizeof(filenames));
  // 初始化响应
  int total_pkg_len_index = i; // 数据包的下标
  i += 4;
  // 解析命令
  i += 4;
  //文件长度
  int filecount = recvbuf[i++];
  filecount |= recvbuf[i++] << 8;
  filecount |= recvbuf[i++] << 16;
  filecount |= recvbuf[i++] << 24;
  for (int k = 0; k < filecount;k++)
  {
    if (resp_len == 1)
    {
      printf("该下载文件不存在!!!:\n");
    }
    else
    {
        int arg_len = recvbuf[i++];
        arg_len |= recvbuf[i++] << 8;
        arg_len |= recvbuf[i++] << 16;
        arg_len |= recvbuf[i++] << 24;
        char arg[1024] = {0};
        strncpy(arg, (char *)&recvbuf[i], arg_len);
        arg[arg_len] = '\0';
        printf("arg :%s", arg);
        i += arg_len;
        int data_len = recvbuf[i++];
        data_len |= recvbuf[i++] << 8;
        data_len |= recvbuf[i++] << 16;
        data_len |= recvbuf[i++] << 24;
      char newname[1024] = {0};
      sprintf(newname, "./get_%s", arg); // 生成绝对路径
      printf("newname :%s", newname);
      int fd = open(newname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd < 0)
      {
        perror("文件创建失败!!!");
        return;
      }
      int ret = write(fd, (char *)&recvbuf[i], data_len);
      if (ret < 0)
      {
        perror("写入文件失败!!!");
        close(fd);
        return;
      }
      printf("文件 %s 已成功创建并保存\n", newname);
      i += data_len;
      close(fd);
     }
  }
  resp_length = i;
}

int put_file(char *arg, int arg_len)
{
  struct dirent *dirp = NULL;
  DIR *dir = opendir(".");
  while (dirp = readdir(dir))
  {
    // 跳过两个隐藏目录
    if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
    {
      continue;
    }
    if (strncmp(arg, dirp->d_name, arg_len - 1) == 0)
    {
      int fd = open(dirp->d_name, O_RDONLY);
      if (fd < 0)
      {
        perror("open failed!");
        return -1;
      }
      int ret = read(fd, filenames, sizeof(filenames) - 1); // 读取文件内容
      // printf("filenames %s", filenames);
      if (ret < 0)
      {
        perror("read failed!");
        close(fd);
        return -1;
      }
      filenames[ret] = '\0'; // 确保字符串以null结尾
      close(fd);
      return 0;
    }
  }
  printf("不存在下载文件:%s\n", arg);
  return -1;
}
