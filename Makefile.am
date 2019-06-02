AM_LDFLAGS = \
	-Wl,--as-needed

data_DATA =

bin_PROGRAMS = \
	gbsim

gbsim_SOURCES = \
	arpc.h \
	config.h \
	connection.c \
	functionfs.c \
	gadget.c \
	gbsim.h \
	gbsim_usb.h \
	gbsim_usb.c \
	gpio.c \
	greybus_protocols.h \
	greybus_manifest.h \
	control.c \
	svc.c \
	i2c.c \
	interface.c \
	inotify.c \
	main.c \
	manifest.c \
	pwm.c \
	spi.c \
	uart.c

gbsim_CPPFLAGS = \
	-Wall \
	-Wformat \
	-Wshadow \
	$(GBUS_CPPFLAGS) \
	$(AM_CPPFLAGS) \
	$(SOC_CFLAGS) \
	$(USBG_CFLAGS)

gbsim_LDADD = \
	$(SOC_LIBS) \
	$(USBG_LIBS)


distclean-local:
	rm -rf autom4te.cache
