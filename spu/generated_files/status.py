# Python code to update the LCD with SPU info
# Edu Meneses, 2021, IDMIL, CIRMMT, McGill University

# Import needed modules from osc4py3
from osc4py3.as_eventloop import *
from osc4py3 import oscbuildparse
import subprocess
import threading
import sys

# Start the system.
osc_startup()

# Make client channels to send packets.
osc_udp_client("0.0.0.0", 20000, "lcd")

def updateStatus():
    # services returns 0 when active
    WiFiStatus = subprocess.call(['systemctl','is-active','--quiet','hostapd.service'])
    SCStatus = subprocess.call(['systemctl','--user','is-active','--quiet','supercollider.service'])
    PDStatus = subprocess.call(['systemctl','--user','is-active','--quiet','puredata.service'])
    pedalStatus = subprocess.getoutput("python3 /usr/modep/scripts/modep-ctrl.py current")
    if pedalStatus == '':
        pedalStatus = 'UNTITLED  '
    else:
        pedalStatus = pedalStatus.split('/var/modep/pedalboards/')[1].lstrip().split('.pedalboard')[0]
        pedalStatus = "{:<10}".format(pedalStatus[:10])
    if SCStatus == 0:
        msgSC = oscbuildparse.OSCMessage("/lcd", ",sii", ["SC|", 4, 1])
    else:
        msgSC = oscbuildparse.OSCMessage("/lcd", ",sii", ["  |", 4, 1])
    if PDStatus == 0:
        msgPD = oscbuildparse.OSCMessage("/lcd", ",sii", ["PD|", 4, 4])
    else:
        msgPD = oscbuildparse.OSCMessage("/lcd", ",sii", ["  |", 4, 4])
    if WiFiStatus == 0:
        msgWifi = oscbuildparse.OSCMessage("/lcd", ",sii", [" AP|", 4, 7])
    else:
        msgWifi = oscbuildparse.OSCMessage("/lcd", ",sii", ["STA|", 4, 7])
    msgPedal = oscbuildparse.OSCMessage("/lcd", ",sii", [pedalStatus, 4, 11])
    bun = oscbuildparse.OSCBundle(oscbuildparse.OSC_IMMEDIATELY,[msgSC, msgPD, msgWifi, msgPedal])
    osc_send(bun, "lcd")
    threading.Timer(5, updateStatus).start() # scheduling event every 5 seconds

updateStatus()

def main():
    while True:
        osc_process()

if __name__ == '__main__':

    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        osc_terminate()
        sys.exit(0)