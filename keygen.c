#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		printf("incorect number of arguments\n");
		exit(1);
	}

	int i;
	for (i = 0; i < strlen(argv[1]); ++i)
	{
		if (!isdigit(argv[1][i]))
		{
			printf("argument not an integer\n");
			exit(1);
		}
			
	}

	int length 0;
	length = atoi(argv[1]);
	char key[length +1];
	memset(key, sizeof(key), '\0');
	srand(time(NULL));

	int j;
	for (j = 0; j < length; ++j)
	{
		int randInt = rand() % 27;
		if (randInt == 0)
		{
			randInt = 32;
		}
		else
		{
			randInt += 64;
		}
		key[j] = randInt;
	}
	printf("%s\n", key);
	return 0;
}