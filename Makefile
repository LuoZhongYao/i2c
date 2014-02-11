#
#	样板Makefile文件,集体项目根据实际情况进行简单修改即可
#	Author : lzy
.PHONY:all clean

#当前目录
CROSS_COMPILE = /opt/FriendlyARM/toolschain/4.5.1/bin/arm-linux-
LOCAL_PATH = $(notdir $(shell pwd))
ECHO = @
CFILES = $(wildcard *.c)
SFILES = $(wildcard *.S)
sFILES = $(wildcard *.s)

LDFLAGS += -static

ifndef	r
#$(warning "根目录未定义,将使用当前目录为根目录")
r = $(LOCAL_PATH)
endif

# .o文件集
OBJS = $(CFILES:%.c=%.o) $(SFILES:%.S=%.o) $(sFILES:%.s=%.o)
obj = i2c
all:$(obj)

$(obj):$(OBJS)
	$(ECHO) echo " LD $@"
	$(ECHO) $(LD) $(OBJS) $(LDFLAGS) -o $@ 
	$(ECHO) echo " INSTALL $@ > /data/goc/"
	$(ECHO) adb push $@ /data/goc
clean:
	$(ECHO) -rm -f *.[od] i2c
include /home/lzy/tools/Makefile.rule
-include $(OBJS:%.o=%.d)
