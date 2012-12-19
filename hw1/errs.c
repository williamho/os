#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

main() {
	fprintf(stderr,"%s",strerror(116));
}
