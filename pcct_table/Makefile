# 设置内核源代码的路径
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

# 设置模块名称
MODULE_NAME = mydriver

# 设置源代码文件名
SRC = mydriver.c

# 设置obj-m变量
obj-m += $(MODULE_NAME).o

# all target默认执行编译
all:
        make -C $(KERNELDIR) M=$(PWD) modules

# 安装模块
install:
        make -C $(KERNELDIR) M=$(PWD) modules_install

# 清除目标文件和模块
clean:
        make -C $(KERNELDIR) M=$(PWD) clean
