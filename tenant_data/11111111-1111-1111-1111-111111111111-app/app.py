import furnace_frontend as fe
import time

class App(object):
    def __init__(self, ctx):
        fe.log('in blank constructor')
        fe.event_register({'event_type': fe.TIMER, 'time_value': 1.0, 'callback': self.timer_callback})

    def timer_callback(self, ctx):
        fe.log('going to sleep for 10 seconds!')
        time.sleep(10)
        fe.log('in callback, exiting!')
        fe.exit()
