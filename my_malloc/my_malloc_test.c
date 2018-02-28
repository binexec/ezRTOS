#include "my_malloc.h"

void* malloc_dbg(size_t len)
{
	void *retval = my_malloc(len);
	check_alignment();
	return retval;
}

void free_dbg(void *p)
{
	my_free(p);
	check_alignment();
}

void* realloc_dbg(void *ptr, size_t len)
{
	void *retval = my_realloc(ptr, len);
	check_alignment();
	return retval;
}



/***************/



void test_stack()
{
	int i;
	int arrtest[3] = {1, 2, 3};
	
	for(i=0; i<3; i++)
		printf("arrtest[%d] = %p\n", i, &arrtest[i]);
}

void test_malloc()
{
	char memory[4096];
	char *str[10];
	
	
	sprintf(&memory[0], "aaabbbcccdddeee");
	sprintf(&memory[15], "111222333");
	printf("%s\n", &memory[0]);
	printf("%s\n", &memory[15]);
	
	init_malloc(&memory[4096], &memory[1024]);
	
	str[0] = malloc_dbg(17);
	sprintf(str[0], "1234567890abcdef");
	printf("%s\n", str[0]);
	printf("\n***\n");
	
	str[1] = malloc_dbg(9);
	sprintf(str[1], "qwertyui");
	printf("%s\n", str[1]);
	printf("\n***\n");
	
	
	str[2] = malloc_dbg(13);
	sprintf(str[2], "hello world!");
	printf("%s\n", str[2]);
	printf("\n***\n");
	
	
	//str[3] = malloc_dbg(44);
	str[3] = malloc_dbg(256);
	sprintf(str[3], "The quick brown fox jumps over the lazy dog");
	printf("%s\n", str[3]);
	printf("\n***\n");
	
	//allocating right to the break
	str[4] = malloc_dbg(137);
	sprintf(str[4], "Sometimes I dream about Cheese");
	printf("%s\n", str[4]);
	printf("\n***\n");
	
	
	str[5] = malloc_dbg(32);
	sprintf(str[5], "GET OFF MY PLANE");
	printf("%s\n", str[5]);
	printf("\n***\n");
	
	
	str[6] = malloc_dbg(12);
	sprintf(str[6], "i wish p=np");
	printf("%s\n", str[6]);
	printf("\n***\n");
	
	
	

	printf("\n***Testing originals***\n");
	printf("%s\n", &memory[0]);
	printf("%s\n", &memory[15]);
	printf("%s\n", str[0]);
	printf("%s\n", str[1]);
	printf("%s\n", str[2]);
	printf("%s\n", str[3]);
	printf("%s\n", str[4]);
	printf("%s\n", str[5]);
	printf("%s\n", str[6]);
}


void test_free()
{
	char memory[4096];
	char *str[10];
	
	sprintf(&memory[0], "aaabbbcccdddeee");
	sprintf(&memory[15], "111222333");
	printf("%s\n", &memory[0]);
	printf("%s\n", &memory[15]);
	
	init_malloc(&memory[4096], &memory[1024]);
	
	str[0] = malloc_dbg(17);
	sprintf(str[0], "1234567890abcdef");
	printf("%s\n", str[0]);
	printf("\n***\n");
	
	str[1] = malloc_dbg(9);
	sprintf(str[1], "qwertyui");
	printf("%s\n", str[1]);
	printf("\n***\n");
	
	str[2] = malloc_dbg(13);
	sprintf(str[2], "hello world!");
	printf("%s\n", str[2]);
	printf("\n***\n");
	
	str[3] = malloc_dbg(64);
	sprintf(str[3], "The quick brown fox jumps over the lazy dog");
	printf("%s\n", str[3]);
	printf("\n***\n");
	
	str[4] = malloc_dbg(65);
	sprintf(str[4], "Sometimes I dream about Cheese");
	printf("%s\n", str[4]);
	printf("\n***\n");
	
	str[5] = malloc_dbg(66);
	sprintf(str[5], "GET OFF MY PLANE");
	printf("%s\n", str[5]);
	printf("\n***\n");
	
	str[6] = malloc_dbg(13);
	sprintf(str[6], "i wish p=np");
	printf("%s\n", str[6]);
	printf("\n***\n");
	
	str[7] = malloc_dbg(403);
	sprintf(str[7], "THIS IS A REALLY BIG PIECE AT THE END");
	printf("%s\n", str[7]);
	printf("\n***\n");
	
	str[8] = malloc_dbg(13);
	sprintf(str[8], "i wish p=np");
	printf("%s\n", str[8]);
	printf("\n***\n");
	
	printf("\n**************FREEING******************\n");
	
	/*printf("\n***\n");
	free_dbg(str[3]);
	printf("\n***\n");
	
	printf("\n***\n");
	free_dbg(str[5]);
	printf("\n***\n");
	
	//testing merge
	printf("\n***\n");
	free_dbg(str[4]);
	printf("\n***\n");*/
	
	
	//testing break reduction
	printf("\n***\n");
	free_dbg(str[8]);
	printf("\n***\n");
	
	printf("\n***\n");
	free_dbg(str[7]);
	printf("\n***\n");
	
	/*//testing exact piece
	printf("\n***\n");
	free_dbg(str[1]);
	printf("\n***\n");*/
	
	str[1] = malloc_dbg(9);
	sprintf(str[1], "iuytrewq");
	printf("%s\n", str[1]);
	printf("\n***\n");
	
	str[3] = malloc_dbg(69);
	sprintf(str[3], "6969696969696969696969696969");
	printf("%s\n", str[3]);
	printf("\n***\n");
	
	printf("\n***Testing originals***\n");
	printf("%s\n", &memory[0]);
	printf("%s\n", &memory[15]);
	printf("%s\n", str[0]);
	printf("%s\n", str[2]);
	printf("%s\n", str[6]);
	printf("%s\n", str[1]);
	printf("%s\n", str[3]);
	
	
	
}

void test_realloc()
{
	
}


int main()
{
	
	//test_malloc();
	test_free();
	
}
