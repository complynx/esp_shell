import socket
import sys

# UDP_IP = "192.168.0.112"
UDP_IP = "238.2.6.6"
UDP_PORT = 8266
# UDP_PORT = 5683
# MESSAGE = '{"who":"*","F":"tester"}'
# MESSAGE = '{"task":"setcolor","F":"tester","T":"ESP-16575678","parameters":{"color":"'+sys.argv[1]+'","duration":5.10}}'
MESSAGE = '{"task":"'+sys.argv[1]+'","F":"tester","T":"ESP-16575678","parameters":'+sys.argv[2]+'}'

print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
print "message:", MESSAGE
 
sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)


sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
