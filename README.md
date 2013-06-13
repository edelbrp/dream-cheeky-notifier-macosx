Dream Cheeky Notifier for MacOS-X
============================

This is a very basic command line tool for illuminating a Dream Cheeky USB notifier appliance in any color.

        The result of this project is a command line tool that will find all Dream Cheeky webmail notifier
  	devices on the USB bus(es), activate their LEDs and set them to the color provided:

		Dream\ Cheeky\ Notifier R G B [A]

	Where RGB are intensities, from 0 to 31 (31 fully on, 0 off).  A is an optional parameter that can be used
	to skip the LED activation process (may reduce blinking of the device?).  A being 0 (zero) means do the LED
	activation (the default), any other value means skip the activation process.
