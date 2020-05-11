#!/bin/bash

hiredis_pkg_name=hiredis-master
jsoncpp_pkg_name=jsoncpp-src-0.5.0.tar.gz
log4cpp_pkg_name=log4cpp-1.1.3.tar.gz
scons_pkg_name=scons-3.1.1.zip

echo "name = $hiredis_pkg_name $jsoncpp_pkg_name $log4cpp_pkg_name $scons_pkg_name" 

function sethiredis()
{
    echo "start setup hiredis"
    zip_name=$hiredis_pkg_name".zip"
    unzip $zip_name
    cd $hiredis_pkg_name
    make
    make install
    mkdir /usr/lib/hiredis
    cp libhiredis.so /usr/lib/hiredis #将动态连接库libhiredis.so至/usr/lib/hiredis
    mkdir /usr/include/hiredis
    cp hiredis.h /usr/include/hiredis
    echo '/usr/local/lib' >>/etc/ld.so.conf 
    ldconfig
}

function setlog4cpp()
{
    echo "start setup log4cpp"
    rm -rf log4cpp
    tar -zxvf $log4cpp_pkg_name
    cd log4cpp/
    ./configure
    make
    make install
    # 头文件在/usr/local/include/log4cpp， 库文件在/usr/local/lib
}

function main()
{
    if [ ! -d /usr/include/hiredis ]
    then
        echo "sethiredis"
        # sethiredis
    else
        echo "already setup hiredis"
    fi

    if [ ! -d /usr/local/include/log4cpp ]
    then
        echo "setlog4cpp"
        # setlog4cpp
    else
        echo "already setup log4cpp"
    fi
}

main
