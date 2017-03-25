from android import Android
droid = Android()

import socket
import sys
import settings

wifiInfo = droid.wifiGetConnectionInfo().result
if wifiInfo['ssid'] != settings.LOCAL_SSID:
    droid.makeToast("Not in home WiFi network")
    sys.exit()


MESSAGE = '{"task":"toggle","F":"py4a","T":"'+settings.ESP_NAME+'"}'
        
droid.log('message '+MESSAGE)
sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)


sock.sendto(MESSAGE, (settings.UDP_IP, settings.UDP_PORT))
