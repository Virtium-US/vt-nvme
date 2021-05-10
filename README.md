# VT NVMe

An "[nvme-cli](https://github.com/linux-nvme/nvme-cli)"-like utility used to manage vendor specific functions of Virtium NVMe devices on Windows and Linux.

## Getting Started

To use the tool, clone the repository and build using the following commands:
```
$ git clone https://github.com/Virtium-US/vt-nvme.git && cd vt-nvme
$ make
$ ./build/vtnvme.exe help

Usage: vtnvme COMMAND [ARGS]

Supported commands:
  help: vtnvme help - Displays this menu
  list: vtnvme list - Prints list of connected NVMe devices
  get-op: vtnvme get-op DEVICE - Returns current over-provisioning information (ex: vtnvme get-op /dev/nvme0n1)
  set-op: vtnvme set-op DEVICE OP% - Sets over-provisioning percentage (ex: vtnvme set-op /dev/nvme0n1 40)
```

For example, to see the over-provisioning percentage for a specific drive, you can run the following commands: 
```
$ ./build/vtnvme.exe list
NVMe devices found:
- \\.\PhysicalDrive1 00000000 VSFCN8CI1920G-ENG
$ ./build/vtnvme.exe get-op \\\\.\\PhysicalDrive1
Current OP:      0
OP on next boot: 0
```
***NOTE: commands like `get/set-op` are vendor specific!***

For Windows, you can build vt-nvme with [Cygwin](https://cygwin.com/index.html). Make sure to install the package [`cygport`](https://cygwin.com/packages/summary/cygport.html) to get a bunch of useful packages.
