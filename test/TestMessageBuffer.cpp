



#include <boost/test/unit_test.hpp>

#include "MessageBuffer.h"
#include <string>
#include <iostream>

using namespace std;

BOOST_AUTO_TEST_CASE( MessageBufferTest )
{

   MessageBuffer<string> uut_buffer;

    uut_buffer.Push( string("red"));
    uut_buffer.Push( string("green"));
    uut_buffer.Push( string("blue"));

    BOOST_CHECK( uut_buffer.Pop() == "red"  );        // #1 continues on error

    BOOST_REQUIRE( uut_buffer.Pop() == "green"  );      // #2 throws on error

    BOOST_CHECK_EQUAL(  uut_buffer.Pop(), "blue" );


}


class Boggy{
    static int instance;

    int myInstance;

public:
    Boggy(){
      myInstance = instance++;

      //cout << "created " << myInstance << "; ";
    }

    ~Boggy(){
        //cout << "destroyed " << myInstance << "; ";
    }

    int GetInstance(){
        return myInstance;
    }

};

int Boggy::instance=0;

BOOST_AUTO_TEST_CASE( MessageBufferTest2 )
{

   MessageBuffer<Boggy> uut_buffer;

    uut_buffer.Push( Boggy());
    uut_buffer.Push( Boggy());
    uut_buffer.Push( Boggy());

    BOOST_CHECK_EQUAL( uut_buffer.Pop().GetInstance(), 100  );        // #1 continues on error

    BOOST_CHECK_EQUAL( uut_buffer.Pop().GetInstance(), 101  );      // #2 throws on error

    BOOST_CHECK_EQUAL( uut_buffer.Pop().GetInstance(), 102 );


}
