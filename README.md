# Linux memory monitor _qznusnih_
This Linux kernel module monitors a configurable byte of user space memory and logs the backtrace that led to the access, with separate handling for read and write operations.

# How it works
The kernel module is configured to be loadable, i.e., it can be attached to or detached from the kernel at will. Once attached, it creates a file interface with user space at **/sys/kernel/qznusnih/set_address_to_debug**. Applications with root privileges can write to this interface to specify a virtual address (in user space) and the corresponding process ID to which the memory is allocated.
This triggers monitoring of all read and write accesses to the specified byte, with separate handling of read and write operations. Every access is logged along with a backtrace (the sequence of function calls that led to the access) which is essential for debugging.

# How to install
This kernel module was primarily designed and has been mostly tested on Linux Yocto for embedded devices using the x86 architecture.
If you intend to use it in this context to deploy a Linux Yocto image with this kernel module, you may decide to follow these steps:
1. Clone the repository of the Poky build system of Linux Yocto on your host machine (where you’ll build the Linux image), and set up its environment.
```
git clone git://git.yoctoproject.org/poky
cd poky
source oe-init-build-env
```

2. Open the file `poky/build/conf/local.conf` and uncomment the line `MACHINE ?= "qemux86"`. You may also want to change it to: `MACHINE = "qemux86"`.
3. In the same file, add the following line to make Linux include your kernel module (named _qznusnih_) in the image: `IMAGE_INSTALL:append = " qznusnih"`.
4. Return to the `poky` folder and
```
bitbake-layers create-layer meta-lkm-layer
bitbake-layers add-layer meta-lkm-layer   # adds to poky/build/conf/bblayers.conf
cd ../..                                  # (out of poky/)
```
5. Then, clone this utility and merge its `meta-lkm-layer/` subfolder into `poky/meta-lkm-layer/`, removing any unneeded subfolders as you go.
```
git clone https://github.com/yuryatin/linux-memory-monitor-qznusnih.git
cp -rf linux-memory-monitor-qznusnih/meta-lkm-layer/ poky/
rm -rf poky/meta-lkm-layer/recipes-example
cd poky/
```

6. Building Linux Yocto from scratch can take several hours. If you're doing this on a server, it's a good idea to use a tmux session so you can disconnect and resume later if needed.
```
tmux new -s mysession
bitbake core-image-minimal
```

7. If the Linux build completes without errors, you will be able to launch Linux Yocto in an x86 virtual machine using QEMU.
```
runqemu qemux86 nographic
```
# How to attach and detach this kernel module

After launching your Linux Yocto system with this kernel module incorporated, you can attach the module with `modprobe qznusnih`, detach it with `rmmod qznusnih` and check if the module is currently loaded using `lsmod | grep qznusnih`. Before loading the module, it’s recommended to limit spontaneous kernel message output in the terminal by running `dmesg -n 1`.

<img width="592" height="693" alt="first_launch_in_image" src="https://github.com/user-attachments/assets/f2b30eb3-9419-4497-bb8e-830c9928bede" />

# Backtraces
Below are examples of backtraces captured when a monitored byte was written.


<img width="592" height="363" alt="header_backtrace" src="https://github.com/user-attachments/assets/118c1ecc-4908-4d7e-944b-8ceaf4c8dfe4" />


<img width="592" height="478" alt="backtrace" src="https://github.com/user-attachments/assets/dddc9076-f380-458d-966e-3fd78623109f" />

# How to test this utility
In the `user_space_tester` folder, you will find a small user-space utility. When run with root privileges, it sends the virtual address of one of its own bytes along with its process ID to **/sys/kernel/qznusnih/set_address_to_debug**. You can monitor the utility’s activity by checking the kernel messages with `dmesg | tail -n 30`.
