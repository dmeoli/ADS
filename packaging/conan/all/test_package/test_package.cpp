#include "ADS/sequence/List.h"

#include <algorithm>
#include <iostream>

int main()
{
 ADS::List< int > L{ 3 , 1 , 2 };
 std::ranges::reverse( L );
 int s = 0;
 for( int x : L ) s = 10 * s + x;
 std::cout << s << '\n';
 return s == 213 ? 0 : 1;
}
