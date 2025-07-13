KERN_DIR = /home/book/100ask_imx6ull-sdk/Linux-4.9.88

all:
	make -C $(KERN_DIR) M=`pwd` modules 
	$(CROSS_COMPILE)gcc -o test_sr04 test_sr04.c

clean: 
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order  test_sr04

obj-m += sr04_drv.o