from android import Android
droid = Android()

import socket
import sys
import settings
# UDP_PORT = 5683
MESSAGE = '{"who":"*","F":"tester"}'
# MESSAGE = '{"task":"setcolor","F":"tester","T":"ESP-16575678","parameters":{"color":"'+sys.argv[1]+'","duration":5.10}}'
# MESSAGE = '{"task":"'+sys.argv[1]+'","F":"tester","T":"ESP-16575678","parameters":'+sys.argv[2]+'}'

# print "UDP target IP:", UDP_IP
# print "UDP target port:", UDP_PORT
# print "message:", MESSAGE
wifiInfo = droid.wifiGetConnectionInfo().result
if wifiInfo['ssid'] != settings.LOCAL_SSID:
    droid.makeToast("Not in home WiFi network")
    sys.exit()

droid.webViewShow('file:///sdcard/sl4a/scripts/lediot/colorpicker.html')

while True:
    event = droid.eventWait().result
    
    if event['name'] == 'toggle':
        MESSAGE = '{"task":"toggle","F":"py4a","T":"'+settings.ESP_NAME+'"}'
    elif event['name'] == 'color':
        MESSAGE = '{"task":"setcolor","F":"py4a","T":"'+settings.ESP_NAME+'","parameters":{"color":"'+event['data']+'","duration":0.00001} }'
    else:
        sys.exit()
        
    sock = socket.socket(socket.AF_INET, # Internet
                         socket.SOCK_DGRAM) # UDP
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)


    sock.sendto(MESSAGE, (settings.UDP_IP, settings.UDP_PORT))

