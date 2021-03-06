/*
 * File:   TestChannel.cpp
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:06 PM
 */


#include <string>
#include <iostream>
#include <memory>
#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include "ChannelProvider.h"
#include "ChannelClient.h"

using namespace std;


class Handler{
public:
    int process( int client_id, map<string,string> command ){
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
    string process(int client_id, map<string,string> arg ){
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
    int process(int client_id, int command ){
       return command + 1;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest3 ){

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
    Command process(int client_id, Command command ){

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
    Command process(int client_id, Command command ){

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

// test if close and destructor can both be called on the client and provider
BOOST_AUTO_TEST_CASE( ChannelTest8 ){

    ChannelProvider<int,int,Handler3> testChannel(1034);

    int client_id = 105366;
    ChannelClient<int,int, Handler3> testClient(client_id, 1034);


    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds().size(), 1ul );

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds()[0], client_id );

    int retVal = testClient.send(41);

    BOOST_CHECK_EQUAL(  retVal, 42 );


    retVal = testChannel.send(client_id, 68);

    BOOST_CHECK_EQUAL(  retVal, 69 );

    testClient.close();
    testChannel.close();


}


class Handler6{
public:
    int process(int client_id, int command ){
       return client_id + command ;
    }
};

BOOST_AUTO_TEST_CASE( ChannelTest9 ){

    ChannelProvider<int,int,Handler6> testChannel(1034);

    int client_id = 8;
    ChannelClient<int,int, Handler6> testClient(client_id, 1034);

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds().size(), 1ul );

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds()[0], client_id );

    int retVal = testClient.send(2);

    BOOST_CHECK_EQUAL(  retVal, 10 );


    retVal = testChannel.send(client_id, 4);

    BOOST_CHECK_EQUAL(  retVal, 12 );

    testClient.close();
    testChannel.close();


}


class Handler7{

public:
    static mutex stopper;

    int process(int client_id, int command ){
       lock_guard<mutex> lock(stopper);

       sleep( 5);
       return client_id + command ;
    }
};

mutex Handler7::stopper;

static void send_test_channel( ChannelClient<int,int, Handler7>& testClient ) {
    testClient.send(2);
}


BOOST_AUTO_TEST_CASE( ChannelTest10 ){

    ChannelProvider<int,int,Handler7> testChannel(1034);

    int client_id = 8;
    ChannelClient<int,int, Handler7> testClient(client_id, 1034, "127.0.0.1", 3);

    cout << "long processing simulation\n";

    BOOST_CHECK_THROW (send_test_channel(testClient), std::runtime_error);

    lock_guard<mutex> lock(Handler7::stopper);

}


BOOST_AUTO_TEST_CASE( ChannelTest11 ){

    ChannelProvider<int,int,Handler6> testChannel(1034);

    int client1_id = 8;
    int client2_id = 18;
    std::vector<std::unique_ptr<ChannelClient<int,int, Handler6>>> clients;
    clients.push_back(std::unique_ptr<ChannelClient<int,int, Handler6>>(new ChannelClient<int,int, Handler6>(client1_id, 1034)));
    clients.push_back(std::unique_ptr<ChannelClient<int,int, Handler6>>(new ChannelClient<int,int, Handler6>(client2_id, 1034)));


    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds().size(), 2ul );

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds()[0], client1_id );
    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds()[1], client2_id );

    int retVal = clients[0]->send(2);

    BOOST_CHECK_EQUAL(  retVal, 10 );

    retVal = testChannel.send(client1_id, 4);

    BOOST_CHECK_EQUAL(  retVal, 12 );

    retVal = clients[1]->send(2);

    BOOST_CHECK_EQUAL(  retVal, 20 );

    retVal = testChannel.send(client2_id, 4);

    BOOST_CHECK_EQUAL(  retVal, 22 );





}



BOOST_AUTO_TEST_CASE( ChannelTest12 ){

    ChannelProvider<int,int> testChannel(1034);

    int client_id = 8;
    ChannelClient<int,int, Handler6> testClient(client_id, 1034);

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds().size(), 1ul );

    BOOST_CHECK_EQUAL(  testChannel.attachedClientIds()[0], client_id );


    int retVal = testChannel.send(client_id, 4);

    BOOST_CHECK_EQUAL(  retVal, 12 );



}

