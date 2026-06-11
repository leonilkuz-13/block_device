# block_device

---

A block device developed as part of educational practice. 
This driver was developed and tested on **Fedora Linux** with kernel version 6.19.7-200.fc43.x86_64. 

---

## Automated Test

The easiest way to test the RAM block device is to use the provided automated bash script.

1. Build the kernel module:

    ```bash
    make
    ```

2. Run the automated test:

    ```bash
    chmod +x test_driver.sh
    sudo ./test_driver.sh
    ```

This script will load the module, format the disk, mount it, run basic read/write and performance tests, and cleanly unmount/unload the device, leaving your system clean.

---

## Full Manual Usage

If you want to understand how the driver works under the hood, here is the complete step-by-step guide to manually initialize, format, and use the RAM block device.

1. Build module

    Compile the driver using the provided Kbuild/Makefile system:

    ```bash
    make
    ```

    This generates the `ram_disk.ko` kernel object file.

2. Load the Module into the Kernel

    Insert the compiled module into the Linux kernel:

    ```bash
    sudo insmod ram_disk.ko
    ```

    Verify that the module is loaded and the block device node is created:

    ```bash
    lsmod | grep ram_disk
    lsblk | grep myramdisk
    ```

    You can also check the kernel logs for the initialization message:

    ```bash
    dmesg | tail -n 5
    ```

3. Format the Device

    Before you can store files on the block device, it needs a file system. Let's format it to `ext4`:

    ```bash
    sudo mkfs.ext4 /dev/myramdisk
    ```

4. Mount the Device

    Create a mount point (a directory) and mount the RAM disk to it:

    ```bash
    mkdir -p /tmp/ramdisk
    sudo mount /dev/myramdisk /tmp/ramdisk
    ```

    Verify the mount:

    ```bash
    df -h | grep myramdisk
    ```

5. Test I/O Operations

    Now the device acts like a regular physical drive. You can write and read data:

    ```bash
    echo "Hello from the custom RAM Block Device!" | sudo tee /tmp/ramdisk/test.txt
    cat /tmp/ramdisk/test.txt
    ```

6. Cleanup (Unmount and Unload)

    Once you are done, unmount the file system and remove the driver from the kernel to free up the allocated RAM:

    ```bash
    sudo umount /tmp/ramdisk
    sudo rmmod ram_disk
    ```

    Check kernel logs to ensure the device was removed successfully:

    ```bash
    dmesg | tail -n 2
    ```

---

## Code Quality

To validate the RAM_device.c source code against the Linux Kernel Coding Style, run:

```bash
make check

## License & Author

- License: GPL-2.0
- Author: Leonid Kuzmischev