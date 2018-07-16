#include<iostream>
#include<assert.h>
#include<unistd.h>

int main()
{
	long sum = 1;
	for(long i = 6000000000; i > 0; i--)
	{
		sum = sum + 1;
	}
	return 0;
}
