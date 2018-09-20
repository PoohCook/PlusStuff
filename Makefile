CC = g++
CPPFLAGS = -Wall -std=c++11 -iquote inc
LFLAGS = -Wall

DEPS = inc/Worker.h inc/Channel.h
OBJ = obj/Worker.o
TEST = obj/TestMessageBuffer.o obj/TestWorker.o obj/TestChannel.o



obj/%.o: src/%.cpp $(DEPS)
	mkdir -p obj
	$(CC) -c -o $@ $< $(CPPFLAGS)

obj/%.o: test/%.cpp $(DEPS)
	mkdir -p obj
	$(CC) -c -o $@ $< $(CPPFLAGS)

both: server client echo

server: $(OBJ) src/Server.cpp
	$(CC) -o $@  $^ -L /usr/lib -lboost_system -lpthread $(CPPFLAGS) $(LFLAGS)

client: $(OBJ) src/Client.cpp
	$(CC) -o $@  $^ -L /usr/lib -lboost_system -lpthread $(CPPFLAGS) $(LFLAGS)

echo: $(OBJ) src/Echo.cpp
	$(CC) -o $@  $^ -L /usr/lib -lboost_system -lpthread $(CPPFLAGS) $(LFLAGS)


main: $(OBJ) src/main.cpp
	$(CC) -o $@  $^ -L /usr/lib -lboost_system -lpthread $(CPPFLAGS) $(LFLAGS)


test: testRunner main
	./testRunner

testRunner: $(OBJ) $(TEST)
	$(CC) -o $@ test/TestMain.cpp $^ -L/usr/local/lib -lboost_unit_test_framework -lboost_system -lboost_serialization -DBOOST_TEST_DYN_LINK  -Wl,-rpath,'.' $(CPPFLAGS) $(LFLAGS)


clean:
	rm -f *.o *.so *.a main testRunner
	rm -rf obj
