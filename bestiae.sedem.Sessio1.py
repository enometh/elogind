#!/usr/bin/env python
#
# Provide bestiae.sedem.Sessio1 on D-Bus.
#
import os,re,logging
from gi.repository import GLib
from pydbus import SessionBus, SystemBus

os.putenv("XDG_RUNTIME_DIR", "/dev/shm/madhu/")

if os.getenv("XDG_RUNTIME_DIR") == None:
		logfile = None
else:
		logfile = os.path.join(os.getenv("XDG_RUNTIME_DIR"), "dbuslog")

logging.basicConfig(filename=logfile,
					filemode='w',
					format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
					level=logging.DEBUG)

logging.info('logging initialized')

def get_dbus_address_from_file(path):
	with  open(path, "r") as file:
		for line in file:
			match = re.search('DBUS_SESSION_BUS_ADDRESS=[\'"](.*)[\'"]', line)
			if match:
				return match.group(1)

env_str = ''
dbus_addr = get_dbus_address_from_file("/home/madhu/g:0")

logging.info('DBUS_SESSION_BUS_ADDRESS to %s', dbus_addr)

if dbus_addr:
		env_str = format("%s DBUS_SESSION_BUS_ADDRESS='%s'" % (env_str, dbus_addr))
env_str = env_str + " HOME=/home/madhu"
env_str = env_str + " PATH=/usr/bin:/bin:/usr/local/bin"
env_str = env_str + " USER=madhu"
env_str = env_str + " SHELL=/bin/zsh"
env_str = env_str + " XDG_RUNTIME_DIR=/dev/shm/madhu"

# ;madhu 200703 dbus sets
#DBUS_STARTER_ADDRESS='unix:path=/run/dbus/system_bus_socket'
#DBUS_STARTER_BUS_TYPE=system
#
# and clients like telepathy-glib try to contact the system bus
# instead of the session bus and they fail.

def unsetenv(var):
		orig = os.getenv(var)
		if orig != None:
				logging.info("unset %s" % var)
				os.unsetenv(var)

unsetenv("DBUS_STARTER_ADDRESS")
unsetenv("DBUS_STARTER_BUS_TYPE")

start_cmd = env_str + " /home/madhu/misc/seat/seat0 -u madhu -s pameg -v 9 /7/gtk/emacs/build-xt/src/emacs -Q --fg-daemon=/tmp/seat0 |& tee -a /dev/shm/dbuslog &"
stop_cmd = "sudo -u madhu emacsclient -s /tmp/seat0 --eval '(kill-emacs)' |& tee -a /dev/shm/dbuslog"

# NOTES:
# attach to emacs with emacsclient -s /tmp/seat0 -t
# dbus-send --system --dest=bestiae.sedem.Sessio1 --print-reply /bestiae/sedem/Sessio1 bestiae.sedem.Sessio1.Start

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