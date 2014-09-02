CC     = g++
FLAGS  = -std=c++11 -Wall
INPUT  = cpu.o hdd.o unix_functions.o serverstatus.o
OUTPUT = serverstatus

#------------------------------------------------------------------------------

install: $(OUTPUT)

$(OUTPUT): $(INPUT)
	$(CC) $(INPUT) -o $(OUTPUT) $(FLAGS)
	
cpu.o: cpu.cpp
	$(CC) -c cpu.cpp $(FLAGS)
	
hdd.o: hdd.cpp
	$(CC) -c hdd.cpp $(FLAGS)
	
unix_functions.o: unix_functions.cpp
	$(CC) -c unix_functions.cpp $(FLAGS)
	
ini.o: ini.cpp
	$(CC) -c ini.cpp $(FLAGS)
	
serverstatus.o: serverstatus.cpp
	$(CC) -c serverstatus.cpp $(FLAGS)

clean:
	rm -f *.o
	
deinstall:
	rm -f $(OUTPUT)