#include <stdio.h>

typedef int ptr_t;

int main()
{
	ptr_t* a = 2000;
	ptr_t* b = 1000;
	
	ptr_t* c = a-b;
	
	printf("%p\n", a-100);
}