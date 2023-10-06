import os

flag = b"SECCON{jump_table_everywhere}"
key = os.urandom(len(flag))

print(len(flag))

o = ''
k = ''
for i, c in enumerate(flag):
    o += f"\\x{c ^ i ^ 0x55 ^ key[i]:02x}"
    k += f"\\x{key[i]:02x}"

print(o)
print(k)

