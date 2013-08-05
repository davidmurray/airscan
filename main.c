// Airscan
// By Cykey.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <CoreFoundation/CoreFoundation.h>
#include <MobileWiFi/MobileWiFi.h>

/*
 * TODO:
 * Fix "Must specify either -s or -h." stuff
 * More colors
 * Clean up
 */

// Colors!
// (Taken from http://stackoverflow.com/a/3586005/1218712.)
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#define ERROR(x) \
	fprintf(stderr, KRED "[Error] " KNRM x); \
	exit(EXIT_FAILURE);

static WiFiManagerRef manager;
static WiFiDeviceClientRef device;
static CFRunLoopTimerRef timer;
static bool verboseMode = false;
static bool timerMode = false;

static void pretty_print_network(WiFiNetworkRef network);
static inline void verbose_log(char *format, ...);
static void scan_callback(WiFiDeviceClientRef device, CFArrayRef results, CFErrorRef error, void *unknown);
static void begin_scan();
static void setup_for_scan();
static void print_usage(char *progname);
static void signal_handler(int signal);

static inline void verbose_log(char *format, ...)
{
	if (verboseMode) {
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
}

static void pretty_print_network(WiFiNetworkRef network)
{
	// SSID & BSSID.
	// FIXME: Add safety checks and use CFStringGetCString if CFStringGetCStringPtr fails.
	const char *SSID = CFStringGetCStringPtr(WiFiNetworkGetSSID(network), kCFStringEncodingMacRoman);
	const char *BSSID = CFStringGetCStringPtr(WiFiNetworkGetProperty(network, CFSTR("BSSID")), kCFStringEncodingMacRoman);

	// RSSI.
	CFNumberRef RSSI = (CFNumberRef)WiFiNetworkGetProperty(network, kWiFiScaledRSSIKey);
	float strength;
	CFNumberGetValue(RSSI, kCFNumberFloatType, &strength);

	strength = strength * 100;

	// Round to the nearest integer.
	strength = ceilf(strength);

	// Convert to a negative number.
	strength = strength * -1;

	CFNumberRef networkChannel = (CFNumberRef)WiFiNetworkGetProperty(network, CFSTR("CHANNEL"));
	int channel;
	CFNumberGetValue(networkChannel, kCFNumberIntType, &channel);

	// Hidden.
	CFStringRef isHidden = (WiFiNetworkIsHidden(network) ? CFSTR("YES") : CFSTR("NO"));

	// Security model.
	CFStringRef isEAP = (WiFiNetworkIsEAP(network) ? CFSTR("YES") : CFSTR("NO"));
	CFStringRef isWEP = (WiFiNetworkIsWEP(network) ? CFSTR("YES") : CFSTR("NO"));
	CFStringRef isWPA = (WiFiNetworkIsWPA(network) ? CFSTR("YES") : CFSTR("NO"));

	CFStringRef format = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s%-40s%s BSSI: %s, RSSI: %.0f dBm, CHANNEL: %d, HIDDEN: %@, EAP: %@, WEP: %@, WPA: %@"), KBLU, SSID, KNRM, BSSID, strength, channel, isHidden, isEAP, isWEP, isWPA);
	CFShow(format);
	CFRelease(format);
}

static void scan_callback(WiFiDeviceClientRef device, CFArrayRef results, CFErrorRef error, void *unknown)
{
	CFIndex count = CFArrayGetCount(results);

	verbose_log("Finished scanning. Found %s%d%s networks.\n", KGRN, count, KNRM);

	unsigned x;
	for (x = 0; x < count; x++) {
		WiFiNetworkRef network = (WiFiNetworkRef)CFArrayGetValueAtIndex(results, x);

		pretty_print_network(network);
	}

	if (!timerMode)
		CFRunLoopStop(CFRunLoopGetCurrent());
	else
		// Print a newline so that results aren't all stuck together.
		printf("\n");

}

static void setup_for_scan()
{
	verbose_log("Creating WiFiManagerClient.\n");

	manager = WiFiManagerClientCreate(kCFAllocatorDefault, 0);
	if (!manager) {
		ERROR("Couldn't create WiFiManagerClient.\n");
	}

	CFArrayRef devices = WiFiManagerClientCopyDevices(manager);
	if (!devices) {
		ERROR("Couldn't get WiFiDeviceClients.\n");
	}

	verbose_log("Getting WiFiDeviceClient.\n");

	device = (WiFiDeviceClientRef)CFArrayGetValueAtIndex(devices, 0);
	CFRelease(devices);

	verbose_log("Scheduling manager client with run loop...\n");
	WiFiManagerClientScheduleWithRunLoop(manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
}

static void begin_scan()
{
	if (!device || !manager) {
		verbose_log("First scan; creating necessary objects.\n");
		setup_for_scan();
	}

	// It is necessary to pass an empty dictionary (NULL does not work).
	CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, NULL, NULL, 0, NULL, NULL);
	verbose_log("Starting to scan...\n");
	WiFiDeviceClientScanAsync(device, options, scan_callback, 0);
	CFRelease(options);
}

static void print_usage(char *progname)
{
	printf("Airscan 1.1\nUsage: %s <options>\nOptions include:\n", progname);
	printf("\t-s:\tScan for nearby WiFi networks.\n");
	printf("\t-t: [secs] Scan again every X (defaults to 10) seconds\n");
	printf("\t-v:\tEnable verbose mode.\n");
	printf("\t-h:\tPrint this help.\n");
}

static void signal_handler(int signal)
{
	printf("Got signal: %d. Exiting.\n", signal);

	if (manager) {
		CFRelease(manager);
	}

	if (timer) {
		CFRelease(timer);
	}

	exit(0);
}

int main(int argc, char *argv[])
{
	long int timerInterval;
	int c;
	bool scanMode = false;
	bool helpMode = false;

	if (argc < 2) {
		printf("Must specify either -s or -h.\n");
		return -1;
	}

	signal(SIGINT, signal_handler);

	while ((c = getopt(argc, argv, "svht")) != -1) {
		switch (c) {
			case 's':
				scanMode = true;
				break;
			case 't':
				if (argv[optind]) {
					timerInterval = strtol(argv[optind], NULL, 10);
				} else {
					verbose_log("No timer interval was specified. Using default (10).\n");
					timerInterval = 10;
				}

				timerMode = true;
				break;
			case 'v':
				verboseMode = true;
				break;
			case 'h':
				helpMode = true;
				break;
			default:
				printf("Must specify either -s or -h.\n");
				return -1;
			}
	}

	// XXX: Is there a better way to handle all of this?

	if (helpMode) {
		print_usage(argv[0]);
		return 0;
	}

	if (!helpMode && !scanMode) {
		printf("Must specify either -s or -h.\n");
		return -1;
	}

	if (scanMode && !timerMode) {
		begin_scan();
		CFRunLoopRun();
	}

	if (timerMode) {
		verbose_log("Creating timer with interval: %ld\n", timerInterval);

		timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(), timerInterval, 0, 0, (CFRunLoopTimerCallBack)begin_scan, NULL);
		CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
		CFRunLoopRun();
	}

	return 0;
}
