import socket
import time
import sys


PORT = 51801
OUTPUT = "./records/" + sys.argv[1] + ".bin"

print("Waiting 10 seconds...")
time.sleep(10)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", PORT))

print("Waiting for one packet...")

data, addr = sock.recvfrom(65535)

with open(OUTPUT, "wb") as f:
	f.write(data)

print(f"Saved {len(data)} bytes to {OUTPUT}")

sock.close()
