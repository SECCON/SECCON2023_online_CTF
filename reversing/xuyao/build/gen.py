import struct

flag = b"Congratulations! You have decrypted the flag: SECCON{x86_he2_zhuan1_you3_zi4_jie2_ma3_de_hun4he2}\n"

sbox = [0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
        0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
        0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
        0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
        0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
        0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
        0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
        0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
        0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
        0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
        0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
        0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
        0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
        0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
        0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
        0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
        0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16]
fk = [0xff324600, 0x4f9a25b8, 0x3cc7477c, 0x0c0b9ecd]
ck = [0xec656287, 0xd9a22031, 0x01c7bca8, 0xabe7033b,
      0x313fe5dc, 0x940ffad0, 0x176edeb8, 0x7c61b20e,
      0x9ead452f, 0x80e2c15b, 0xba500d7b, 0xa2c0449f,
      0xbc0e774f, 0x3e393763, 0x43d46b3f, 0x2adef404,
      0xca884b87, 0x3c953c45, 0x7cdbde63, 0x6e995945,
      0xb6cf3655, 0x8d60396a, 0x9a496b38, 0x9d87d81b,
      0x36fedbc9, 0x79882953, 0x10611e15, 0x0030ab3e,
      0x12503487, 0x187e21ff, 0x6d85127e, 0xdf42c76c]

p32 = lambda x: struct.pack("<I", x)
u32 = lambda x: struct.unpack("<I", x)[0]
rol32 = lambda x, y: ((x << y) | (x >> (32 - y))) & 0xffffffff

def kls(x):
    return x ^ rol32(x, 11) ^ rol32(x, 25)
def els(x):
    return x ^ rol32(x, 3) ^ rol32(x, 14) ^ rol32(x, 15) ^ rol32(x, 9)
def tnls(x):
    y = 0
    for i in range(4):
        y |= (sbox[(x >> (i * 8)) & 0xff]) << (i * 8)
    return y
def ks(x):
    return kls(tnls(x))
def es(x):
    return els(tnls(x))
def r(x, rk):
    return x[0] ^ es(x[1] ^ x[2] ^ x[3] ^ rk)

def expand_key(key):
    assert len(key) == 0x10
    k = [0] * 32
    rk = [0] * 4
    for i in range(4):
        rk[i] = u32(key[i*4:i*4+4][::-1]) ^ fk[i]
    for i in range(32):
        t = rk[0] ^ ks(rk[1] ^ rk[2] ^ rk[3] ^ ck[i])
        k[i] = t
        rk[0] = rk[1]
        rk[1] = rk[2]
        rk[2] = rk[3]
        rk[3] = t
    return k

def encrypt_block(p, key):
    c = b''
    rk = expand_key(key)
    x = [0] * 4
    for i in range(4):
        x[i] = u32(p[i][::-1])
    for i in range(32):
        t = r(x, rk[i])
        x[0] = x[1]
        x[1] = x[2]
        x[2] = x[3]
        x[3] = t
    for i in range(4):
        cb = p32(x[3-i])
        c += cb[::-1]
    return c

def encrypt(plain, key):
    padlen = 0x10 - (len(plain) % 0x10)
    plain += bytes([padlen] * padlen)
    print(hex(len(plain)))
    cipher = b''
    for i in range(0, len(plain), 0x10):
        pt = [plain[i+j*4:i+(j+1)*4] for j in range(4)]
        ct = encrypt_block(pt, key)
        cipher += ct
    return cipher

key = b"SECCON CTF 2023!"
c = encrypt(flag, key)
print(''.join(map(lambda x: f"\\x{x:02x}", c)))