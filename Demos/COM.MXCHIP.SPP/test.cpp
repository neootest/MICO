#include <cstdint>
#include <iostream>
//using namespace std;

#include "cfunctions.h"
#include "cppfunctions.h"

void cpp_main(void)
{
  c_function(100,100);
}

void cpp_function(int a, int b)
{
    // This C++ function needs C linkage to be visible to C applications
  cout << "[cpp_function]: Hello from C++: " << a << "," << b << endl;
}

