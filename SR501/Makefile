KERN_DIR = /home/book/100ask_imx6ull-sdk/Linux-4.9.88

all:
	make -C $(KERN_DIR) M=`pwd` modules 
	$(CROSS_COMPILE)gcc -o test_sr501 test_sr501.c

clean: 
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order  test_sr501

obj-m += sr501_drv.o