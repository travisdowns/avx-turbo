/*
 * unit-test.cpp
 */


#include "catch.hpp"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "factorial" ) {
    REQUIRE( Factorial(3) == 6 );
}





