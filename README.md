ServerStatus
===


Description
---
ServerStatus is a daemon that can be used on UNIX operation systems to create JSON, CSV and html files that contain system status informations. The general idea is to provide these files via http, https, ftp or any other way and let external (remote) devices parse these informations.

An example is the iPad app StatusBoard that can print all these files beautifully. I use this app to monitor my server status no matter where I am without the need to use ssh and remember some complicated shell commands.


Dependencies
---
Based on the different UNIX distributions there are different dependencies. On FreeBSD for example you need a third-party program to get the disc temperature like "smartmontools".

To build the daemon a C++ compiler is required that supports C++11 (GCC 4.7 and higher, clang 2.9 and higher, ...)


How to use...
---
To compile the code run "make install clean".

Then just add serverstatus to your autostart programs (e.g. on FreeBSD you can add "serverstatus_enable=YES" to the rc.conf file.

Based on your operation system you might need to adjust some of the commands in serverstatus.conf to make them work.