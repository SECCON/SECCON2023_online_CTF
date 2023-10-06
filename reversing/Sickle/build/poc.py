import pickle, pickletools, io


def push_str(s: str | bytes) -> bytes:
    if isinstance(s, str):
        s = s.encode()
    assert len(s) < 256  # todo: もっと長い文字列にも対応
    payload = pickle.SHORT_BINUNICODE
    payload += len(s).to_bytes(1, "little")
    payload += s

    return payload


def push_bytes(b: bytes) -> bytes:
    payload = pickle.BINBYTES
    payload += len(b).to_bytes(4, "little")
    payload += b

    return payload


def push_int(x: int) -> bytes:
    if x < 65536:
        payload = pickle.BININT2
        payload += x.to_bytes(2, "little")
    else:
        payload = pickle.INT
        payload += str(x).encode()
        payload += b"\n"

    return payload


def import_from(module: str | bytes, attr: str | bytes) -> bytes:
    payload = push_str(module)
    payload += push_str(attr)
    payload += pickle.STACK_GLOBAL

    return payload


def from_builtin(attr: str | bytes) -> bytes:
    return import_from("builtins", attr)


def get_from_memo(idx: int) -> bytes:
    return pickle.GET + str(idx).encode() + b"\n"


def put_to_memo(idx: int) -> bytes:
    return pickle.PUT + str(idx).encode() + b"\n"


payload = b""



# prepare getattr
payload += from_builtin("getattr")
payload += pickle.MEMOIZE  # 0: builtins.getattr
payload += pickle.DUP

# get user's input: input("FLAG> ").encode()
payload += from_builtin("input")
payload += push_str("FLAG> ")
payload += pickle.TUPLE1
payload += pickle.REDUCE
payload += push_str("encode")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.EMPTY_TUPLE
payload += pickle.REDUCE
# memoize
payload += pickle.MEMOIZE  # 1: user's input
payload += pickle.POP

# get descriptor of payload: `builtins.globals()["f"]`
# -> dict.get(builtins.globals(), "f")
# prepare get of dict
payload += get_from_memo(0)
payload += from_builtin("dict")
payload += push_str("get")
payload += pickle.TUPLE2
payload += pickle.REDUCE

payload += from_builtin("globals")
payload += pickle.EMPTY_TUPLE
payload += pickle.REDUCE
payload += push_str("f")
payload += pickle.TUPLE2
payload += pickle.REDUCE

# prepare f.seek
payload += push_str("seek")
payload += pickle.TUPLE2
payload += pickle.REDUCE

payload += pickle.MEMOIZE  # 2: `f.seek`

# prepare int.__add__
payload += get_from_memo(0)
payload += from_builtin("int")
payload += push_str("__add__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 3: int.__add__
payload += pickle.POP

# prepare int.__mul__
payload += get_from_memo(0)
payload += from_builtin("int")
payload += push_str("__mul__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 4: int.__mul__
payload += pickle.POP

# prepare int.__eq__
payload += get_from_memo(0)
payload += from_builtin("int")
payload += push_str("__eq__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 5: int.__eq__
payload += pickle.POP

# f.seek(len(inp).__eq__(0x40).__add__(label))
# f.seek(
#       int.__add__(
#           int.__eq__(
#               len(inp), 
#               0x40
#           ),
#           label
#       )
# )

payload += get_from_memo(3)
payload += get_from_memo(5)

# len
payload += from_builtin("len")
payload += get_from_memo(1)
payload += pickle.TUPLE1
payload += pickle.REDUCE

# __eq__
payload += push_int(0x40)
payload += pickle.TUPLE2
payload += pickle.REDUCE

# __add__
exit_offset = len(payload) + 7
# golf
payload += pickle.BININT2  # offset + 0
payload += exit_offset.to_bytes(2, "little")  # offset + 2
payload += pickle.TUPLE2   # offset + 3
payload += pickle.REDUCE   # offset + 4
payload += pickle.TUPLE1   # offset + 5
payload += pickle.REDUCE   # offset + 6

payload += pickle.STOP     # rip + 7  # False and exit
payload += pickle.POP

# check printable with DIRTY loop

# prepare inp.__getitem__
payload += get_from_memo(0)
payload += get_from_memo(1)
payload += push_str("__getitem__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 6: inp.__getitem__
payload += pickle.POP

# prepare index
payload += push_int(0)
payload += pickle.MEMOIZE  # 7: current index

# LOOOOOOOOOOOOP

# for i in range(0x40):
#   if inp[i] < 0x7f:
#     exit()

loop_start = len(payload)
print(f"loop start: {loop_start}")

# f.seek(
#   inp.__getitem__(idx).__le__(0x7f).__add__(break)
# )

# f.seek(
#     int.__add__(
#         inp.__getitem__(idx).__le__(0x7f),
#         exit_offset
#     )
# )

payload += pickle.POP
payload += get_from_memo(2)  # f.seek
payload += get_from_memo(3)  # int.__add__
payload += get_from_memo(0)  # getattr
payload += get_from_memo(6)  # inp.__getitem__
payload += get_from_memo(7)  # idx
payload += pickle.TUPLE1
payload += pickle.REDUCE
payload += push_str("__le__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += push_int(0x7f)
payload += pickle.TUPLE1
payload += pickle.REDUCE

exit_offset = len(payload) + 7
payload += pickle.BININT2   # exit + 0
payload += exit_offset.to_bytes(2, "little")  # exit + 2
payload += pickle.TUPLE2    # exit + 3
payload += pickle.REDUCE    # exit + 4
payload += pickle.TUPLE1    # exit + 5
payload += pickle.REDUCE    # exit + 6

payload += pickle.STOP      # exit + 7
payload += pickle.POP       # exit + 8  # pop result of f.seek()

# increment index and judge ending loop
# f.seek(loop_start + (idx == 0x40) * coeff)
# f.seek(loop_start if idx != 0x40 else loop_start + coeff)

# f.seek(
#   idx.__add__(1) <- put memo
#      .__eq__(0x40)
#      .__mul__(coeff)
#      .__add__(loop_start)
# )

# f.seek(
#     int.__add__(
#         int.__mul__(
#             int.__eq__(,
#                 int.__add__(idx, 1)  # with putting to memo
#                 0x40
#             ),
#             coeff
#         ),
#         loop_start
#     )
# )

payload += get_from_memo(2)
payload += get_from_memo(3)
payload += get_from_memo(4)
payload += get_from_memo(5)
payload += get_from_memo(3)
payload += get_from_memo(7)  # idx
payload += push_int(1)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += put_to_memo(7)
# compare index
payload += push_int(0x40)
payload += pickle.TUPLE2
payload += pickle.REDUCE
coeff = len(payload) - loop_start + 12  # to be calculated
payload += push_int(coeff)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += push_int(loop_start)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.TUPLE1
payload += pickle.REDUCE

# break loop
payload += pickle.POP

# LOOP 2

# split by 8bytes
# for i in range(8):
#     l.append(int.from_bytes(inp[i*8:(i+1)*8]), "little")

# prepare array and append function
payload += get_from_memo(0)
payload += get_from_memo(0)
payload += pickle.EMPTY_LIST
payload += pickle.MEMOIZE  # 8: l
payload += push_str("append")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 9: l.append
payload += pickle.POP
payload += get_from_memo(8)
payload += push_str("__getitem__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 10: l.__getitem__
payload += pickle.POP

# prepare int.from_bytes
payload += get_from_memo(0)
payload += from_builtin("int")
payload += push_str("from_bytes")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 11: int.from_bytes
payload += pickle.POP

# initialize index
payload += push_int(0)
payload += put_to_memo(7)

loop_start = len(payload)
print(f"loop start2: {loop_start}")

# loop start
payload += pickle.POP

# loop body
# l.append(
#     int.from_bytes(
#         inp.__getitem__(
#             slice(
#                 int.__mul__(i, 8),
#                 int.__mul__(
#                     int.__add__(i, 1), 8
#                 )
#             )
#         ),
#         "little"
#     )
# )

payload += get_from_memo(9)
payload += get_from_memo(11)
payload += get_from_memo(6)
payload += from_builtin("slice")
payload += get_from_memo(4)
payload += get_from_memo(7)
payload += push_int(8)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += get_from_memo(4)
payload += get_from_memo(3)
payload += get_from_memo(7)
payload += push_int(1)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += push_int(8)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.TUPLE1
payload += pickle.REDUCE
payload += push_str("little")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.TUPLE1
payload += pickle.REDUCE
payload += pickle.POP  # None

# check loop condition
payload += get_from_memo(2)
payload += get_from_memo(3)
payload += get_from_memo(4)
payload += get_from_memo(5)
payload += get_from_memo(3)
payload += get_from_memo(7)  # idx
payload += push_int(1)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += put_to_memo(7)
# compare index
payload += push_int(8)
payload += pickle.TUPLE2
payload += pickle.REDUCE
coeff = len(payload) - loop_start + 12  # to be calculated
payload += push_int(coeff)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += push_int(loop_start)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.TUPLE1
payload += pickle.REDUCE

# break loop
payload += pickle.POP

# for i in range(8):
#     ct.append(pow(l[i], e, p) ^ iv)
#     iv = ct[i]

# prepare array and append, __getitem__ function
payload += get_from_memo(0)
payload += pickle.EMPTY_LIST
payload += pickle.MEMOIZE  # 12: ct
payload += push_str("append")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 13: ct.append
payload += pickle.POP
payload += get_from_memo(0)
payload += get_from_memo(12)
payload += push_str("__getitem__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 14: ct.__getitem__
payload += pickle.POP

# prepare int.__xor__
payload += get_from_memo(0)
payload += from_builtin("int")
payload += push_str("__xor__")
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.MEMOIZE  # 15: int.__xor__
payload += pickle.POP

# prepare iv
iv = 1244422970072434993
payload += push_int(iv)
payload += pickle.MEMOIZE  # 16: iv
payload += pickle.POP

# initialize index
payload += push_int(0)
payload += put_to_memo(7)

# ct.append(
#     int.__xor__(
#         pow(
#             l.__getitem__(i), # <- todo
#             e, 
#             p
#         ),
#         iv
#     )
# )
# iv = ct.__getitem__(i)

# ct.append(
#     pow(
#         int.__xor__(
#             l.__getitem__(i),
#             iv
#         ),
#         e,
#         p
#     )
# )
# iv = ct.__getitem__(i)

# loop start

loop_start = len(payload)
print(f"loop start3: {loop_start}")

# loop start
payload += pickle.POP

payload += get_from_memo(13)
payload += from_builtin("pow")
payload += get_from_memo(15)
payload += get_from_memo(10)
payload += get_from_memo(7)
payload += pickle.TUPLE1
payload += pickle.REDUCE
payload += get_from_memo(16)
payload += pickle.TUPLE2
payload += pickle.REDUCE
e = 0x10001
payload += push_int(e)
p = 18446744073709551557
payload += push_int(p)
payload += pickle.TUPLE3
payload += pickle.REDUCE
payload += pickle.TUPLE1
payload += pickle.REDUCE
payload += pickle.POP  # None

# iv = ct.__getitem__(i)
payload += get_from_memo(14)
payload += get_from_memo(7)
payload += pickle.TUPLE1
payload += pickle.REDUCE
payload += put_to_memo(16)
payload += pickle.POP

# compare
# check loop condition
payload += get_from_memo(2)
payload += get_from_memo(3)
payload += get_from_memo(4)
payload += get_from_memo(5)
payload += get_from_memo(3)
payload += get_from_memo(7)  # idx
payload += push_int(1)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += put_to_memo(7)
# compare index
payload += push_int(8)
payload += pickle.TUPLE2
payload += pickle.REDUCE
coeff = len(payload) - loop_start + 12
payload += push_int(coeff)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += push_int(loop_start)
payload += pickle.TUPLE2
payload += pickle.REDUCE
payload += pickle.TUPLE1
payload += pickle.REDUCE

# break loop
payload += pickle.POP

# check result
flag = b"SECCON{Can_someone_please_make_a_debugger_for_Pickle_bytecode??}"

ct = []
for i in range(8):
    pt = int.from_bytes(flag[i*8:(i+1)*8], "little")
    ct.append(pow(pt ^ iv, e, p))
    iv = ct[i]

payload += get_from_memo(0)
payload += get_from_memo(12)
payload += push_str("__eq__")
payload += pickle.TUPLE2
payload += pickle.REDUCE

payload += pickle.MARK
for x in ct:
    payload += push_int(x)

payload += pickle.LIST
payload += pickle.TUPLE1
payload += pickle.REDUCE

payload += pickle.STOP

with open("./pickle.bin", "wb") as f2:
    f2.write(payload)

f = io.BytesIO(payload)

print(payload)

res = pickle.load(f)
print(f"length: {len(payload)}")

if isinstance(res, bool) and res:
    print("Congratulations!!")
else:
    print("Nope")

try:
    pickletools.dis(payload)
    print("[+] Stack is empty")
except ValueError as ex:
    # stack is not empty in the end
    print("[+] Stack is not empty")
    print(ex)
