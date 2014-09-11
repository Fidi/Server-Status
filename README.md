ServerStatus
===

Description
---
ServerStatus is a daemon that can be used on UNIX operation systems to create JSON files (and in the future maybe CSV and html files) that contain system status informations. The general idea is to provide these files via http, https, ftp or any other way and let external (remote) devices parse these informations.

An example is the iPad app [StatusBoard](http://panic.com/statusboard/) that can print all these files beautifully.


Dependencies
---
To build this daemon a C++ compiler is required that supports C++11 (e.g. GCC 4.7 and above, clang 2.9 and above, ...).

The daemon might require other programs at runtime. For example to get the hdd temperature on FreeBSD you need a third-party program like `smartmontools`. 
On other operation systems there might be other programs or commands necessary that need to be installed.

####Installation####
If either GCC or clang is installed on your system all you need to do is to run:

     gmake install clean     # FreeBSD
      - or - 
     make install clean      # Debian / Ubuntu     

If you want to use a different C++ compiler you have to manually edit the `Makefile` and set `CC = your_g++_compiler`.

By default ServerStatus will be installed into a directory that allows to run it as a service:

     /usr/local/etc/rc.d/      # FreeBSD
      - or -
     /etc/init.d/              # Ubuntu / Debian


Configuration
---
ServerStatus is looking for a configuration file named `serverstatus.conf` and has to be located either at `/etc/`or at `/usr/local/etc/`.

A sample configuration file will be copied to `/usr/local/etc/` if ServerStatus is installed with the provided Makefile.

You can either configure ServerStatus manually by editing the `serverstatus.conf` file with any editor or you can call 

     service serverstatus --config
      - or -
     service serverrstatus -c
     
and get let through the configuration process by answering a couple of questions. This will have the advantage that you don't have to learn its syntax.

*Notes (if you edit the file manually):*
 - *If you change the filepath make sure that the new directory exists.*
 - *If you change the count of HDD, Mount or CPU make sure that you have exactly that many commands (starting at 1).*

 
The sample configuration file is written for FreeBSD. However at the end of this file there will be collection of commands for different Linux systems that can be used on Debian or Ubuntu.


How to use...
---
You can start ServerStatus manually by running:

     service serverstatus start

But you might want to consider to add ServerStatus to your autostart programs so that der system status can be monitored right after your system started.

**Note**:
It is strongly recommended to run ServerStatus as **root** (or sudo) to make sure that all commands will work and that you have permission to write the json-files.


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