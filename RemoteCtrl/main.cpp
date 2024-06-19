#include "pch.h"
class A 
{
public:
	int func() { return a; }
public:
	int a, b, c;
	const int d =  10; 
};

int main()
{
	A obj;
	std::cout << obj.func();
}