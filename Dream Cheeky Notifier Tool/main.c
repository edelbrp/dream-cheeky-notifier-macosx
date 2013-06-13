//
//	File:		main.c
//
//	Contains:	Dream Cheeky Notifier driver based on Apple's HID LED test tool
//
//	Copyright:	Copyright (c) 2007 Apple Inc., and (c) 2013 Philip Edelbrock <phil@philedelbrock.com>, All Rights Reserved
//
//  Usage:		Permission is given by Apple and Phil Edelbrock to reuse/repurpose this code without restriction.  Please
//				refer to Apple's original HID LED test tool source for further usage/disclaimer information.
//
//	Output:		The result of this project is a command line tool that will find all Dream Cheeky webmail notifier
//				devices on the USB bus(es), activate their LEDs and set them to the color provided:
//
//					Dream\ Cheeky\ Notifier R G B [A]
//
//				Where RGB are intensities, from 0 to 31 (31 fully on, 0 off).  A is an optional parameter that can be used
//				to skip the LED activation process (may reduce blinking of the device?).  A being 0 (zero) means do the LED
//				activation (the default), any other value means skip the activation process.
//
#pragma mark -
#pragma mark * complation directives *
// ----------------------------------------------------

#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif

// ****************************************************
#pragma mark -
#pragma mark * includes & imports *
// ----------------------------------------------------

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>

// ****************************************************
#pragma mark -
#pragma mark * typedef's, struct's, enums, defines, etc. *
// ----------------------------------------------------
// function to create a matching dictionary for usage page & usage
static CFMutableDictionaryRef hu_CreateMatchingDictionaryUsagePageUsage( Boolean isDeviceNotElement,
																		UInt32 inUsagePage,
																		UInt32 inUsage )
{
	// create a dictionary to add usage page / usages to
	CFMutableDictionaryRef result = CFDictionaryCreateMutable( kCFAllocatorDefault,
															  0,
															  &kCFTypeDictionaryKeyCallBacks,
															  &kCFTypeDictionaryValueCallBacks );
	
	if ( result ) {
		if ( inUsagePage ) {
			// Add key for device type to refine the matching dictionary.
			CFNumberRef pageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsagePage );
			
			if ( pageCFNumberRef ) {
				if ( isDeviceNotElement ) {
					CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsagePageKey ), pageCFNumberRef );
				} else {
					CFDictionarySetValue( result, CFSTR( kIOHIDElementUsagePageKey ), pageCFNumberRef );
				}
				CFRelease( pageCFNumberRef );
				
				// note: the usage is only valid if the usage page is also defined
				if ( inUsage ) {
					CFNumberRef usageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsage );
					
					if ( usageCFNumberRef ) {
						if ( isDeviceNotElement ) {
							CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsageKey ), usageCFNumberRef );
						} else {
							CFDictionarySetValue( result, CFSTR( kIOHIDElementUsageKey ), usageCFNumberRef );
						}
						CFRelease( usageCFNumberRef );
					} else {
						fprintf( stderr, "%s: CFNumberCreate( usage ) failed.", __PRETTY_FUNCTION__ );
					}
				}
			} else {
				fprintf( stderr, "%s: CFNumberCreate( usage page ) failed.", __PRETTY_FUNCTION__ );
			}
		}
	} else {
		fprintf( stderr, "%s: CFDictionaryCreateMutable failed.", __PRETTY_FUNCTION__ );
	}
	return result;
}	// hu_CreateMatchingDictionaryUsagePageUsage

// function to get a long device property
// returns FALSE if the property isn't found or can't be converted to a long
static Boolean IOHIDDevice_GetLongProperty(
										   IOHIDDeviceRef inDeviceRef,     // the HID device reference
										   CFStringRef inKey,              // the kIOHIDDevice key (as a CFString)
										   long * outValue)                // address where to return the output value
{
    Boolean result = FALSE;
	
    CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty(inDeviceRef, inKey);
    if (tCFTypeRef) {
        // if this is a number
        if (CFNumberGetTypeID() == CFGetTypeID(tCFTypeRef)) {
            // get its value
            result = CFNumberGetValue((CFNumberRef) tCFTypeRef, kCFNumberSInt32Type, outValue);
        }
    }
    return result;
}   // IOHIDDevice_GetLongProperty

int main( int argc, const char * argv[] )
{
	Boolean initialized = FALSE;
	
	if (( argc != 4 ) && ( argc != 5 ))  {
		printf ( "usage: Dream Cheeky Notifier R G B [A]\n\tRGB values should be 0-31.  A is an optional parameter on whether to do the LED activation sequence.  Anything larger than 0 is YES, default is 0 (activate).\n\n");
		exit ( -1 );
	}
	
	char r = (int)strtol ( argv[1], NULL, 10 );
	char g = (int)strtol ( argv[2], NULL, 10 );
	char b = (int)strtol ( argv[3], NULL, 10 );
	if ( argc == 5 ) {
		initialized = TRUE;
	}
	
	if ((r < 0) || (r > 31) || (g < 0) || (g > 31) || (b < 0) || (b > 31)) {
		printf("RGB values must be within 0-31.");
		exit -1;
	}
	
	// create a IO HID Manager reference
	IOHIDManagerRef tIOHIDManagerRef = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
	// Create a device matching dictionary
	CFDictionaryRef matchingCFDictRef = hu_CreateMatchingDictionaryUsagePageUsage( TRUE,
																				  kHIDPage_GenericDesktop,
																				  0x10 );
	// set the HID device matching dictionary
	IOHIDManagerSetDeviceMatching( tIOHIDManagerRef, matchingCFDictRef );
	if ( matchingCFDictRef ) {
		CFRelease( matchingCFDictRef );
	}
	
	// Now open the IO HID Manager reference
	IOReturn tIOReturn = IOHIDManagerOpen( tIOHIDManagerRef, kIOHIDOptionsTypeNone );
	
	// and copy out its devices
	CFSetRef deviceCFSetRef = IOHIDManagerCopyDevices( tIOHIDManagerRef );
	
	// how many devices in the set?
	CFIndex deviceIndex, deviceCount = CFSetGetCount( deviceCFSetRef );
	
	// allocate a block of memory to extact the device ref's from the set into
	IOHIDDeviceRef * tIOHIDDeviceRefs = malloc( sizeof( IOHIDDeviceRef ) * deviceCount );
	
	// now extract the device ref's from the set
	CFSetGetValues( deviceCFSetRef, (const void **) tIOHIDDeviceRefs );
	
	// before we get into the device loop we'll setup our element matching dictionary (Note: we don't do element matching anymore)
	matchingCFDictRef = hu_CreateMatchingDictionaryUsagePageUsage( FALSE, 0, 0 );
	
	for ( deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++ ) {
		
		// if this isn't the notifier device...  TODO: let's detect via vendor/product ids instead
		long vendor_id = 0;
		IOHIDDevice_GetLongProperty(tIOHIDDeviceRefs[deviceIndex], CFSTR(kIOHIDVendorIDKey), &vendor_id);
		long product_id = 0;
		IOHIDDevice_GetLongProperty(tIOHIDDeviceRefs[deviceIndex], CFSTR(kIOHIDProductIDKey), &product_id);
		if ((vendor_id != 0x1D34 ) || (product_id != 0x0004 )) {
			printf("	skipping device %p.\n", tIOHIDDeviceRefs[deviceIndex] );
			continue;	// ...skip it
		}
		
		printf( "	 device = %p.\n", tIOHIDDeviceRefs[deviceIndex] );
		
		unsigned char report[8];
		if (initialized == FALSE) {
			report[0] = 0x1F; // turn on LEDs
			report[1] = 0x02;
			report[2] = 0x00;
			report[3] = 0x5F;
			report[4] = 0x00;
			report[5] = 0x00;
			report[6] = 0x1A;
			report[7] = 0x03;
			
            // Note: We don't use the returned value here, so the compiler might throw a warning.
			IOReturn  tIOReturn = IOHIDDeviceSetReport(
													   tIOHIDDeviceRefs[deviceIndex],          // IOHIDDeviceRef for the HID device
													   kIOHIDReportTypeInput,   // IOHIDReportType for the report (input, output, feature)
													   0,           // CFIndex for the report ID
													   report,             // address of report buffer
													   8);      // length of the report
			initialized = TRUE;
		}
		
		report[0] = r; // set brightness on LEDs to r, g, & b
		report[1] = g;
		report[2] = b;
		report[3] = 0x00;
		report[4] = 0x00;
		report[5] = 0x00;
		report[6] = 0x1A;
		report[7] = 0x05;
		
		tIOReturn = IOHIDDeviceSetReport(
										 tIOHIDDeviceRefs[deviceIndex],          // IOHIDDeviceRef for the HID device
										 kIOHIDReportTypeInput,   // IOHIDReportType for the report (input, output, feature)
										 0,           // CFIndex for the report ID
										 report,             // address of report buffer
										 8);      // length of the report
		
	next_device: ;
		continue;
	}
	
	
	if ( tIOHIDManagerRef ) {
		CFRelease( tIOHIDManagerRef );
	}
	
	if ( matchingCFDictRef ) {
		CFRelease( matchingCFDictRef );
	}
Oops:	;
	return -1;
} /* main */
