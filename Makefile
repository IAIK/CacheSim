OBJECTS = sha-256.o ScatterCache.o SACache.o CacheHierarchy.o SplitCache.o RMapper.o GenericCache.o CacheMemory.o RSHA256.o RAES_NI.o RSASS_AES_NI.o RCAT.o RP_LRU.o RP_PLRU.o RP_BIP.o RP_QLRU.o RP_RANDOM.o

DEBUG_FLAG = -DNDEBUG

ifeq ($(DEBUG), 1)
  DEBUG_FLAG = 
endif

FLAGS = -Wall -O3 -g -Wno-unused-variable -std=c++17 $(DEBUG_FLAG)

all: objects

objects: $(OBJECTS)

ScatterCache.o: ScatterCache.cpp sha-256.o NoisyCache.h ScatterCache.h Cache.h
	g++ $(FLAGS) -c $(basename $@).cpp

sha-256.o: sha-256.c sha-256.h
	gcc -Wall -O3 -std=gnu11 -c $(basename $@).c

RAES_NI.o: RAES_NI.cpp
	g++ $(FLAGS) -msse2 -march=native -maes -c $<
	
RSASS_AES_NI.o: RSASS_AES_NI.cpp
	g++ $(FLAGS) -msse2 -march=native -maes -c $<

%.o: %.cpp %.h RPolicy.h NoisyCache.h Cache.h
	g++ $(FLAGS) -c $(basename $@).cpp

%: %.cpp $(OBJECTS)
	g++ $(OBJECTS) $(FLAGS) -o $@.x $@.cpp -lpthread


.PHONY: clean
clean:
	rm -f sim *.o *.x
