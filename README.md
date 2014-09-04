ServerStatus
===


Description
---
ServerStatus is a daemon that can be used on UNIX operation systems to create JSON, CSV and html files that contain system status informations. The general idea is to provide these files via http, https, ftp or any other way and let external (remote) devices parse these informations.

An example is the iPad app StatusBoard that can print all these files beautifully. I use this app to monitor my server status no matter where I am without the need to use ssh and remember some complicated shell commands.


Dependencies
---
Based on the different UNIX distributions there are different dependencies. On FreeBSD for example you need a third-party program to get the disc temperature like "smartmontools".

To build the daemon a C++ compiler is required that supports C++11 (GCC 4.7 and higher, clang 2.9 and higher, ...).

You can either compile the code all yourself or use the command `make`. Using the Makefile has the advantage that it moves the program to the `rc.d`-folder too so that it can be run as a service 1

###FreeBSD###
Simply run `make install clean`. The clang compiler is already included in FreeBSD 10. Otherwise install `clang` or change the line

`CC = /usr/bin/clang++`

to the c++ compiler that is installed on your system. All the other configuration are written for FreeBSD.


###Debian/Ubuntu###
Debian and Ubuntu do not seem to have a c++ compiler installed. So install your favorite compiler of choice and write the compiler (with its absolute path) in the Makefile.

Debian and Ubuntu need an other export path, too. So you need to change one more line of the Makefile to:

`PATH = /usr/local/etc/rc2.d/`


Configuration
---
The program is looking for a config file that is either `/etc/serverstatus.conf` or `/usr/local/etc/serverstatus.conf`.

The configuration file uses a simple syntax that is known by ini-files.

Notes:
 - If you change the filepath make sure that the new directory exists.
 - If you change the count of HDD, Mount or CPU make sure that you have exactly that many commands (starting at 1).
 
The sample commands are for FreeBSD. However you will need to install S.M.A.R.T on your FreeBSD system to get the HDD temperature.

Run: `cd /usr/ports/sysutils/smartmontools/ && make install clean`

On other systems there might be other commands necessary!

How to use...
---

You might want to consider to add ServerStatus to your autostart programs so that der system status can be monitored at any time.

It is recommended to run ServerStatus as root.

###FreeBSD###
You can start the program by running `service serverstatus start` or `/usr/local/etc/rc.d/serverstatus start`. 

###Debian/Ubuntu###
Run `/etc/rc2.d/serverstartus start`.