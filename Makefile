# Compiler
ifeq ("$(shell which clang++)", "/usr/bin/clang++")
	CC = /usr/bin/clang++
else
	CC = $(shell which g++)
endif

#Compiler flags
FLAGS = -std=c++11 -Wall

# Input files
INPUT = system_stats.o unix_functions.o ini.o serverstatus.o

# Output path
ifeq ("$(shell uname -s)", "FreeBSD")
    PATH = /usr/local/etc/rc.d/
else 
    PATH = /etc/init.d/
endif

# Output filename
OUTPUT = serverstatus

#------------------------------------------------------------------------------

install: $(OUTPUT)
	/bin/mv $(OUTPUT) $(PATH)$(OUTPUT)
	/bin/mkdir -p /usr/local/etc/serverstatus/
	/bin/cp serverstatus.conf /usr/local/etc/serverstatus.conf
	/bin/cp serverstatus.man /usr/share/man/man8/serverstatus.8
	
$(OUTPUT): $(INPUT)
	$(CC) $(INPUT) -o $(OUTPUT) $(FLAGS)

unix_functions.o: unix_functions.cpp
	$(CC) -c unix_functions.cpp $(FLAGS)
			
system_stats.o: system_stats.cpp
	$(CC) -c system_stats.cpp $(FLAGS)
	
ini.o: ini.cpp
	$(CC) -c ini.cpp $(FLAGS)
	
serverstatus.o: serverstatus.cpp
	$(CC) -c serverstatus.cpp $(FLAGS)

clean:
	/bin/rm -f *.o
	
deinstall:
	/bin/rm -f $(PATH)$(OUTPUT)