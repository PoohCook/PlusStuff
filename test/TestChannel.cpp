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


class Handler{

public:
    int process(string command, map<string,string> arg ){

       if (arg["bunny"] == "white") return 66;

       return 99;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest1 ){

    boost::asio::io_context io_context;

    ChannelProvider<string,map<string,string>,int,Handler> testChannel(1028);


    ChannelClient<string,map<string,string>,int> testClient(1028);

    std::map<string, string> args;
    args["pooh"] = "yellow";
    args["bunny"] = "white";
    args["kuma"] = "brown";

    int retVal = testClient.send("command1", args);

    BOOST_CHECK_EQUAL(  retVal, 66 );

    args["bunny"] = "red";

    retVal = testClient.send("command1", args);

    BOOST_CHECK_EQUAL(  retVal, 99 );


}


class Handler2{

public:
    string process(string command, map<string,string> arg ){
        stringstream ssRet;
        ssRet << command << ":"  << arg["pooh"] << ":" << arg["bunny"] << ":" << arg["kuma"];

        return ssRet.str();
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest2 ){

    boost::asio::io_context io_context;

    ChannelProvider<string,map<string,string>,string,Handler2> testChannel(1028);


    ChannelClient<string,map<string,string>,string> testClient(1028);

    std::map<string, string> args;
    args["pooh"] = "yellow";
    args["bunny"] = "white";
    args["kuma"] = "brown";

    string retVal = testClient.send("red", args);

    BOOST_CHECK_EQUAL(  retVal, "red:yellow:white:brown" );

    args["bunny"] = "red";

    retVal = testClient.send("blue", args);

    BOOST_CHECK_EQUAL(  retVal, "blue:yellow:red:brown" );


}



class Handler3{

public:
    int process(int command, int arg ){
       return command + arg;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest3 ){

    boost::asio::io_context io_context;

    ChannelProvider<int,int,int,Handler3> testChannel(1034);


    ChannelClient<int,int,int> testClient(1034);

    int retVal = testClient.send(41,1);

    BOOST_CHECK_EQUAL(  retVal, 42 );


    retVal = testClient.send(66, 3);

    BOOST_CHECK_EQUAL(  retVal, 69 );


}
