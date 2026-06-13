#!/bin/bash

MODULE="ram_disk.ko"
MODULE_NAME="ram_disk"
DEV="/dev/myramdisk"
MOUNT_DIR="/tmp/my_ramdisk_test"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Starting RAM Disk Automated Testing${NC}"

if [ ! -f "$MODULE" ]; then
    echo -e "${RED}[!] File $MODULE not found. Run 'make' first.${NC}"
    exit 1
fi

echo -e "\n[*] Performing preliminary cleanup..."
umount $MOUNT_DIR 2>/dev/null
rmmod $MODULE_NAME 2>/dev/null

echo "[*] Loading driver into the kernel..."
insmod $MODULE
if [ $? -ne 0 ]; then
    echo -e "${RED}[!] Failed to load kernel module!${NC}"
    exit 1
fi

# Wait for udev to create the device node
sleep 0.5

if [ ! -b "$DEV" ]; then
    echo -e "${RED}[!] Block device $DEV was not created!${NC}"
    rmmod $MODULE_NAME
    exit 1
fi
echo -e "${GREEN}[+] Device $DEV created successfully.${NC}"

echo "[*] Formatting device with ext4..."
mkfs.ext4 -q $DEV
if [ $? -ne 0 ]; then
    echo -e "${RED}[!] Formatting failed!${NC}"
    rmmod $MODULE_NAME
    exit 1
fi

echo "[*] Mounting device to $MOUNT_DIR..."
mkdir -p $MOUNT_DIR
mount $DEV $MOUNT_DIR
if [ $? -ne 0 ]; then
    echo -e "${RED}[!] Mounting failed!${NC}"
    rmmod $MODULE_NAME
    exit 1
fi
echo -e "${GREEN}[+] Disk mounted successfully.${NC}"

echo "[*] Running basic I/O test (write/read)..."
TEST_STRING="Hello, Linux Kernel! This is a RAM disk test."
echo "$TEST_STRING" > $MOUNT_DIR/test.txt

READ_STRING=$(cat $MOUNT_DIR/test.txt)
if [ "$TEST_STRING" == "$READ_STRING" ]; then
    echo -e "${GREEN}[+] I/O test passed successfully!${NC}"
else
    echo -e "${RED}[!] I/O test failed! Data mismatch.${NC}"
fi

echo -e "\n[*] Running write performance test (dd)..."
dd if=/dev/zero of=$MOUNT_DIR/bigfile.bin bs=1M count=10 status=progress
echo -e "${GREEN}[+] Performance test completed.${NC}"

echo -e "\n[*] Cleaning up..."
umount $MOUNT_DIR
rm -rf $MOUNT_DIR
rmmod $MODULE_NAME

echo -e "${GREEN}All tests completed successfully! Driver unloaded.${NC}"

echo -e "\n${YELLOW}Latest kernel messages (dmesg):${NC}"
dmesg | tail -n 3