#include "my_malloc.h"

int main()
{
	char memory[4096];
	char *str[10];
	
	
	sprintf(&memory[0], "aaabbbcccdddeee");
	sprintf(&memory[15], "111222333");
	printf("%s\n", &memory[0]);
	printf("%s\n", &memory[15]);
	printf("\n***\n");
	
	init_malloc(&memory[4096], &memory[1024]);
	
	str[0] = my_malloc(17);
	sprintf(str[0], "1234567890abcdef");
	printf("%s\n", str[0]);
	printf("\n***\n");
	
	str[1] = my_malloc(9);
	sprintf(str[1], "qwertyui");
	printf("%s\n", str[1]);
	printf("\n***\n");
	
	
	str[2] = my_malloc(13);
	sprintf(str[2], "hello world!");
	printf("%s\n", str[2]);
	printf("\n***\n");
	
	//str[3] = my_malloc(44);
	str[3] = my_malloc(250);
	sprintf(str[3], "The quick brown fox jumps over the lazy dog");
	printf("%s\n", str[3]);
	printf("\n***\n");
	
	str[4] = my_malloc(128);
	sprintf(str[4], "Sometimes I dream about Cheese");
	printf("%s\n", str[4]);
	printf("\n***\n");
	
	/*
	str[5] = my_malloc(128);
	sprintf(str[5], "GET OFF MY PLANE\n");
	printf("%s\n", str[5]);
	printf("\n***\n");
	*/
	

	printf("\n***Testing originals***\n");
	printf("%s\n", &memory[0]);
	printf("%s\n", &memory[15]);
	printf("%s\n", str[0]);
	printf("%s\n", str[1]);
	printf("%s\n", str[2]);
	printf("%s\n", str[3]);
	printf("%s\n", str[4]);
	//printf("%s\n", str[5]);
	
}

void test_stack()
{
	int i;
	int arrtest[3] = {1, 2, 3};
	
	for(i=0; i<3; i++)
		printf("arrtest[%d] = %p\n", i, &arrtest[i]);
}