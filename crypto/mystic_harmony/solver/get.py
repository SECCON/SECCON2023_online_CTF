import socket
import os

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((os.getenv("SECCON_HOST"), int(os.getenv("SECCON_PORT"))))
while True:
    response = client.recv(4096)
    print(response.decode('utf-8'), end='')
    if response == b"":
        break
