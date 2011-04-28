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
        self.event_scheduler = EventScheduler()
        self.event_scheduler.start()
        self.granted = True
        self.clients = {}


    def error(self, foo):
        pass # all status replies appear to time out


    def reply(self, *args):
        print "Reply"


    def path(self, id):
        return "/org/maemo/resource/client" + str(id)


    def advice(self, sender, id, reqno, resources):
        type = 6

        print ""
        print "ADVICE"
        print "type:     ", str(type)
        print "id:       ", str(id)
        print "reqno:    ", str(reqno)
        print "resources:", str(resources)

        path = self.path(id)

        if self.clients.has_key(sender):
            if self.clients[sender].has_key(path):
                resource = self.clients[sender][self.path(id)]
                resource["dbus"].advice(dbus.Int32(type), dbus.UInt32(id), dbus.UInt32(reqno), dbus.UInt32(resources),
                       reply_handler=self.reply, error_handler=self.error)


    def grant(self, sender, id, reqno, resources):
        type = 5

        print ""
        print "GRANT"
        print "type:     ", str(type)
        print "id:       ", str(id)
        print "reqno:    ", str(reqno)
        print "resources:", str(resources)

        path = self.path(id)

        if self.clients.has_key(sender):
            if self.clients[sender].has_key(path):
                resource = self.clients[sender][path]
                resource["dbus"].grant(dbus.Int32(type), dbus.UInt32(id),
                        dbus.UInt32(reqno), dbus.UInt32(resources),
                        reply_handler=self.reply, error_handler=self.error)


    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="iuuuuuusu", out_signature="iuuis",
            sender_keyword="sender")
    def register(self, type, id, reqno, mandatory, optional, share, mask, klass, mode, sender=None):

        errorCode = 0
        errorString = "OK"

        print ""
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

        client = {}

        if self.clients.has_key(sender):
            client = self.clients[sender]

        path = self.path(id)

        clientProxy = self.bus.get_object(sender, path, introspect=False)
        clientIf = dbus.Interface(clientProxy, CLIENT_DBUS_INTERFACE)

        resource = { "mandatory":mandatory, "optional":optional, "dbus":clientIf }

        client[path] = resource
        self.clients[sender] = client

        self.event_scheduler.add_event(1, self.advice, sender, id, reqno, resource["mandatory"])

        return [9, id, reqno, errorCode, errorString]


    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="iuuuuuusu", out_signature="iuuis",
            sender_keyword="sender")
    def update(self, type, id, reqno, mandatory, optional, share, mask, klass, mode, sender=None):

        errorCode = 0
        errorString = "OK"

        print ""
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

        return [9, id, reqno, errorCode, errorString]


    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="iuu", out_signature="iuuis",
            sender_keyword="sender")
    def acquire(self, type, id, reqno, sender=None):

        errorCode = 0
        errorString = "OK"

        print ""
        print "ACQUIRE", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 

        # find the client
        resource = self.clients[sender][self.path(id)]

        self.event_scheduler.add_event(1, self.grant, sender, id, reqno, resource["mandatory"])

        return [9, id, reqno, errorCode, errorString]


    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="iuu", out_signature="iuuis",
            sender_keyword="sender")
    def release(self, type, id, reqno, sender=None):

        errorCode = 0
        errorString = "OK"

        print ""
        print "RELEASE", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 

        self.event_scheduler.add_event(1, self.grant, sender, id, reqno, 0)

        return [9, id, reqno, errorCode, errorString]


    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="iuu", out_signature="iuuis",
            sender_keyword="sender")
    def unregister(self, type, id, reqno, sender=None):

        errorCode = 0
        errorString = "OK"

        print ""
        print "UNREGISTER", sender
        print "type:     ", str(type) 
        print "id:       ", str(id) 
        print "reqno:    ", str(reqno) 

        client = self.clients[sender]
        path = self.path(id)
        del client[path]

        if len(client) == 0:
            del self.clients[sender]

        return [9, id, reqno, errorCode, errorString]


    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="iuususis",
            out_signature="iuuis", sender_keyword="sender")
    def audio(self, type, id, reqno, group, pid, streamName, method, pattern, sender=None):

        errorCode = 0
        errorString = "OK"

        print ""
        print "AUDIO", sender
        print "type:       ", str(type) 
        print "id:         ", str(id) 
        print "reqno:      ", str(reqno) 
        print "pid:        ", str(pid) 
        print "stream name:", streamName
        print "method:     ", str(method) 
        print "pattern:    ", pattern

        return [9, id, reqno, errorCode, errorString]


    @dbus.service.method(POLICY_DBUS_INTERFACE, in_signature="iuuu", out_signature="iuuis",
            sender_keyword="sender")
    def video(self, type, id, reqno, pid, sender=None):

        errorCode = 0
        errorString = "OK"

        print ""
        print "VIDEO", sender
        print "type:       ", str(type) 
        print "id:         ", str(id) 
        print "reqno:      ", str(reqno) 
        print "pid:        ", str(pid) 

        return [9, id, reqno, errorCode, errorString]


def main(argv=None):
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    mainloop = gobject.MainLoop()
    gobject.threads_init()
    call_daemon = DummyPolicyDaemon()
    mainloop.run()

if __name__ == "__main__":
    sys.exit(main())

