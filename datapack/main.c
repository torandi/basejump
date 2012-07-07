#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "files.h"

int main(int argc, const char* argv[]){
	unpack_override("spam/data/test");

	char* d1; unpack(&DATA1, &d1);
	char* d2; unpack_filename("data3.txt", &d2);

	FILE* fp = unpack_open("data3.txt", "w");
	if ( fp ){
		fprintf(fp, "hurr durr\n");
		fclose(fp);
	} else {
		fprintf(stderr, "failed to open: %s\n", strerror(errno));
	}

	printf("data1: %s\n", d1);
	printf("data3: %s\n", d2);

	free(d1);
	free(d2);

	return 0;
}
