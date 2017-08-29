# RTP-Sender-Filter
基于DirectShow的RTP Sender Filter实现
1.VS2017——工具集为V120-VS2013
2.jrtplib-3.11.1 jthread-1.3.3 编译为32位版
3.程序为32位程序
4.DirectShow链路图如下（控制台为RTP发送地址与本机端口）
5.RTP拆包方式为FU-A，支持VLC播放
6.x264编码filter为ffdshow codec（本filter支持输入为H264的MediaSample）
7.本程序开源（部分实现借鉴了许多CSDN博客与大牛的程序），请遵守开源协议
CSDN博客：http://blog.csdn.net/fan2273/article/details/77653700
