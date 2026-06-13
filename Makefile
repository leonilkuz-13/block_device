all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

check:
	/lib/modules/$(shell uname -r)/build/scripts/checkpatch.pl --no-tree -f RAM_device.c
	