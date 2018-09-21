/*
 * File:   TcpServer.cpp
 * Author: Pooh
 *
 * Created on September 21, 2018, 3:39 PM
 */

#include "TcpServer.h"


class DummyHandler1{

public:
    int process(int command ){
       return command ;
    }
};

static void testCreate(){

    boost::asio::io_service io;
    TcpServer<int, int, DummyHandler1> server(io, 1033);

}
