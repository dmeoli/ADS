/*--------------------------------------------------------------------------*/
/*---------------------------- File module.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The program of main.cpp on the C++20 module ads, which the package has
 * when ADS was built with ADS_BUILD_MODULE: a List reversed by
 * std::ranges::reverse and summed.
 */
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <iostream>

import ads;

int main()
{
 ADS::List< int > L{ 3 , 1 , 2 };
 std::ranges::reverse( L );
 int s = 0;
 for( int x : L ) s = 10 * s + x;
 std::cout << s << '\n';
 return s == 213 ? 0 : 1;
}

/*--------------------------------------------------------------------------*/
/*-------------------------- End File module.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
