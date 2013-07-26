// Airscan
// By Cykey.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <CoreFoundation/CoreFoundation.h>
#include <MobileWiFi/MobileWiFi.h>

static WiFiManagerRef manager;
static WiFiDeviceClientRef device;
static bool verbose_mode = false;
static bool scan_mode = false;

static void pretty_print_network(WiFiNetworkRef network);
static inline void verbose_log(char *format, ...);
static void scan_callback(WiFiDeviceClientRef device, CFArrayRef results, CFErrorRef error, void *unknown);
static void begin_scan();
static void print_usage(char *progname);

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

	CFStringRef format = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%-40s BSSI: %s, RSSI: %.0f dBm, CHANNEL: %d, HIDDEN: %@, EAP: %@, WEP: %@, WPA: %@"), SSID, BSSID, strength, channel, isHidden, isEAP, isWEP, isWPA);
	CFShow(format);
	CFRelease(format);
}

static inline void verbose_log(char *format, ...)
{
	if (verbose_mode) {
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
}

static void scan_callback(WiFiDeviceClientRef device, CFArrayRef results, CFErrorRef error, void *unknown)
{
	verbose_log("Finished scanning. Found %d networks.\n", CFArrayGetCount(results));

	unsigned x;
	for (x = 0; x < CFArrayGetCount(results); x++) {
		WiFiNetworkRef network = (WiFiNetworkRef)CFArrayGetValueAtIndex(results, x);

		pretty_print_network(network);
	}

	CFRunLoopStop(CFRunLoopGetCurrent());
}

static void begin_scan()
{
	verbose_log("Creating WiFiManagerClient...\n");
	manager = WiFiManagerClientCreate(kCFAllocatorDefault, 0);
	assert(manager != NULL);
	CFArrayRef devices = WiFiManagerClientCopyDevices(manager);
	assert(devices != NULL);
	verbose_log("Getting WiFiDeviceClient...\n");
	device = (WiFiDeviceClientRef)CFArrayGetValueAtIndex(devices, 0);
	assert(device != NULL);
	CFRelease(devices);

	verbose_log("Scheduling manager client with run loop...\n");
	WiFiManagerClientScheduleWithRunLoop(manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

	// It is necessary to pass an empty dictionary (NULL does not work).
	CFDictionaryRef options = CFDictionaryCreate(NULL, NULL, NULL, 0, NULL, NULL);
	verbose_log("Starting to scan...\n");
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
