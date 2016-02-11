all : evdev-joystick

evdev-joystick: evdev-joystick.c
	gcc -Wall -O2 -o evdev-joystick evdev-joystick.c

install: evdev-joystick 80-stelladaptor-joystick.rules
	install -s evdev-joystick /usr/local/bin/
	install -m 644 80-stelladaptor-joystick.rules /etc/udev/rules.d/

clean:
	rm -rf evdev-joystick
