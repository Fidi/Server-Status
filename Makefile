CC     = /usr/bin/clang++
FLAGS  = -std=c++11 -Wall
INPUT  = system_stats.o unix_functions.o ini.o serverstatus.o
PATH   = /usr/local/etc/rc.d/
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