#include <stdio.h>
#include <string.h>

int main() {
	char s[100];
	printf("what is your name? ");
	fgets(s, sizeof(s), stdin);
	
	if (s[strlen(s) - 1] == '\n') {
		s[strlen(s) - 1] = '\0';
	}
	
	printf("hello, %s!\n", s);
	
	return 0;
}

