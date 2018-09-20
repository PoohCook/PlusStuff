/*
 * File:   TestChannel.cpp
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:06 PM
 */


#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>

#include "Channel.h"
#include <string>
#include <iostream>

using namespace std;

class TestCommand{

    string comm;
    map<string, string> args;

public:
    TestCommand( string command, map<string, string> arguments){
        comm = command;
        args = arguments;
    }




};

BOOST_AUTO_TEST_CASE( ChannelTest1 ){

    boost::asio::io_context io_context;

    ChannelProvider<TestCommand> testChannel(1028);


    ChannelClient<TestCommand> testClient(1028);

    cout << testClient.sendReceive("Pooh Says: ") << "\n";

//    boost::asio::steady_timer t(io_context, boost::asio::chrono::seconds(5));
//    t.wait();


}