
//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
// File:  uarttest_device.c
//
// Serial driver test
//
// Usage:
// 
//         UartTest.exe -d [device name]
//                      -b [baud rate]
//                      -o [operation], w=write, r=read, l=loopback, a=async test
//                      -t [operation timeout (ms)]
//                      -l [transfer length]
//                      -c [cycle count]
//                      -x [duration (sec)], async test only 
//                      -y [max transfer size, 1-4096], async test only
//                      -f use flow control
//                      -m master mode
//                      -? usage
// 


#include "..\common\uarttest.cpp"
