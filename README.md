ServerStatus
===

Description
---
ServerStatus is a daemon that can be used on UNIX operation systems to create JSON files that contain system status informations. The general idea is to provide these files via http, https, ftp or any other way and let remote devices parse these informations.

An example is the iPad app [StatusBoard](http://panic.com/statusboard/) that can print all these files in beautiful charts.


Dependencies
---

####Build dependencies
 1. Any C++ compiler that supports C++11 (e.g. GCC 4.7 and above, clang 2.9 and above, ...).
 2. libconfig [(Link)](http://www.hyperrealm.com/libconfig/libconfig_manual.html)
 3. For optional data encryption: OpenSSL [(Link)](https://www.openssl.org)

####Runtime dependencies
 1. libconfig
 2. OpenSSL (optional encryption)
 3. Any other third-party application to monitor an aspect of your server (e.g. `smartmontools` for HDD temperature)

####Installation####
If either GCC or clang is installed on your system all you need to do is to run:

     gmake install clean     # FreeBSD
      - or - 
     make install clean      # Debian / Ubuntu / OS X    

If you want to use a different C++ compiler you have to manually edit the `Makefile` and set `CC = your_g++_compiler`.

By default ServerStatus will be installed into a directory that allows to run it as a service:

     /usr/local/etc/rc.d/      # FreeBSD
      - or -
     /etc/init.d/              # Ubuntu / Debian
      - or - 
     /usr/local/bin/		   # OS X



Configuration
---
ServerStatus is looking for a configuration file named `serverstatus.cfg` and has to be located either at `/etc/`or at `/usr/local/etc/`.

A sample configuration file will be copied to `/usr/local/etc/` if ServerStatus is installed with the provided Makefile.

The sample configuration file is written for FreeBSD. However, at the end of this readme file there will be collection of commands for different UNIX systems that can be used on Debian or Ubuntu.

#### Data Distribution
ServerStatus distinguishes between two modes:
 1. ServerStatus can run as "client". This will execute commands on the local machine. Their return value can then be used locally ("standalone" mode) or can bei transmitted to another machine running in "server" mode. 
 2. "Server" mode is a great way to combine data from multiple servers (or jails). ServerStatus will then listen to incoming data from "client" systems and provide their information on the "server" system.
 
#### SSL
ServerStatus supports encrypted communication between client and server using OpenSSL. All that is required is a SSL certificate which can be created using the following command:

	openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout serverstatus.pem -out serverstatus.pem
	
Specify the path of the certificate and key-file inside the cfg file. An absolute path is required.

**Note:**
If you use the command above both certificate and key-file are the same (serverstatus.pem).

How to use...
---
You can start ServerStatus manually by running:

     service serverstatus start

But you might want to consider to add ServerStatus to your autostart programs so that your systems status can be monitored right after your system started.

**Note**:
With the latest changes, ServerStatus will require _root_ privileges to run.  


Commands
---
The following commands return a numeric value without any units that can be used with serverstatus without any problems.

####CPU temperature####
<table>
	<tr>
		<th>OS</th>
		<th>Command</th>
	</tr>
	<tr>
		<td>FreeBSD</td>
		<td>sysctl -n dev.cpu.&lt;CORE&gt;.temperature | sed 's/C//g'
		<br/><i>If not enabled run: kldload coretemp</i></td>
	</tr>
	<!--
	<tr>
		<td>Raspian</td>
		<td>vcgencmd measure_temp | cut -c 6- | sed "s/'/°/"</td>
	</tr>
	-->
</table>

####Load####
<table>
	<tr>
		<th>OS</th>
		<th colspan="2">Command</th>
	</tr>
	<tr>
		<td>All</td>
		<td>Load 1<br/>Load 5<br/>Load 15</td>
		<td>uptime | awk '{print $(NF)}' | sed 's/,/./'
		<br/>uptime | awk '{print $(NF-1)}' | sed 's/,/./'
		<br/>uptime | awk '{print $(NF-2)}' | sed 's/,/./'</td>
	</tr>
</table>

####HDD temperature####
<table>
	<tr>
		<th>OS</th>
		<th>Command</th>
	</tr>
	<tr>
		<td>FreeBSD</td>
		<td>smartctl -a /dev/&lt;DISC&gt; | awk '/Temperature_Celsius/{print $0}' | awk '{print $10}'</td>
	</tr>
</table>

####Disc Space####
<table>
	<tr>
		<th>OS</th>
		<th colspan="2">Command</th>
	</tr>
	<tr>
		<td>All</td>
		<td>Free<br/>Used</td>
		<td>df | grep ^&lt;MOUNT&gt; | awk '{print $3}'
		<br/>df | grep ^&lt;MOUNT&gt; | awk '{print $4}'</td>
	</tr>
</table>

####Memory####
<table>
	<tr>
		<th>OS</th>
		<th colspan="2">Command</th>
	</tr>
	<tr>
		<td>FreeBSD</td>
		<td>Active<br/>Inactive<br/>Wired<br/>Buffered<br/>Free</td>
		<td>top -d1 | grep ^Mem | awk '{print $2}' | sed 's/M/000000/' | sed 's/K/000/'
		<br/>top -d1 | grep ^Mem | awk '{print $4}' | sed 's/M/000000/' | sed 's/K/000/'
		<br/>top -d1 | grep ^Mem | awk '{print $6}' | sed 's/M/000000/' | sed 's/K/000/'
		<br/>top -d1 | grep ^Mem | awk '{print $8}' | sed 's/M/000000/' | sed 's/K/000/'
		<br/>top -d1 | grep ^Mem | awk '{print $10}' | sed 's/M/000000/' | sed 's/K/000/'</td>
	</tr>
	<tr>
		<td>Debian</td>
		<td>Used<br/>Buffered<br/>Cached<br/>Free</td>
		<td>free -m | egrep ^-/+ |  awk '{print $3}'
		<br/>free -m | egrep ^Mem |  awk '{print $6}'
		<br/>free -m | egrep ^Mem |  awk '{print $7}'
		<br/>free -m | egrep ^Mem |  awk '{print $4}'</td>
	</tr>
</table>

####Network####
<table>
	<tr>
		<th>OS</th>
		<th colspan="2">Command</th>
	</tr>
	<tr>
		<td>FreeBSD</td>
		<td>In<br/>Out</td>
		<td>netstat -I &lt;INTERFACE&gt; -b | awk '{ if (/Link/) { print $8 } }'
		<br>netstat -I &lt;INTERFACE&gt; -b | awk '{ if (/Link/) { print $11 } }'</td>
	</tr>
	<tr>
		<td>Debian</td>
		<td>In<br/>Out</td>
		<td>ifconfig wlan0 | grep "RX bytes" | awk '{ print $2 }' | sed 's/bytes://'
		<br/>ifconfig wlan0 | grep "RX bytes" | awk '{ print $6 }' | sed 's/bytes://'</td>
	</tr>
</table>