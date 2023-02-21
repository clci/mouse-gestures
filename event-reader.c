/**
 * gcc -o event-reader event-reader.c `pkg-config --cflags --libs libinput libudev` -lm
 *
 * https://wayland.freedesktop.org/libinput/doc/latest/api/
 * to get button codes:
 * libinput debug-events
 * see MOUSE_GESTURE_BTN_CODE constant below
**/

 /*

/etc/X11/xorg.conf.d/M590-button-map.conf:
Section "InputClass"
	Identifier      "M590 button map"
	MatchProduct    "M590"
	Option          "ButtonMapping" "1 2 3 4 5 6 7 30 31 10 11 12 13 14 15 16 17 18 19 20"
EndSection

 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <libudev.h>
#include <libinput.h>
#include <math.h>

#define SWIPE_EMIT_DISTANCE 30
#define MOUSE_EMIT_DISTANCE 120
#define MOUSE_GESTURE_BTN_CODE 275
#define MOUSE_CUSTOM_BTN_1 276


static int open_restricted(const char *path, int flags, void *user_data) {
	int fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
	close(fd);
}

const static struct libinput_interface interface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted
};

int main(void) {

	struct libinput *li, *gesture_li;
	struct libinput_event *ev;
	struct udev *udev = udev_new();
	struct libinput_device *dev;
	struct libinput_device *gesture_dev = NULL;
	struct pollfd fds;
	int count = 0;
	int stop = false;
	bool swipe_emitted;
	double swipe_x, swipe_y;
	double ang;
	char *type;

	struct libinput_event_gesture *gesture_ev;
	struct libinput_event_pointer *pointer_ev;
	uint32_t btn_pressed;
	bool mouse_gesture = false;
	double mg_x, mg_y;

	li = libinput_udev_create_context(&interface, NULL, udev);
	libinput_udev_assign_seat(li, "seat0");

	fds.fd = libinput_get_fd(li);
	fds.events = POLLIN;
	fds.revents = 0;

	while (!stop && poll(&fds, 1, -1) > -1) {

		libinput_dispatch(li);

		while (ev = libinput_get_event(li)) {

			switch (libinput_event_get_type(ev)) {

				case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
					swipe_x = swipe_y = 0.0;
					swipe_emitted = false;
					type = "SWIPE_GESTURE_BEGIN";
				break;

				case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:

					if (!swipe_emitted) {

						gesture_ev = libinput_event_get_gesture_event(ev);
						swipe_x += libinput_event_gesture_get_dx(gesture_ev);
						swipe_y += libinput_event_gesture_get_dy(gesture_ev);

						if (sqrt(pow(swipe_x, 2.0) + pow(swipe_y, 2.0)) >= SWIPE_EMIT_DISTANCE) {

							ang = atan2(swipe_y, swipe_x) * (180 / M_PI);
							if ((ang < 15.0 && ang >= 0) || (ang > -15.0 && ang <= 0)) {
								printf("event: SWIPE_GESTURE_RIGHT_INCOMPLETE\n");
							} else if (ang > 75.0 && ang < 105.0) {
								printf("event: SWIPE_GESTURE_DOWN_INCOMPLETE\n");
							} else if (ang > 165.0 || ang < -165.0){
								printf("event: SWIPE_GESTURE_LEFT_INCOMPLETE\n");
							} else if (ang > -105.0 && ang < -75.0) {
								printf("event: SWIPE_GESTURE_UP_INCOMPLETE\n");
							}

							fflush(stdout);
							swipe_emitted = true;
						}

					}

					type = "SWIPE_GESTURE_UPDATE";
				break;

				case LIBINPUT_EVENT_GESTURE_SWIPE_END:
					type = "SWIPE_GESTURE_END";
				break;

				case LIBINPUT_EVENT_DEVICE_ADDED:
					type = "device added event.";
				break;

				case LIBINPUT_EVENT_POINTER_BUTTON:

					pointer_ev = libinput_event_get_pointer_event(ev);
					uint32_t btn;
					switch (btn = libinput_event_pointer_get_button(pointer_ev)) {

						case MOUSE_GESTURE_BTN_CODE:
							btn_pressed = libinput_event_pointer_get_button_state(pointer_ev);
							if (btn_pressed) {
								mouse_gesture = true;
								mg_x = mg_y = 0.0;
								type = "MOUSE_GESTURE_BEGIN";
							} else {
								mouse_gesture = false;
								type = "MOUSE_GESTURE_END";
							}
						break;

						case MOUSE_CUSTOM_BTN_1:
							btn_pressed = libinput_event_pointer_get_button_state(pointer_ev);
							if (btn_pressed) {
								printf("event: MOUSE_CUSTOM_BTN_1_PRESSED\n");
							} else {
								printf("event: MOUSE_CUSTOM_BTN_1_RELEASED\n");
							}
							fflush(stdout);
						break;

						default:
						break;

					}

				break;

				case LIBINPUT_EVENT_POINTER_MOTION:

					if (mouse_gesture) {

						pointer_ev = libinput_event_get_pointer_event(ev);
						mg_x += libinput_event_pointer_get_dx(pointer_ev);
						mg_y += libinput_event_pointer_get_dy(pointer_ev);

						if (sqrt(pow(mg_x, 2.0) + pow(mg_y, 2.0)) >= MOUSE_EMIT_DISTANCE) {

							ang = atan2(mg_y, mg_x) * (180 / M_PI);
							if ((ang < 15.0 && ang >= 0) || (ang > -15.0 && ang <= 0)) {
								printf("event: MOUSE_GESTURE_RIGHT_INCOMPLETE\n");
							} else if (ang > 75.0 && ang < 105.0) {
								printf("event: MOUSE_GESTURE_DOWN_INCOMPLETE\n");
							} else if (ang > 165.0 || ang < -165.0){
								printf("event: MOUSE_GESTURE_LEFT_INCOMPLETE\n");
							} else if (ang > -105.0 && ang < -75.0) {
								printf("event: MOUSE_GESTURE_UP_INCOMPLETE\n");
							}

							fflush(stdout);
							mouse_gesture = false;
						}
					}

					type = "MOUSE_GESTURE_UPDATE";

				break;

				default:
					type = NULL;
				break;

			}

			libinput_event_destroy(ev);
			libinput_dispatch(li);

		}

	}

	libinput_unref(li);

	return 0;

}

