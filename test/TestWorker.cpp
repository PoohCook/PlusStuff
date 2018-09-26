/*
 * File:   TestWorker.cpp
 * Author: Pooh
 *
 * Created on September 16, 2018, 1:20 PM
 */


#include <boost/test/unit_test.hpp>

#include "Worker.h"
#include <string>
#include <iostream>

using namespace std;



class PrimeProcessor{

    int myNumber;
public:
    static vector<int> primes;

    PrimeProcessor(int num){
        myNumber = num;
    }

    //  required method to be used by a Worker class
    void  process(){
        bool isPrime = true;

        for(int divCandidate = 2; divCandidate<= myNumber/2; ++divCandidate)
        {
          if(myNumber % divCandidate == 0)
          {
              isPrime = false;
              break;
          }
        }

        if(isPrime){
            primes.push_back(myNumber);
            //cout << myNumber << " [" << primes.size() << "] is prime\n";
        }

    }
};

vector<int> PrimeProcessor::primes = vector<int>();

BOOST_AUTO_TEST_CASE( WorkerTest1 ){

    Worker<PrimeProcessor> myWorker;

    for ( int indx = 1 ; indx < 10000 ; indx++ ){
        myWorker.push(PrimeProcessor(indx));
    }

    //cout << "shutting down\n";
    myWorker.shutdown(true);


    BOOST_CHECK_EQUAL(  PrimeProcessor::primes.size(), 1230ul );

    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[0], 1 );

    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[2], 3 );
    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[3], 5 );
    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[4], 7 );

    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[23], 83 );
    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[24], 89 );
    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[25], 97 );

    BOOST_CHECK_EQUAL(  PrimeProcessor::primes[1229], 9973 );


}

