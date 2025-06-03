
./build/qemu-system-aarch64 -net none -L /mnt/c/users/jeffgla/Desktop/share/ \
-drive file=/mnt/c/users/jeffgla/Desktop/GCC-AARCH64/VirtualDrive.img,if=virtio \
-m 2048 -machine sbsa-ref -cpu max,sve=off,sme=off -smp 4 \
-global driver=cfi.pflash01,property=secure,value=on \
-drive if=pflash,format=raw,unit=0,file=/mnt/c/users/jeffgla/Desktop/GCC-AARCH64/SECURE_FLASH0.fd \
-drive if=pflash,format=raw,unit=1,file=/mnt/c/users/jeffgla/Desktop/GCC-AARCH64/QEMU_EFI.fd,readonly=on \
-device qemu-xhci,id=usb -device usb-tablet,id=input0,bus=usb.0,port=1 -device usb-kbd,id=input1,bus=usb.0,port=2 \
-smbios type=0,vendor="Project Mu",version="mu_tiano_platforms-v9.0.0-51-gaf696faf",date=05/14/2025,uefi=on \
-smbios type=1,manufacturer=Palindrome,product="QEMU SBSA",family=QEMU,version="9.0.0",serial=42-42-42-42 \
-smbios type=3,manufacturer=Palindrome,serial=42-42-42-42,asset=SBSA,sku=SBSA \
-serial tcp:127.0.0.1:50001,server,nowait

telnet 127.0.0.1 50001
