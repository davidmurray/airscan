// Airscan
// By Cykey.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

static bool verbose_mode = false;
static bool scan_mode = false;

static void print_usage(char *progname)
{
	printf("\nUsage: %s <options>\nOptions include:\n", progname);
	printf("\t-s:\tScan for nearby WiFi networks.\n");
	printf("\t-v:\tEnable verbose mode.\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage(argv[0]);
		exit(0);
	}

	int c;
	while ((c = getopt(argc, argv, "sv")) != -1) {
		switch (c) {
			case 's':
				scan_mode = true;
				break;
			case 'v':
				verbose_mode = true;
				break;
			default:
				print_usage(argv[0]);
				exit(0);
			}
	}

	return 0;
}
