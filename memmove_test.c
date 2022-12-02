#include <stdio.h>
#include <stdlib.h>

char str[] = "Hell world....";

int main() {
	printf("Original: %s (shift by %d)\n", str, strlen(str)-5);

	memmove(
		str+5,
		str+4,
		strlen(str)-5
	);

	str[4] = 'o';

	printf("Result: %s\n", str);
	
	return 0;
}
