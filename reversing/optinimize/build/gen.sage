flag = b"SECCON{3b4297373223a58ccf3dc06a6102846f}"
P = Primes()
pseudo = [271441, 904631, 16532714, 24658561, 27422714, 27664033, 46672291, 102690901, 130944133, 196075949, 214038533, 517697641, 545670533, 801123451, 855073301, 903136901, 970355431, 1091327579, 1133818561, 1235188597, 1389675541, 1502682721, 2059739221, 2304156469, 2976407809, 3273820903]

ns = []
for i in range(10):
    ns.append(randrange(0x40 + 0x10*i, 0x40 + 0x10*(i+1)))
for i in range(10):
    ns.append(randrange(0x100 + 0x1000*i, 0x100 + 0x1000*(i+1)))
for i in range(10):
    ns.append(randrange(0x100000 + 0x100000*i, 0x100000 + 0x100000*(i+1)))
for i in range(10):
    ns.append(randrange(0x1000000 + 0x4ccccc*i, 0x1000000 + 0x4ccccc*(i+1)))
cs = []

for c, n in zip(flag, ns):
    p = P.unrank(n-2)
    delta = 0
    while pseudo[delta] < p:
        delta += 1
    print(delta)
    if delta:
        p = P.unrank(n-2-delta)
    cs.append(c ^^ (p % 0x100))

print(ns)
print(cs)
