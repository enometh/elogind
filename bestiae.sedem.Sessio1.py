#!/usr/bin/env python
#
# Provide bestiae.sedem.Sessio1 on D-Bus.
#
import os
from gi.repository import GLib
from pydbus import SessionBus, SystemBus

start_cmd = "HOME=/home/madhu USER=madhu /home/madhu/misc/seat/seat0 -u madhu -s pameg -v 9 emacs -Q --fg-daemon=/tmp/seat0 |& tee /dev/shm/dbuslog &"
stop_cmd = "sudo -u madhu emacsclient -s /tmp/seat0 --eval '(kill-emacs)'"
# attach to emacs with emacsclient -s /tmp/seat0 -t

class Sessio1(object):
	"""
	<node>
		<interface name="bestiae.sedem.Sessio1">
			<method name="Start">
				<arg direction="out" type="u"/>
			</method>
			<method name='Quit'/>
			<method name="Stop">
				<arg direction="out" type="u"/>
			</method>
		</interface>
	</node>
	"""

	def Start(self):
		return os.system(start_cmd)

	def Stop(self):
		return os.system(stop_cmd)

	def Quit(self):
		loop.quit()

if os.geteuid() == 0:
	bus = SystemBus()
else:
	bus = SessionBus()

bus.publish("bestiae.sedem.Sessio1", Sessio1())
loop = GLib.MainLoop()
loop.run()

# Local Variables:
# tab-width: 4
# End:
#