一,环境搭建
1, 用老师给的Linux内核镜像和文件系统在开发板中搭建linux环境
	1> 将zImage拷贝到/tftpboot中
	2> 将文件系统rootfs.tar.gz拷贝到/opt
		1) 解压
			lpf@ubuntu:/opt$ tar -xvf rootfs.tar.gz
		2) 修改NFS配置文件
				lpf@ubuntu:/opt/rootfs$ sudo vim /etc/exports
			添加下面一行:
				/opt/rootfs *(subtree_check,rw,no_root_squash,async)
		3) 重启服务:
				lpf@ubuntu:~$ sudo /etc/init.d/nfs-kernel-server restart
				lpf@ubuntu:~$ sudo exportfs -a
	3> 配置uboot参数:
		 set serverip 192.168.7.5       //tftp下载的服务器IP
		 set ipaddr 192.168.7.6			//开发板IP
		 set gatewayip 192.168.7.1
		 set netmask 255.255.255.0
		 set ethaddr 11:22:33:44:55:ee
		 ------------------------------------------
		 set bootcmd tftp 40800000 zImage \;bootm 40800000       	//下载zImage,并启动内核
		 set bootargs root=/dev/nfs nfsroot=192.168.7.106:/opt/rootfs ip=192.168.7.166 init=/linuxrc console=ttySAC0,115200
		 save
	4> 配置交叉编译器:
		1)将交叉编译器拷贝到/opt中,并解压:
			lpf@ubuntu:/opt$ tar -xvf toolchain-4.5.1-farsight.tar.bz2
		2) 配置交叉编译器:
			sudo vim /etc/environment
			在PATH的后面添加:
				/opt/toolchain-4.5.1-farsight/bin
			如:
				PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/opt/toolchain-4.5.1-farsight/bin"
			使之生效:
				source /etc/environment
			测试:
				在命令行:
					arm-n  按tab键观察是否补全    如:arm-none-linux-gnueabi-
2,Linux内核移植
	1> 获取Linux源码
		1) 从官网下载
			 www.kernel.org
		2) 老师发的:
			 E:\peter\1709\初级驱动\source\linux-3.0.8.tar.bz2   -----大家可以通过飞秋下载
	2> 在ubuntu中创建相应的目录:
		lpf@ubuntu:~$ mkdir s5pv210
		lpf@ubuntu:~$ mkdir s5pv210/kernel
		lpf@ubuntu:~$ mkdir s5pv210/driver
	3> 将内核源码拷贝到/home/lpf/s5pv210/kernel,并解压
		lpf@ubuntu:~/s5pv210/kernel$ tar -xvf linux-3.0.8.tar.bz2
	-------------------------------------------------------------------
	以下的操作都是在: 解压之后的linux目录下----/home/lpf/s5pv210/kernel/linux-3.0.8
	4> 配置交叉编译器:
			vim Makefile
		 修改以下两行:
		 195 ARCH            ?= arm
		 196 CROSS_COMPILE   ?= arm-none-linux-gnueabi-
	5> 选择平台:soc----s5pv210
		方法一:
			cp arch/arm/configs/s5pv210_defconfig .config
		方法二:  推荐方法
			make s5pv210_defconfig
	6> 与平台(开发板)无关的配置
		-------------------------------------------------------------------
		注意,如果执行make menuconfig,出现下面的错误提示:
		lpf@ubuntu:~/s5pv210/kernel/linux-3.0.8$ make menuconfig
		 *** Unable to find the ncurses libraries or the
		 *** required header files.
		 *** 'make menuconfig' requires the  libraries.
		 *** 
		 *** Install ncurses (ncurses-devel) and try again.
		 *** 
		make[1]: *** [scripts/kconfig/dochecklxdialog] Error 1
		make: *** [menuconfig] Error 2
		lpf@ubuntu:~/s5pv210/kernel/linux-3.0.8$ 
		说明当前系统缺少库:libncurses5-dev ,此时只需安装此库即可:
			sudo apt-get install libncurses5-dev
		-----------------------------------------------------------------------
		make menuconfig
		进行以下的选择:
			1)选择串口COM1
				System Type  --->
					 (0) S3C UART to use for low-level messages
			2) 选择支持网络协议
				[*] Networking support  ---> 
					  Networking options  ---> 
							 <*> Packet socket 
							 <*> Unix domain sockets 
							 [*] TCP/IP networking
							 [*]   IP: kernel level autoconfiguration
			3) 选择支持网络文件系统
				File systems  ---> 
					 [*] Network File Systems (NEW)  --->
							<*>   NFS client support                                                                                │ │  
                                    [*]     NFS client support for NFS version 3                                                            │ │  
                                    [*]       NFS client support for the NFSv3 ACL protocol extension
									[*]   Root file system on NFS
	7> 编译内核:
		 make -j2
	8> 将编译好的镜像zImage拷贝到/tftpboot中
		cp arch/arm/boot/zImage /tftpboot/
	9> 重启开发板:
		出现错误:
			VFS: Unable to mount root fs via NFS, trying floppy.
			VFS: Cannot open root device "nfs" or unknown-block(2,0)
			Please append a correct "root=" boot option; here are the available partitions:
			Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(2,0)
			[<80032564>] (unwind_backtrace+0x0/0xf0) from [<8024c978>] (panic+0x70/0x19c)
			[<8024c978>] (panic+0x70/0x19c) from [<80008de4>] (mount_block_root+0x15c/0x210)
			[<80008de4>] (mount_block_root+0x15c/0x210) from [<80009030>] (mount_root+0xa8/0xc4)
			[<80009030>] (mount_root+0xa8/0xc4) from [<800091b0>] (prepare_namespace+0x164/0x1bc)
			[<800091b0>] (prepare_namespace+0x164/0x1bc) from [<80008aa0>] (kernel_init+0xe4/0x118)
			[<80008aa0>] (kernel_init+0xe4/0x118) from [<8002dee0>] (kernel_thread_exit+0x0/0x8)
			挂载失败,原因是编译内核时,没有将网卡驱动编译进内核
	10> 重新编译内核,将网卡驱动编译进去  -------- 与平台相关的配置
		1) 修改内核源码 ------ 驱动相关的源程序
			vi arch/arm/mach-s5pv210/mach-smdkv210.c

			修改结构体数组smdkv210_dm9000_resources为：

				static struct resource smdkv210_dm9000_resources[] = {
						[0] = {
								.start = 0x88000000,
								.end = 0x88000000 + 0x3,
								.flags = IORESOURCE_MEM,   //地址端口
						},
						[1] = {
								.start = 0x88000000 + 0x4,
								.end = 0x88000000 + 0x4 + 0x3,
								.flags = IORESOURCE_MEM,   //数据端口
						},
						[2] = {
								.start = IRQ_EINT(10),
								.end = IRQ_EINT(10),
								.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
						},
				};
				static struct dm9000_plat_data smdkv210_dm9000_platdata = {
						.flags          = DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM,
						.dev_addr       = { 0x00, 0x09, 0xc0, 0xff, 0xec, 0xYY}, /* YY为座位号，如果座位号小于10, 则在前面添加0， 比如座位号为6，则可以填06 */
				};
	 
			修改函数smdkv210_dm9000_init为：

				static void __init smdkv210_dm9000_init(void)
				{
					unsigned int tmp;
					gpio_request(S5PV210_MP01(1), "nCS1");
					s3c_gpio_cfgpin(S5PV210_MP01(1), S3C_GPIO_SFN(2));
					gpio_free(S5PV210_MP01(1));
						tmp = (5 << S5P_SROM_BCX__TACC__SHIFT);
					__raw_writel(tmp, S5P_SROM_BC1);
					tmp = __raw_readl(S5P_SROM_BW);
					tmp &= (S5P_SROM_BW__CS_MASK << S5P_SROM_BW__NCS1__SHIFT);
					tmp |= (1 << S5P_SROM_BW__NCS1__SHIFT);
					__raw_writel(tmp, S5P_SROM_BW);
				}
		2) 在make menuconfig中选择网卡驱动
			Device Drivers  ---> 
					[*] Network device support  --->  
							 [*]   Ethernet (10 or 100Mbit)  --->
									<*>   DM9000 support                                                                                    │ │  
                                    (4)     DM9000 maximum debug level (NEW) 
		3) 重新编译内核	
			make -j2
		4) 将编译好的内核镜像zImage再次拷贝到/tftpboot中
			cp arch/arm/boot/zImage /tftpboot/
		5) 重启开发板:
				dm9000 dm9000: eth0: link down
				IP-Config: Guessing netmask 255.255.255.0
				IP-Config: Complete:
					 device=eth0, addr=192.168.7.6, mask=255.255.255.0, gw=255.255.255.255,
					 host=192.168.7.6, domain=, nis-domain=(none),
					 bootserver=255.255.255.255, rootserver=192.168.7.5, rootpath=
				dm9000 dm9000: eth0: link up, 100Mbps, full-duplex, lpa 0x4DE1
				VFS: Mounted root (nfs filesystem) on device 0:10.
				Freeing init memory: 124K
				==================^_^ Finished===================

				Please press Enter to activate this console.
				[root@farsight /]#
		--------------------------------------内核移植OK-------------------------------------------
二,驱动开发-----简单实例

1,模块的基本组成
	//头文件
	#include <linux/init.h>
	#include <linux/module.h>


	//模块加载函数
	static int __init hello_init(void)
	{
		  printk("--------%s-------------\n",__FUNCTION__);
		  return 0;
	}
	//模块卸载函数
	static void __exit hello_exit(void)
	{
		  printk("--------%s-------------\n",__FUNCTION__);
	}


	//模块的声明
	module_init(hello_init);
	module_exit(hello_exit);

	//认证
	MODULE_LICENSE("GPL");
2,模块编译------编写Makefile文件:
	#指定内核源码的路径
	KERNEL_DIR = /home/lpf/s5pv210/kernel/linux-3.0.8
	CUR_DIR = $(shell pwd)

	all:
			# 进入内核目录，并告诉内核当前目录下的指定的源文件作为内核模块编译
			make -C $(KERNEL_DIR) M=$(CUR_DIR) modules

	clean:
			make -C $(KERNEL_DIR) M=$(CUR_DIR) clean
			

	#指定将当前目录下的哪个源文件作为内核模块编译
	obj-m = hello_drv.o
3,模块加载和卸载
	1> 将编译好的ko文件拷贝到/
		cp hello_drv.ko /opt/rootfs/drv_modules/
	2> 模块加载:
		[root@farsight /drv_modules]# insmod hello_drv.ko
		--------hello_init-------------
		[root@farsight /drv_modules]#
	3> 查看模块信息
			[root@farsight /drv_modules]# lsmod
			hello_drv 747 0 - Live 0x7f000000
	4> 卸载模块:
			[root@farsight /drv_modules]# rmmod hello_drv
			--------hello_exit-------------
三, 用source insight编写程序和查看代码
	将si_linux308-ori.tgz 拷贝到内核源码中:  
		\\192.168.7.5\lpf\s5pv210\kernel\linux-3.0.8
	解压:
		lpf@ubuntu:~/s5pv210/kernel/linux-3.0.8$ tar -xvf si_linux308-ori.tgz 
		linux308-ori.IAB
		linux308-ori.IAD
		linux308-ori.IMB
		linux308-ori.IMD
		linux308-ori.PFI
		linux308-ori.PO
		linux308-ori.PR
		linux308-ori.PRI
		linux308-ori.PS
		linux308-ori.WK3
	在共享目录中,双击linux308-ori.PR可以打开工程
四,模块的特性
1,模块传参 ------ 在加载模块时,可以传递参数进去
	加载模块:
		insmod hello_drv.ko sno=1002 name="peter"
		--------hello_init-------------
		sno = 1002
		name = peter
	程序中:
		module_param(sno,int,0644);
		module_param(name,charp,0644);
		格式: module_param(变量名,类型名称,权限);
	关于变量权限:
		每个需要传递参数的变量,在文件系统中都会自动生成一个同名的文件,该权限指的就是同名的文件的权限:
	[root@farsight /drv_modules]# ls /sys/module/hello_drv/parameters/ -l
	total 0
	-rw-r--r--    1 0        0             4096 Jan  1 01:05 name
	-rw-r--r--    1 0        0             4096 Jan  1 01:05 sno


2,模块调用 ------ 多个模块中的函数,可以相互调用
	
	A.ko   						B.ko
	
	fun();						int fun(void)
								{
									......	
								}
	例如:
		被调用模块的实现:
			#include <linux/init.h>
			#include <linux/module.h>


			int myadd(int a,int b)
			{
				return a + b;
			}
			EXPORT_SYMBOL(myadd);

			int mysub(int a,int b)
			{
				return a - b;
			}
			EXPORT_SYMBOL(mysub);

			MODULE_LICENSE("GPL");
		调用模块的实现:
			
		#include "myfun.h"
	
		printk("5 + 3 = %d\n",myadd(5,3));
		printk("5 - 3 = %d\n",mysub(5,3));
				
	编译:  Makefile中
		 obj-m = hello_drv.o
		 obj-m += myfun.o
	加载模块:
		[root@farsight /drv_modules]# insmod myfun.ko
		[root@farsight /drv_modules]# insmod hello_drv.ko
		--------hello_init-------------
		sno = 1001
		name = 张三
		5 + 3 = 8
		5 - 3 = 2

				
				
				