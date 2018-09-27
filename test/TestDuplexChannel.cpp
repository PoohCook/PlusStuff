
#include <string>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include "ChannelProvider.h"
#include "ChannelClient.h"
#include "IsPrime.h"
#include "Worker.h"

using namespace std;


class PrimesHandler{
public:
    bool process( int candidate ){

       return IsPrime( candidate );

    }
};

BOOST_AUTO_TEST_CASE( DuplexChannelTest1 ){

    ChannelProvider<int,bool,PrimesHandler> testChannel(1028);

    int client_id = 105280;
    ChannelClient<int,bool,PrimesHandler> testClient(client_id, 1028);


    bool retVal = testClient.send( 7);

    BOOST_CHECK_EQUAL(  retVal, true );

    retVal = testClient.send( 8);

    BOOST_CHECK_EQUAL(  retVal, false );


    retVal = testChannel.send( client_id, 8);

    BOOST_CHECK_EQUAL(  retVal, false );

    retVal = testChannel.send( client_id, 11);

    BOOST_CHECK_EQUAL(  retVal, true );


}


class PrimeChannelProcessor{

    int myNumber_;
    ChannelProvider<int,bool,PrimesHandler>* testChannel_ = NULL;
    ChannelClient<int,bool,PrimesHandler>* testClient_ = NULL;
    int client_id_ = 0;

public:
    static vector<int> primes;

    PrimeChannelProcessor(
        int num,
        ChannelProvider<int,bool,PrimesHandler>* chanProvider,
        ChannelClient<int,bool,PrimesHandler>* chanClient,
        int client_id ):
        myNumber_(num), testChannel_(chanProvider),
        testClient_(chanClient), client_id_(client_id){

    }

    //  required method to be used by a Worker class
    void  process(){
        bool isPrime = false;
        if( testChannel_ != NULL){
            isPrime = testChannel_->send(client_id_, myNumber_);
        }
        else if( testClient_ != NULL ){
            isPrime = testClient_->send(myNumber_);
        }

        if(isPrime){
            primes.push_back(myNumber_);
            //cout << myNumber_ << ", ";
        }

    }
};

vector<int> PrimeChannelProcessor::primes = vector<int>();

BOOST_AUTO_TEST_CASE( DuplexChannelTest2 ){


    ChannelProvider<int,bool,PrimesHandler> testChannel(1033);

    int client_id = 105289;
    ChannelClient<int,bool,PrimesHandler> testClient(client_id, 1033);


    Worker<PrimeChannelProcessor> myWorker;

    for ( int indx = 1 ; indx < 10000 ; indx++ ){
        myWorker.push(PrimeChannelProcessor(indx, &testChannel, NULL, client_id));
    }


    //cout << "shutting down\n";
    myWorker.shutdown(true);


    sort( PrimeChannelProcessor::primes.begin(), PrimeChannelProcessor::primes.end() );

    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes.size(), 1230ul );

    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[0], 1 );

    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[2], 3 );
    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[3], 5 );
    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[4], 7 );

    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[23], 83 );
    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[24], 89 );
    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[25], 97 );

    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[1229], 9973 );


}


BOOST_AUTO_TEST_CASE( DuplexChannelTest3 ){


//    ChannelProvider<int,bool,PrimesHandler> testChannel(1033);
//
//    int client_id = 105289;
//    ChannelClient<int,bool,PrimesHandler> testClient(client_id, 1033);
//
//
//    Worker<PrimeChannelProcessor> myWorker;
//    Worker<PrimeChannelProcessor> myWorker2;
//
//    for ( int indx = 1 ; indx < 5000 ; indx++ ){
//        myWorker.push(PrimeChannelProcessor(indx, &testChannel, NULL, client_id));
//    }
//
//    for ( int indx = 5000 ; indx < 10000 ; indx++ ){
//        myWorker2.push(PrimeChannelProcessor(indx, NULL, &testClient, 0));
//    }
//
//    //cout << "shutting down\n";
//    myWorker.shutdown(true);
//    myWorker2.shutdown(true);
//
//
//    sort( PrimeChannelProcessor::primes.begin(), PrimeChannelProcessor::primes.end() );
//
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes.size(), 1230ul );
//
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[0], 1 );
//
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[2], 3 );
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[3], 5 );
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[4], 7 );
//
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[23], 83 );
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[24], 89 );
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[25], 97 );
//
//    BOOST_CHECK_EQUAL(  PrimeChannelProcessor::primes[1229], 9973 );
//

}



