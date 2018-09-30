/*
 * File:   TestChannel.cpp
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:06 PM
 */


#include <string>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include "ChannelProvider.h"
#include "ChannelClient.h"

using namespace std;


class Handler{
public:
    int process( map<string,string> command ){
       if (command["bunny"] == "white") return 66;
       return 99;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest1 ){


    ChannelProvider<map<string,string>,int,Handler> testChannel(1028);

    int client_id = 105280;
    ChannelClient<map<string,string>,int> testClient(client_id, 1028);

    std::map<string, string> args;
    args["pooh"] = "yellow";
    args["bunny"] = "white";
    args["kuma"] = "brown";

    int retVal = testClient.send( args);

    BOOST_CHECK_EQUAL(  retVal, 66 );

    args["bunny"] = "red";

    retVal = testClient.send( args);

    BOOST_CHECK_EQUAL(  retVal, 99 );

}


class Handler2{
public:
    string process(map<string,string> arg ){
        stringstream ssRet;
        ssRet << arg["pooh"] << ":" << arg["bunny"] << ":" << arg["kuma"];

        return ssRet.str();
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest2 ){

    ChannelProvider<map<string,string>,string,Handler2> testChannel(1028);

    int client_id = 105288;
    ChannelClient<map<string,string>,string> testClient(client_id, 1028);

    std::map<string, string> args;
    args["pooh"] = "yellow";
    args["bunny"] = "white";
    args["kuma"] = "brown";

    string retVal = testClient.send( args);

    BOOST_CHECK_EQUAL(  retVal, "yellow:white:brown" );

    args["bunny"] = "red";

    retVal = testClient.send(args);

    BOOST_CHECK_EQUAL(  retVal, "yellow:red:brown" );


}


class Handler3{
public:
    int process(int command ){
       return command + 1;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest3 ){

    cout << "begin here\n";

    ChannelProvider<int,int,Handler3> testChannel(1034);

    int client_id = 105366;
    ChannelClient<int,int> testClient(client_id, 1034);

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds().size(), 1ul );

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds()[0], client_id );

    int retVal = testClient.send(41);

    BOOST_CHECK_EQUAL(  retVal, 42 );


    retVal = testClient.send(68);

    BOOST_CHECK_EQUAL(  retVal, 69 );


}


class Command{
private:
    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & command;
        ar & args;
    }

public:
    string command;
    map<string,string> args;

};

class Handler4{
public:
    Command process(Command command ){

        Command response;
        response.command = command.command + "_response";


        for( const auto& sm_pair : command.args ){
            response.args[sm_pair.first] = sm_pair.second + "_resp";
        }

        return response;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest4 ){

    ChannelProvider<Command,Command,Handler4> testChannel(1028);

    int client_id = 100666;
    ChannelClient<Command,Command> testClient(client_id, 1028);

    Command command;
    command.command = "purple";
    command.args["pooh"] = "yellow";
    command.args["bunny"] = "white";
    command.args["kuma"] = "brown";

    Command response = testClient.send( command);

    BOOST_CHECK_EQUAL(  response.command, "purple_response" );

    BOOST_CHECK_EQUAL(  response.args["pooh"], "yellow_resp");

}


static void create_test_channel( int client_id, short port ) {
    ChannelClient<int,int> testClient(client_id, port);
}

BOOST_AUTO_TEST_CASE( ChannelTest5 ){

    ChannelProvider<int,int,Handler3> testChannel(1034);

    int client_id = testChannel.generateWhitelistId();

    int wrong_id = client_id + 1;

    BOOST_CHECK_THROW (create_test_channel(wrong_id, 1034), boost::system::system_error);

    ChannelClient<int,int> testClient(client_id, 1034);

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds()[0], client_id );

    int retVal = testClient.send(41);

    BOOST_CHECK_EQUAL(  retVal, 42 );

}


class Handler5{
public:
    Command process(Command command ){

        Command response;
        response.command = command.command + "_reply";


        for( const auto& sm_pair : command.args ){
            response.args[sm_pair.first] = sm_pair.second + "_reply";
        }

        return response;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest6 ){

    ChannelProvider<Command,Command,Handler4> testChannel(1044);

    int client_id = testChannel.generateWhitelistId();

    ChannelClient<Command,Command, Handler5> testClient(client_id, 1044);

    Command command;
    command.command = "purple";
    command.args["pooh"] = "yellow";
    command.args["bunny"] = "white";
    command.args["kuma"] = "brown";

    Command response = testClient.send( command);

    BOOST_CHECK_EQUAL(  response.command, "purple_response" );

    BOOST_CHECK_EQUAL(  response.args["pooh"], "yellow_resp");

    command.command = "orange";
    command.args["pooh"] = "yellow";
    command.args["bunny"] = "white";
    command.args["kuma"] = "brown";

    response = testChannel.send( client_id, command);

    BOOST_CHECK_EQUAL(  response.command, "orange_reply" );

    BOOST_CHECK_EQUAL(  response.args["bunny"], "white_reply");

    command.command = "purple";
    command.args["pooh"] = "yellow";
    command.args["bunny"] = "white";
    command.args["kuma"] = "brown";

    response = testClient.send( command);

    BOOST_CHECK_EQUAL(  response.command, "purple_response" );

    BOOST_CHECK_EQUAL(  response.args["pooh"], "yellow_resp");

    command.command = "orange";
    command.args["pooh"] = "yellow";
    command.args["bunny"] = "white";
    command.args["kuma"] = "brown";

    response = testChannel.send( client_id, command);

    BOOST_CHECK_EQUAL(  response.command, "orange_reply" );

    BOOST_CHECK_EQUAL(  response.args["bunny"], "white_reply");

}

// test that an unattached provider can be destroyed
BOOST_AUTO_TEST_CASE( ChannelTest7 ){

    ChannelProvider<int,int,Handler3> testChannel(1034);

}
