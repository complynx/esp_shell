import socket
import struct

import json

UDP_IP = "238.2.6.6"
UDP_PORT = 8266

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(('', UDP_PORT))  # use UDP_IP instead of '' to listen only
                             # to UDP_IP, not all groups on UDP_PORT
mreq = struct.pack("4sl", socket.inet_aton(UDP_IP), socket.INADDR_ANY)

sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)


while True:
	data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
	print "received message:", data, " from ", addr
	# d=json.loads(data)

	# sockreply = socket.socket(socket.AF_INET, # Internet
	#                  socket.SOCK_DGRAM) # UDP
	## sockreply.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	## sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	# sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
	# sockreply.sendto('{"sender":"tester","cmd":"I am","options":{"title":"Test suite."}}', (UDP_IP,UDP_PORT))

