import socket
import time

fileName = "flash.bin"

host = '192.168.1.10'
port = 7
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(10)
print('init socket')

s.connect((host, port))

print('Connected')

file = open(fileName, "rb")

NUM_PAGES = 2

size = 256*NUM_PAGES

byte = file.read(size)

page_count = 0

while(byte):

    if (page_count > 0):
        byte = file.read(size)

    command = b'FLASH_WRITE;'
    page = str(page_count).encode('ascii')
    end_separate = b';'

    command = command + page + end_separate + byte

    s.sendall(command)

    data = s.recv(3)
    page_count = page_count + 1

    print(str(data) + " " + str(page_count))
        
s.close()

    
print("closed")
