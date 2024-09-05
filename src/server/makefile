TARGET := ftp_server
CC := gcc
HEADPATH :=../inc
SRCPATH :=.        #表示当前目录，即源代码文件所在的目录
LIBPATH :=../lib
LIBNAME :=main

CSRCS_CLIENT :=$(wildcard $/*.c)  
#：使用wildcard函数查找srcpath（即当前目录）下所有.c文件，并将它们赋值给变量csrcs。

$(TARGET) : $(CSRCS_CLIENT) 
	$(CC) $^ -o $@  -I$(HEADPATH) 

#这是一个目标规则，目标为$(target)（即tree），依赖为$(CSRCS)（即所有.c文件）。
#当执行make命令时,make会尝试构建main。
#命令：$(CC) $^ -o $@ 。这里代表所有不重复的依赖文件，以空格隔开，$@代表目标（即main）。

clean :
	rm -rf  $(TARGET)
#这条命令删除所有可执行文件

 