#include <time.h>
#include <stdio.h>

main() {
	struct tm tm;
	strptime("0m12.50s", "%Mm%S.%Ys",&tm);
	printf("%d.%d\n",tm.tm_sec,tm.tm_year+1900);
	printf("%d secs",tm.tm_min*60+tm.tm_sec+tm.tm_year+1900);
}
