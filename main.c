// Airscan
// By Cykey.

/* TODO: -Add asserts() and do verbose mode thing. */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <MobileWiFi/MobileWiFi.h>

static WiFiManagerRef manager;
static WiFiDeviceClientRef device;
static bool verbose_mode = false;
static bool scan_mode = false;

static void scan_callback(WiFiDeviceClientRef device, CFArrayRef results, CFErrorRef error, void *unknown)
{
	CFShow(results);

	CFRunLoopStop(CFRunLoopGetCurrent());
}

static void begin_scan()
{
	manager = WiFiManagerClientCreate(kCFAllocatorDefault, 0);
	CFArrayRef devices = WiFiManagerClientCopyDevices(manager);
    device = (WiFiDeviceClientRef)CFArrayGetValueAtIndex(devices, 0);
    CFRelease(devices);

    WiFiManagerClientScheduleWithRunLoop(manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    // It is necessary to pass an empty dictionary (NULL does not work).
    CFDictionaryRef options = CFDictionaryCreate(NULL, NULL, NULL, 0, NULL, NULL);
    WiFiDeviceClientScanAsync(device, options, scan_callback, 0);
    CFRelease(options);
}

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

	if (scan_mode) {
		begin_scan();
		CFRunLoopRun();
	}

	return 0;
}
