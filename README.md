# fsstat

## Installation

Clone the repository and build using your favorite C++ compiler.

```bash
$ git clone https://github.com/ianbrault/fsstat.git
$ cd fsstat
$ g++ --std=c++11 -march=native -O2 -o fsstat fsstat.cpp
$ cp fsstat /usr/local/bin
```

## Usage

If you ran the last command under Installation, the `fsstat` binary should be installed along your `PATH`, and you can simply run `fsstat`. To add the program output to your shell load, add the command to your config file (i.e. `echo fsstat >> ~/.bashrc`). The output should look something like below (note that the usage bar is colored but the colors won't show up in the README):

```
Filesystem            Size      Used     Avail      Used    Mount
  /dev/disk1s1     250.7GB   137.7GB   109.1GB     54.9%        /
  [=============================================================]
```
