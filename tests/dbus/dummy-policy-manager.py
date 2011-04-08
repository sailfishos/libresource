#!/usr/bin/python

#   for scratchbox use: #!/usr/bin/python2.5

import gobject

import dbus
import dbus.service
import dbus.mainloop.glib

import sys

# for the pseudo interface
import os
import signal

# for the event scheduler
import threading
import time
import sched

# Resource policy manager D-BUS parameters
POLICY_DBUS_SERVICE   = "org.maemo.resource.manager"
POLICY_DBUS_PATH      = "/org/maemo/resource/manager"
POLICY_DBUS_INTERFACE = "org.maemo.resource.manager"

CLIENT_DBUS_INTERFACE = "org.maemo.resource.client"

REGISTER   = "register"
UNREGISTER = "unregister"
UPDATE     = "update"
ACQUIRE    = "acquire"
RELEASE    = "release"
GRANT      = "grant"
ADVICE     = "advice"
AUDIO      = "audio"
VIDEO      = "video"

mainloop = None

class EventScheduler(threading.Thread):

    def __init__(self, **kwds):
        threading.Thread.__init__(self, **kwds)
        self.setDaemon(True)

        # scheduler for timed signals
        self.scheduler = sched.scheduler(time.time, time.sleep)
        self.condition = threading.Condition()

    def run(self):
        while True:
            # note that this is should be actually thread-safe
            if self.scheduler.empty():
                # there are no events to schedule, let's wait
                self.condition.acquire()
                # print "started waiting"
                self.condition.wait()
                # Note: after wait, this acquires the lock again. That's
                # why we need to release, so that we can add events
                self.condition.release()
                # print "stopped waiting"

            # print "started running events"
            self.scheduler.run()
            # print "stopped running events"

    def add_event(self, delay, function, *args):
        # print "> add_event"
        self.scheduler.enter(delay, 1, function, args)
        # tell the runner to stop waiting
        self.condition.acquire()
        self.condition.notifyAll()
        self.condition.release()
        # print "< add_event"


class DummyPolicyDaemon(dbus.service.Object):
    def __init__(self):
        self.bus = dbus.SystemBus()
        self.name = dbus.service.BusName(POLICY_DBUS_SERVICE, self.bus)
        print "registered name", self.name
        dbus.service.Object.__init__(self, self.name, POLICY_DBUS_PATH)
        self.clients = []

    """
    method call sender=:1.132 -> dest=org.maemo.resource.manager serial=3 path=/org/maemo/resource/manager; interface=org.maemo.resource.manager; member=register
       int32 0
       uint32 1
       uint32 1
       uint32 1
       uint32 0
       uint32 0
       uint32 0
       string "player"
       uint32 0
    """

    """
    @dbus.service.method(POLICY_DBUS_INTERFACE)
    def register(self, type, id, reqno, mandatory, optional, share, mask, klass,
            mode, in_signature="iuuuuuusu", out_signature="", sender_keyword="sender"):
        print "REGISTER", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 
        print "mandatory:", str(mandatory) 
        print "optional: ", str(optional) 
        print "share:    ", str(share) 
        print "mask:     ", str(mask) 
        print "class:    ", klass 
        print "mode:     ", str(mode) 
        return None
   """
    
    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="i", out_signature="", sender_keyword="sender")
    def register(self, type, sender):
        print "REGISTER", sender
        print "type:     ", str(type) 
        return None

    @dbus.service.method(POLICY_DBUS_INTERFACE)
    def update(self, type, id, reqno, mandatory, optional, share, mask, klass, mode,
            in_signature="iuuuuuusu", out_signature="", sender_keyword="sender"):
        print "UPDATE", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 
        print "mandatory:", str(mandatory) 
        print "optional: ", str(optional) 
        print "share:    ", str(share) 
        print "mask:     ", str(mask) 
        print "class:    ", klass 
        print "mode:     ", str(mode) 
        return None

    """
    method call sender=:1.132 -> dest=org.maemo.resource.manager serial=4 path=/org/maemo/resource/manager; interface=org.maemo.resource.manager; member=acquire
       int32 3
       uint32 1
       uint32 2
    """

    @dbus.service.method(POLICY_DBUS_INTERFACE)
    def acquire(self, type, id, reqno, in_signature="iuu", out_signature="",
            sender_keyword="sender"):
        print "ACQUIRE", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 
        return None

    @dbus.service.method(POLICY_DBUS_INTERFACE)
    def release(self, type, id, reqno, in_signature="iuu", out_signature="",
            sender_keyword="sender"):
        print "RELEASE", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 
        return None

    @dbus.service.method(POLICY_DBUS_INTERFACE)
    def unregister(self, type, id, reqno, in_signature="iuu", out_signature="", sender_keyword="sender"):
        print "UNREGISTER", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 
        return None

    @dbus.service.method(POLICY_DBUS_INTERFACE)
    def audio(self, type, id, reqno, group, pid, streamName, method, pattern, in_signature="iuususis",
            out_signature="", sender_keyword="sender"):
        print "AUDIO", sender
        print "type:       ", str(type) 
        print "id:         ", str(id) 
        print "reqno:      ", str(reqno) 
        print "pid:        ", str(pid) 
        print "stream name:", streamName
        print "method:     ", str(method) 
        print "pattern:    ", pattern
        return None

    @dbus.service.method(POLICY_DBUS_INTERFACE)
    def video(self, type, id, reqno, pid, in_signature="iuuu", out_signature="",
            sender_keyword="sender"):
        print "VIDEO", sender
        print "type:       ", str(type) 
        print "id:         ", str(id) 
        print "reqno:      ", str(reqno) 
        print "pid:        ", str(pid) 
        return None

    """
    method call sender=:1.5 -> dest=:1.132 serial=96 path=/org/maemo/resource/client1; interface=org.maemo.resource.client; member=grant
       int32 5
       uint32 1
       uint32 2
       uint32 1
    """

    """
    method call sender=:1.5 -> dest=:1.132 serial=94 path=/org/maemo/resource/client1; interface=org.maemo.resource.client; member=advice
       int32 6
       uint32 1
       uint32 0
       uint32 1
    """

def main(argv=None):
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    mainloop = gobject.MainLoop()
    gobject.threads_init()
    call_daemon = DummyPolicyDaemon()
    mainloop.run()

if __name__ == "__main__":
    sys.exit(main())

