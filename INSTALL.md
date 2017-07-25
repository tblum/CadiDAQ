# CAEN USB driver

  The CAEN USB driver needs to be compiled against the running kernel and re-compiled and installed when the kernel is changed e.g. due to an update.
  To automatize this procedure, Dynamic Kernel Module Support (DKMS) can be used.
  
## DKMS on CentOS 7
* install EPEL repository
* install kernel-devel package
* install dkms:
```
yum -y install dkms
```
* copy the source code to `/usr/src`:
```
sudo cp CAENUSBdrvB-1.5.1.tgz /usr/src
cd /usr/src
sudo tar xvzf CAENUSBdrvB-1.5.1.tgz
```
* add a `dkms.conf` into the new directory with the following contents:
```
PACKAGE_NAME="CAENUSBdrvB"
PACKAGE_VERSION="1.5.1"
BUILT_MODULE_NAME[0]="$PACKAGE_NAME"
PACKAGE_NAME="CAENUSBdrvB"
DEST_MODULE_LOCATION[0]="/extra"
AUTOINSTALL="yes"
```
* register the driver with DKMS
```
sudo dkms add -m CAENUSBdrvB -v 1.5.1
```

* replace the Makefile (not fully tested! Will lack udev rules file, needs to be done by running `make install` manually or copying it manually)
```
obj-m   := CAENUSBdrvB.o

all:
        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
```

* `sudo dkms build -m CAENUSBdrvB -v 1.5.1`
* `sudo dkms install -m CAENUSBdrvB -v 1.5.1`
* to remove: `sudo dkms remove -m CAENUSBdrvB -v 1.5.1 --all`
