#!/usr/bin/env python
#
# Provide bestiae.sedem.Sessio1 on D-Bus.
#
import os
from gi.repository import GLib
from pydbus import SessionBus, SystemBus
import logging

# dbus spawns us with an empty PATH
os.putenv('PATH','/usr/bin:/bin:/usr/local/bin')
logging.basicConfig(filename='/dev/shm/dbuslog',
					filemode='w',
					format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
					level=logging.DEBUG)

logging.info('logging initialized')

start_cmd = "SHELL=/usr/bin/zsh HOME=/home/madhu USER=madhu /home/madhu/misc/seat/seat0 -u madhu -s pameg -v 9 /7/gtk/emacs/build-xt/src/emacs -Q --fg-daemon=/tmp/seat0 |& tee -a /dev/shm/dbuslog &"
stop_cmd = "sudo -u madhu emacsclient -s /tmp/seat0 --eval '(kill-emacs)' |& tee -a /dev/shm/dbuslog"
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
		logging.info('starting: %s', start_cmd)
		return os.system(start_cmd)

	def Stop(self):
		logging.info('stopping: %s', stop_cmd)
		return os.system(stop_cmd)

	def Quit(self):
		logging.info('quitting: %s', stop_cmd)
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