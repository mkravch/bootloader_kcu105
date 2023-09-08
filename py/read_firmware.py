import socket
import time

fileName = 'read_flash.hex'

num_pages = 100

host = '192.168.1.10'
port = 7
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(10)
print('init socket')

s.connect((host, port))

print('Connected')

open(fileName, 'w').close()

for i in range(0,num_pages):

    command = b'FLASH_READ;'
    page = str(i).encode('ascii')
    end_separate = b';'

    command = command + page + end_separate

    print(command)

    s.sendall(command)

    data = s.recv(256)
    print(data)        
    #data = s.recv(1024)

    with open(fileName, 'ba+') as f:
        f.write(data)
        
s.close()

    
print("closed")
