#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include "common.h"
#include "ftp_server.h"
#define   Max_File_Lenth 1024

int sockfd;
int confd;  //accept变量
int filecount;
int flag = 1;
struct sockaddr_in client; // 用来保存客户端的地址
socklen_t len ;
unsigned char recvbuf[4096] = {0};//接收数据的数组
unsigned char filedata[4096] = {0}; //存放ls的文件名
unsigned char filenames[4096] = {0}; //存放文件数据
int filelength = 0;
unsigned char resp[10240] = {0}; // 回复包
int resp_length= 0; //回复数据的长度
int main()      //./tcp_server IP PORT
{
  // 1.创建一个套接字
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sockfd)
  {
    perror("socket failed");
    return -1;
  }
  //端口地址复用
  int s = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &s, 4);
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &s, 4);
  // 2.绑定服务器的地址
  struct sockaddr_in serv;
  serv.sin_family = AF_INET;
  serv.sin_port = htons(atoi("7777"));
  serv.sin_addr.s_addr = inet_addr("172.70.0.154");
  int ret = bind(sockfd, (struct sockaddr *)&serv, sizeof(serv));
  if (-1 == ret)
  {
    perror("bind failed");
    close(sockfd);
    return -1;
  }
  // 3.进入监听模式
  ret = listen(sockfd, 1);
  if (-1 == ret)
  {
    perror("listen failed");
    close(sockfd);
    return -1;
  }
    // 每当有一个客户端连接成功之后，开辟一个子进程去与其通信，父进程依旧等待新的客户端
  while (1)
  {
    len = sizeof(client);
    confd = accept(sockfd, (struct sockaddr *)&client, &len);
    if (confd == -1)
    {
      perror("accept failed");
      return -1;
    }
    printf("connect %s[port:%d] success!!\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port)); // 打印客户端的信息
    while (1)
    {
      if (flag == 0)
      {
        break;
      }
      memset(recvbuf, 0, sizeof(recvbuf));
      recv(confd, recvbuf, sizeof(recvbuf) - 1, 0);
      parse_cmd(recvbuf);
    }
  }
}

/* 解析客户端发送过来的数据包,判断命令*/
void parse_cmd(unsigned char *recvbuf)
{
  int i=0,j=0;
  // 解析长度 0xc0包头
  resp[j++] = recvbuf[i++];  //0xc0
  // 数据包长度
  i += 4;
  // 解析命令
  int cmd_no = recvbuf[i++];
  cmd_no |= recvbuf[i++] << 8;
  cmd_no |= recvbuf[i++] << 16;
  cmd_no |= recvbuf[i++] << 24;
  switch (cmd_no)
  {
  case FTP_CMD_LS:
    get_filename();
    cmd_ls(recvbuf,&resp_length);
    // send(confd, resp, sizeof(resp) - 1, 0);
    break;
  case FTP_CMD_GET:
    cmd_get(recvbuf, &resp_length);
    // send(confd, resp, sizeof(resp) - 1, 0);
    break;
  case FTP_CMD_PUT:
    cmd_put(recvbuf, &resp_length);
    // send(confd, resp, sizeof(resp) - 1, 0);
    break;
  case FTP_CMD_BYE:
    cmd_bye(recvbuf, &resp_length);
    // send(confd, resp, sizeof(resp) - 1, 0);
    break;
  default:
    printf("Unknown command!\n");
    break;
  }
  if (resp_length > 0)
  {
    send(confd, resp, resp_length, 0);
   }
}

//执行ls命令
void cmd_ls(unsigned char *recvbuf, int *resp_length)
{
  int j = 0,i=1;
  int x = strlen((char *)filedata);
  int pkg_len = 13 + x;
  char result = 1;
  int resp_len = 1 + x;
  //长度
  resp[j++] = 0xC0;
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  //命令
  int cmd_no = recvbuf[i++];
  cmd_no |= recvbuf[i++] << 8;
  cmd_no |= recvbuf[i++] << 16;
  cmd_no |= recvbuf[i++] << 24;
  resp[j++] = cmd_no & 0xff;
  resp[j++] = (cmd_no >> 8) & 0xff;
  resp[j++] = (cmd_no >> 16) & 0xff;
  resp[j++] = (cmd_no >> 24) & 0xff;
  //文件长度
  resp[j++] = resp_len & 0xff;
  resp[j++] = (resp_len >> 8) & 0xff;
  resp[j++] = (resp_len >> 16) & 0xff;
  resp[j++] = (resp_len >> 24) & 0xff;
  resp[j++] = result ;
  strncpy((char *)&resp[j], (char *)filedata, x);
  j += x;
  resp[j++] = 0xF0;
  *resp_length = j;
}

//执行get命令
void cmd_get(unsigned char *recvbuf, int *resp_length)
{
  int j = 0, i = 0;
  memset(filenames, 0, sizeof(filenames));
  // 初始化响应
  resp[j++] = recvbuf[i++];
  int total_pkg_len_index = j; // 数据包的下标
  //数据长度
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  // 解析命令
  int cmd_no = recvbuf[i++];
  cmd_no |= recvbuf[i++] << 8;
  cmd_no |= recvbuf[i++] << 16;
  cmd_no |= recvbuf[i++] << 24;
  
  resp[j++] = cmd_no & 0xff;
  resp[j++] = (cmd_no >> 8) & 0xff;
  resp[j++] = (cmd_no >> 16) & 0xff;
  resp[j++] = (cmd_no >> 24) & 0xff;

 //文件数
  int filecount = recvbuf[i++];
  filecount |= recvbuf[i++] << 8;
  filecount |= recvbuf[i++] << 16;
  filecount |= recvbuf[i++] << 24;
  resp[j++] = filecount & 0xff;
  resp[j++] = (filecount >> 8) & 0xff;
  resp[j++] = (filecount >> 16) & 0xff;
  resp[j++] = (filecount >> 24) & 0xff;

  for (int f = 0; f < filecount; f++)
  {
    // 解析客户端需要的文件名长度
    int arg_len = recvbuf[i++];
    arg_len |= recvbuf[i++] << 8;
    arg_len |= recvbuf[i++] << 16;
    arg_len |= recvbuf[i++] << 24;

    resp[j++] = arg_len & 0xff;
    resp[j++] = (arg_len >> 8) & 0xff;
    resp[j++] = (arg_len >> 16) & 0xff;
    resp[j++] = (arg_len >> 24) & 0xff;

    char arg[1024] = {0};
    strncpy(arg, (char *)&recvbuf[i], arg_len);
    strncpy((char *)&resp[j], arg, arg_len);
    arg[arg_len] = '\0';
    i += arg_len;
    j += arg_len;
    printf("客户端请求下载的文件: %s\n", arg);
    if (down_file(arg, arg_len) == 0)
    {
      int resp_len = strlen(filenames);
      resp[j++] = resp_len & 0xff;
      resp[j++] = (resp_len >> 8) & 0xff;
      resp[j++] = (resp_len >> 16) & 0xff;
      resp[j++] = (resp_len >> 24) & 0xff;
      strncpy((char *)&resp[j], filenames, resp_len);
      j += resp_len;
    }
    else
    {
      int resp_len = 1;
      resp[j++] = resp_len & 0xff;
      resp[j++] = (resp_len >> 8) & 0xff;
      resp[j++] = (resp_len >> 16) & 0xff;
      resp[j++] = (resp_len >> 24) & 0xff;
    }
  }
  char result = 1;
  resp[j++] = result;
  resp[j++] = 0xF0;
  // 数据包的总长度
  int total_pkg_len = j;
  resp[total_pkg_len_index] = total_pkg_len & 0xff;
  resp[total_pkg_len_index + 1] = (total_pkg_len >> 8) & 0xff;
  resp[total_pkg_len_index + 2] = (total_pkg_len >> 16) & 0xff;
  resp[total_pkg_len_index + 3] = (total_pkg_len >> 24) & 0xff;
  *resp_length = j;
  printf("\n");
}

//执行put命令
void cmd_put(unsigned char *recvbuf, int *resp_length)
{
  int i = 1;
  memset(filenames, 0, sizeof(filenames));
  // 初始化响应
  int pkg_len = recvbuf[i++];
  pkg_len |= recvbuf[i++] << 8;
  pkg_len |= recvbuf[i++] << 16;
  pkg_len |= recvbuf[i++] << 24;

  // 解析命令
  int cmd_no = recvbuf[i++];
  cmd_no |= recvbuf[i++] << 8;
  cmd_no |= recvbuf[i++] << 16;
  cmd_no |= recvbuf[i++] << 24;

 //要传输的文件数量
  int filecount = recvbuf[i++];
  filecount |= recvbuf[i++] << 8;
  filecount |= recvbuf[i++] << 16;
  filecount |= recvbuf[i++] << 24;
  for (int k = 0;k<filecount;k++)
  {
    //解析客户端要上传文件名长度
    int arg_len = recvbuf[i++];
    arg_len |= recvbuf[i++] << 8;
    arg_len |= recvbuf[i++] << 16;
    arg_len |= recvbuf[i++] << 24;
    // 解析要上传的文件名
    char arg[1024] = {0};
    strncpy(arg, (char *)&recvbuf[i], arg_len);
    arg[arg_len] = '\0';
    i += arg_len;
    // 解析客户端要上传的数据长度
    int put_len = recvbuf[i++];
    put_len |= recvbuf[i++] << 8;
    put_len |= recvbuf[i++] << 16;
    put_len |= recvbuf[i++] << 24;
    printf("数据长度 %d", put_len);
    if (put_len == 0)
    {
      printf("该上传文件不存在!!!:\n");
    }
    else
    {
      char newname[1024] = {0};
      sprintf(newname, "./put_%s", arg); // 生成绝对路径
      printf("newname: %s\n", newname);  // 检查生成的文件名
      int fd = open(newname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd < 0)
      {
        perror("文件创建失败!!!");
        return;
      }
      int ret = write(fd, (char *)&recvbuf[i], put_len - 1);
      if (ret < 0)
      {
        perror("写入文件失败!!!");
        close(fd);
        return;
      }
      i += put_len;
      printf("文件%s上传成功!!!", arg);
      close(fd);
    }
    printf("\n");
  }
  printf("\n");
}

// 执行bye命令
void cmd_bye(unsigned char *recvbuf, int *resp_length)
{
  int j = 0, i = 1;
  int x = strlen("close client socket!!!");
  int pkg_len = 13 + x;
  char result = 1;
  int resp_len = 1 + x;
  memset(filenames, 0, sizeof(filenames));
  // 长度
  resp[j++] = 0xC0;
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  resp[j++] = recvbuf[i++];
  // 命令
  int cmd_no = recvbuf[i++];
  cmd_no |= recvbuf[i++] << 8;
  cmd_no |= recvbuf[i++] << 16;
  cmd_no |= recvbuf[i++] << 24;
  resp[j++] = cmd_no & 0xff;
  resp[j++] = (cmd_no >> 8) & 0xff;
  resp[j++] = (cmd_no >> 16) & 0xff;
  resp[j++] = (cmd_no >> 24) & 0xff;
  // 文件长度
  resp[j++] = resp_len & 0xff;
  resp[j++] = (resp_len >> 8) & 0xff;
  resp[j++] = (resp_len >> 16) & 0xff;
  resp[j++] = (resp_len >> 24) & 0xff;
  resp[j++] = result;
  strncpy((char *)&resp[j], "close client socket!!!", x);
  j += x;
  resp[j++] = 0xF0;
  *resp_length = j;
  send(confd, resp, *resp_length, 0);
  // flag = 0;
  close(confd);
  flag = 0;
  printf("Client disconnected.\n");
}

//将客户端要下载的文件保存到filenames
int down_file(char *arg,int arg_len)
{
  struct dirent *dirp = NULL;
  DIR *dir = opendir(".");
  while (dirp=readdir(dir))
  {
    // 跳过两个隐藏目录
    if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
    {
      continue;
    }
    // printf("dirp->d_name:%s\n", dirp->d_name);
    if (strncmp(arg, dirp->d_name, arg_len-1) == 0)
    {
      int fd = open(dirp->d_name, O_RDONLY);
      if (fd < 0)
      {
        perror("open failed!");
        return -1;
      }
      // printf("sizeof %ld", sizeof(filenames));
      int ret = read(fd, filenames, sizeof(filenames)-1); // 读取文件内容
      printf("客户端要下载的数据为 %s\n", filenames);
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
  filecount = 0;
  return -1;
}

//获取当前目录下所有文件名
int get_filename()
{
  DIR *dir = opendir(".");
  if (dir == NULL)
  {
    perror("opendir failed");
    return -1;
  }
  // 读取目录
  struct dirent *dirp;
  int i = 0;
  while (dirp = readdir(dir))
  {
    // 跳过隐藏目录 . ..
    if (strncmp(dirp->d_name, ".", 1) == 0 || strncmp(dirp->d_name, "..", 2) == 0)
    {
      continue;
    }
    int name_len = strlen(dirp->d_name);
    if (i + name_len +1 < sizeof(filedata))
    {
      strcpy((char *)&filedata[i], dirp->d_name);
      i += name_len;
      filedata[i++] = '\n';
    }
  }
  filedata[i] = '\0';
  filelength = i;
  // printf("Filenames: %s\n", filedata);
  // 关闭目录
  closedir(dir);
}