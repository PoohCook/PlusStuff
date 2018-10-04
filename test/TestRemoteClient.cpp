/*
 * File:   TestRemoteClient.cpp.cpp
 * Author: Pooh
 *
 * Created on October 3, 2018, 5:24 PM
 */


#include <string>
#include <iostream>
#include <memory>
#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/process.hpp>
#include "ChannelProvider.h"
#include "PoohCommand.h"

namespace bp = boost::process;


BOOST_AUTO_TEST_CASE( RemoteClientTest1 ){

    cout << "starting remote client \n";

    PoohOptions options;
    options.port = 1044;
    options.client_id = 42;
    options.secret = "piglet";

    ChannelProvider<PoohCommand,string> testChannel(options.port);


    bp::child c("./eeyore", bp::args(options.get_option_args()));

    while( testChannel.attachedClientIds().size() < 1)  {
        sleep(1);
    }

    PoohCommand command;
    command.type = POOH_COMMAND_HELLO;

    string reply = testChannel.send(options.client_id, command);


    BOOST_CHECK_EQUAL(  reply, "Hello Pooh" );

    command.type = POOH_COMMAND_QUESTION;
    command.arg.push_back("name");

    reply = testChannel.send(options.client_id, command);

    BOOST_CHECK_EQUAL(  reply, "Eeyore" );


    command.type = POOH_COMMAND_QUESTION;
    command.arg.clear();
    command.arg.push_back("secret");

    reply = testChannel.send(options.client_id, command);

    BOOST_CHECK_EQUAL(  reply, options.secret );


    command.type = POOH_COMMAND_GOODBYE;

    reply = testChannel.send(options.client_id, command);


    BOOST_CHECK_EQUAL(  reply, "Bye now" );


    c.wait(); //wait for the process to exit


}