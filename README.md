## dependencies
```
apt install -y libyaml-cpp-dev
```
```
apt install -y libboost-all-dev libasio-dev
```

```
header only cpp log system
https://github.com/leaout/lolog

```
```
install brpc
sudo apt-get install -y git g++ make libssl-dev libgflags-dev libprotobuf-dev libprotoc-dev protobuf-compiler libleveldb-dev
wget https://codeload.github.com/apache/brpc/zip/refs/tags/1.9.0
unzip brpc-1.9.0.zip
cd brpc-1.9.0
sh config_brpc.sh --headers=/usr/include --libs=/usr/lib
make -j
```
```
wget https://libzip.org/download/libzip-1.10.1.tar.gz
tar -xvf libzip-1.10.1.tar.gz
cd libzip-1.10.1
./configure
make
sudo make install
```
## build
```
mkdir _build
cd _build
cmake ..
make
```
    
`
