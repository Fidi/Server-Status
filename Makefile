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

#PATH variable
EXEPATH = /bin/

# Output path
ifeq ("$(shell uname -s)", "FreeBSD")
    PATH = /usr/local/etc/rc.d/
else ifeq ("$(shell uname -s)", "Darwin")
	PATH = /usr/local/bin/
else
	PATH = /etc/init.d/
endif

# Output filename
OUTPUT = serverstatus

#------------------------------------------------------------------------------

install: $(OUTPUT)
	$(EXEPATH)mv $(OUTPUT) $(PATH)$(OUTPUT)
	$(EXEPATH)mkdir -p /usr/local/etc/serverstatus/
	$(EXEPATH)cp serverstatus.conf /usr/local/etc/serverstatus.conf
	$(EXEPATH)cp serverstatus.man /usr/share/man/man8/serverstatus.8
	@echo "ServerStatus successfully installed. \n"
	@echo "Install path:" $(PATH)$(OUTPUT)
	@echo "Config path: /usr/local/etc/serverstatus.conf \n"
	
$(OUTPUT): $(INPUT)
	@echo "All dependencies successfully built."
	$(CC) $(INPUT) -o $(PATH) $(FLAGS)
	
system_stats.o: system_stats.cpp
	$(CC) -c system_stats.cpp $(FLAGS)
		
ini.o: ini.cpp
	$(CC) -c ini.cpp $(FLAGS)

unix_functions.o: unix_functions.cpp
	$(CC) -c unix_functions.cpp $(FLAGS)
	
serverstatus.o: serverstatus.cpp
	$(CC) -c serverstatus.cpp $(FLAGS)

clean:
	/bin/rm -f *.o
	
deinstall:
	$(EXEPATH)rm -f $(PATH)$(OUTPUT)
	$(EXEPATH)rm -f /usr/local/etc/serverstatus.conf
	$(EXEPATH)rm -f /usr/share/man/man8/serverstatus.8