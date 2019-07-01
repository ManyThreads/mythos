## basic compiler and mythos-libcxx
### packages on centos7
```
yum install centos-release-scl centos-release-scl-rh
yum install environment-modules devtoolset-8 rh-python36 rh-git29 cmake3

rm -f /opt/rh/devtoolset-8/root/bin/sudo
/usr/share/Modules/bin/createmodule.sh /opt/rh/devtoolset-8/enable > /etc/modulefiles/devtoolset-8
/usr/share/Modules/bin/createmodule.sh /opt/rh/rh-git29/enable > /etc/modulefiles/git29
/usr/share/Modules/bin/createmodule.sh /opt/rh/rh-python36/enable > /etc/modulefiles/python36
```

### packages on ubuntu
TODO


### setup
```
module load devtoolset-8 python36
git submodule init 
git submodule update 
./3rdparty/mcconf/install-python-libs.sh
./3rdparty/install-libcxx.sh
```

### usage
```
module load devtoolset-8 python36
make
cd kernel-amd64
make
```


## compiling for XeonPhi KNC
### packages
```
/usr/share/Modules/bin/createmodule.sh /opt/intel/compilers_and_libraries_2017/linux/bin/compilervars.sh intel64 > /etc/modulefiles/intel-phi
#/usr/share/Modules/bin/createmodule.sh /opt/intel/compilers_and_libraries_2019/linux/bin/compilervars.sh intel64 > /etc/modulefiles/intel2019
```

### setup
```
module load intel-phi
./3rdparty/install-libcxx.sh
```

### usage
```
module load intel-phi
make
cd host-knc
make
cd ..
cd kernel-knc
make
make micrun
# CTLR-C to stop
make micstop
```


## debugging in qemu
### packages
```
yum install qemu-system-x86
```

### usage
```
cd kernel-amd64
make qemu
```


## debugging in bochs
### packages on centos7
```
yum install ncurses-devel readline-devel gtk2-devel xorriso grub2-tools
```

### packages on ubuntu
TODO libtool readline-dev
```
#sudo apt-get build-dep bochs
sudo apt install libgtk2.0-dev ncurses-dev
```

### setup
```
module load devtoolset-8 python36
./3rdparty/install-bochs.sh
```

### usage
```
module load devtoolset-8 python36
cd kernel-amd64
make bochs
```

