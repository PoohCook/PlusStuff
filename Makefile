CC = g++
CPPFLAGS = -g -Wall -std=c++11 -pthread -iquote inc -I boostlib/inc
LFLAGS = -Wall -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK -DBOOST_LOG_DYN_LINK
BOOST = -Lboostlib/lib -lboost_unit_test_framework -lboost_system -lboost_serialization -lboost_wserialization -Wl,-rpath,'boostlib/lib'

DEPS = inc/Worker.h inc/ChannelProvider.h inc/ChannelClient.h inc/ChannelClientSession.h inc/ChannelProviderSession.h inc/ChannelSendProcessor.h inc/IsPrime.h
OBJ = obj/IsPrime.o
TEST = obj/TestWorker.o obj/TestChannel.o obj/TestDuplexChannel.o

obj/%.o: src/%.cpp $(DEPS)
	mkdir -p obj
	$(CC) -c -o $@ $< $(CPPFLAGS)

obj/%.o: test/%.cpp $(DEPS)
	mkdir -p obj
	$(CC) -c -o $@ $< $(CPPFLAGS)

main: $(OBJ) src/main.cpp
	$(CC) -o $@  $^ $(BOOST) $(CPPFLAGS) $(LFLAGS)


test: main testRunner
	./testRunner

testRunner: $(OBJ) $(TEST) test/TestMain.cpp
	$(CC) -o $@  $^ -DBOOST_TEST_DYN_LINK $(BOOST)  $(CPPFLAGS) $(LFLAGS)

boostlib:
	rm -f /usr/lib/libboost*
	rm -f /usr/local/lib/libboost*
	rm -fr /usr/include/boost/
	rm -fr /usr/local/include/boost/
	wget https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz
	tar -zxvf boost_1_67_0.tar.gz
	(cd boost_1_67_0/ && ./bootstrap.sh)
	(cd boost_1_67_0/ && ./b2 install)
	rm -rf boost_1_67_0
	rm boost_1_67_0.tar.gz
	find /usr/ -name text_iarchive.hpp
	mkdir -p boostlib
	mkdir -p boostlib/lib
	mkdir -p boostlib/inc
	cp /usr/local/lib/libboost* boostlib/lib/
	cp -rf /usr/local/include/boost boostlib/inc/
	ls boostlib/inc/boost
	ls boostlib/lib

clean:
	rm -f *.o *.so *.a main testRunner
	rm -rf obj
	rm -f testRunner
	rm -f main
