/*
 * File:   TestOptions.cpp
 * Author: Pooh
 *
 * Created on October 4, 2018, 8:28 AM
 */


#include <string>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include "Options.h"


using namespace std;



class TestOptions : public process_params::Options{

public:
    bool A;
    int B;
    string C;

    TestOptions(){

        add_option<bool>("a", &A, "set A");
        add_option<int>("b", &B, "set B");
        add_option<string>("c", &C, "set C");

    }

};



BOOST_AUTO_TEST_CASE( OptionsTest1 ){

    cout << "starting options test \n";

    TestOptions to;

    const char* argv[] = {"progid", "--a", "true", "--b", "88", "--c", "100 acre woods"};

    to.parse_options(7, argv);


    BOOST_CHECK_EQUAL(  to.A, true );
    BOOST_CHECK_EQUAL(  to.B, 88 );
    BOOST_CHECK_EQUAL(  to.C, "100 acre woods" );

    std::vector<string> args = to.get_option_args();

    BOOST_CHECK_EQUAL(  args.size(), 6 );


    BOOST_CHECK_EQUAL(  args[0], "--a" );
    BOOST_CHECK_EQUAL(  args[1], "1" );
    BOOST_CHECK_EQUAL(  args[2], "--b" );
    BOOST_CHECK_EQUAL(  args[3], "88" );
    BOOST_CHECK_EQUAL(  args[4], "--c" );
    BOOST_CHECK_EQUAL(  args[5], "100 acre woods" );





};


BOOST_AUTO_TEST_CASE( OptionsTest2 ){


    TestOptions to;

    to.A = true;
    to.B = 88;
    to.C = "100 acre woods";

    std::vector<string> args = to.get_option_args();

    BOOST_CHECK_EQUAL(  args.size(), 6 );


    BOOST_CHECK_EQUAL(  args[0], "--a" );
    BOOST_CHECK_EQUAL(  args[1], "1" );
    BOOST_CHECK_EQUAL(  args[2], "--b" );
    BOOST_CHECK_EQUAL(  args[3], "88" );
    BOOST_CHECK_EQUAL(  args[4], "--c" );
    BOOST_CHECK_EQUAL(  args[5], "100 acre woods" );





};





