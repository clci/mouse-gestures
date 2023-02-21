import sys, os, time, subprocess


EVENT_TO_KEY_MAP = {
    # b'SWIPE_GESTURE_UP_INCOMPLETE': 'super+Down',
    # b'SWIPE_GESTURE_RIGHT_INCOMPLETE': 'super+Left',
    # b'SWIPE_GESTURE_DOWN_INCOMPLETE': 'super+Up',
    # b'SWIPE_GESTURE_LEFT_INCOMPLETE': 'super+Right',
    # b'MOUSE_GESTURE_UP_INCOMPLETE': 'super+Down',
    # b'MOUSE_GESTURE_RIGHT_INCOMPLETE': 'super+Left',
    # b'MOUSE_GESTURE_DOWN_INCOMPLETE': 'super+Up',
    # b'MOUSE_GESTURE_LEFT_INCOMPLETE': 'super+Right'
    b'SWIPE_GESTURE_UP_INCOMPLETE': 'keydown super keydown Down keyup Down keyup super',
    b'SWIPE_GESTURE_RIGHT_INCOMPLETE': 'keydown super keydown Left keyup Left keyup super',
    b'SWIPE_GESTURE_DOWN_INCOMPLETE': 'keydown super keydown Up keyup Up keyup super',
    b'SWIPE_GESTURE_LEFT_INCOMPLETE': 'keydown super keydown Right keyup Right keyup super',
    b'MOUSE_GESTURE_UP_INCOMPLETE': 'keydown super keydown Down keyup Down keyup super',
    b'MOUSE_GESTURE_RIGHT_INCOMPLETE': 'keydown super keydown Left keyup Left keyup super',
    b'MOUSE_GESTURE_DOWN_INCOMPLETE': 'keydown super keydown Up keyup Up keyup super',
    b'MOUSE_GESTURE_LEFT_INCOMPLETE': 'keydown super keydown Right keyup Right keyup super',
    b'MOUSE_CUSTOM_BTN_1_PRESSED': 'keydown super keydown s keyup s keyup super',
    b'MOUSE_CUSTOM_BTN_1_RELEASED': None
}


class XDoTool:

    def __init__(self):
        self.subp = subprocess.Popen(['xdotool', '-'], stdin=subprocess.PIPE, bufsize=0)


    def press_key(self, key_name):
        self.subp.stdin.write(f'key {key_name}\n'.encode('ascii'))
        # self.subp.stdin.write(f'key --delay 20 {key_name}\n'.encode('ascii'))


    def move_mouse_relative(self, rel_x, rel_y):
        self.subp.stdin.write(f'mousemove_relative -- {rel_x} {rel_y}\n'.encode('ascii'))


    def click_mouse(self, button):

        btn = {
            'left': 1, 'middle': 2, 'right': 3, 'wheel-up': 4, 'wheel-down': 5
        }[button]
        self.subp.stdin.write(f'click {btn}\n'.encode('ascii'))


class EventReader:

    def __init__(self):
        self.subp = subprocess.Popen(['./event-reader'], stdout=subprocess.PIPE)


    def wait_event(self):

        while True:
            line = self.subp.stdout.readline()
            if not line:
                raise EOFError

            line_type, sep, event = line.partition(b':')

            if line_type == b'event':
                return event.strip()
            else:
                pass


if __name__ == '__main__':

    event_reader = EventReader()
    xdotool = XDoTool()

    while True:

        event = event_reader.wait_event()

        if event in EVENT_TO_KEY_MAP:
            command = EVENT_TO_KEY_MAP[event]
            if command is not None:
                xdotool.press_key(EVENT_TO_KEY_MAP[event])
        else:
            print('unhandled event', event)

