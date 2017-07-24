# Requirements

* CAEN digitizer libraries
* Boost libraries (version >= v1.54.0)
  
  To install, use your distribution's package manager (e.g. `yum install boost-devel`) or download and install the newest version from the [Boost homepage](http://www.boost.org/users/download/)
  
* CMake (version >= 3.6)

  The version of CMake should have been released after the Boost version installed above (to correctly determine dependencies). If you run into issues when executing CMake, please download and install the latest version from the [CMake Homepage](https://cmake.org/install/)

# to run the test program:
```
cd build
cmake ..
make
./cadidaq -f ../mytest.ini
```
