## basic compiler and mythos-libcxx
### setup
```
yum install centos-release-scl centos-release-scl-rh
yum install environment-modules devtoolset-8 rh-python36 rh-git29 cmake3

rm -f /opt/rh/devtoolset-8/root/bin/sudo
/usr/share/Modules/bin/createmodule.sh /opt/rh/devtoolset-8/enable > /etc/modulefiles/devtoolset-8
/usr/share/Modules/bin/createmodule.sh /opt/rh/rh-git29/enable > /etc/modulefiles/git29
/usr/share/Modules/bin/createmodule.sh /opt/rh/rh-python36/enable > /etc/modulefiles/python36

module load devtoolset-8 python36
./3rdparty/install-libcxx.sh
```

### usage
```
module load devtoolset-8 python36
cd kernel-amd64
make
```


## compiling for XeonPhi KNC
### setup
```
/usr/share/Modules/bin/createmodule.sh /opt/intel/compilers_and_libraries_2017/linux/bin/compilervars.sh intel64 > /etc/modulefiles/intel-phi
#/usr/share/Modules/bin/createmodule.sh /opt/intel/compilers_and_libraries_2019/linux/bin/compilervars.sh intel64 > /etc/modulefiles/intel2019
```

### usage
```
module load intel-phi
./3rdparty/install-libcxx.sh
cd kernel-knc
make
# and look in the README.md
```


## debugging in qemu
### setup
```
yum install qemu-system-x86
```

### usage
```
cd kernel-amd64
make qemu
```


## debugging in bochs
### setup
```
yum install ncurses-devel readline-devel gtk2-devel xorriso grub2-tools
```

### usage
```
cd kernel-amd64
make bochs
```

