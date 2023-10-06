import bigints
import options
import os

proc P(n: BigInt): BigInt =
  # Return n-th perrin number
  var a: BigInt = 3.initBigInt
  var b: BigInt = 0.initBigInt
  var c: BigInt = 2.initBigInt
  if n == 0.initBigInt:
    result = a
  elif n == 1.initBigInt:
    result = b
  elif n == 2.initBigInt:
    result = c
  elif n > 2.initBigInt:
    var i = n
    while i > 2.initBigInt:
      var t: BigInt = a + b
      a = b
      b = c
      c = t
      i = i - 1.initBigInt
    result = c
  else:
    raise newException(OSError, "Invalid argument")

proc Q(n: BigInt): BigInt =
  # Return n-th perrin number such that i divides P(i)
  var j = 0.initBigInt
  var i = 0.initBigInt
  while j < n:
    i = i + 1.initBigInt
    if P(i) mod i == 0.initBigInt:
      j = j + 1.initBigInt
  result = i

let ns = [74, 85, 111, 121, 128, 149, 174, 191, 199, 213, 774, 6856, 9402, 15616, 17153, 22054, 27353, 28931, 36891, 40451, 1990582, 2553700, 3194270, 4224632, 5969723, 7332785, 7925541, 8752735, 10012217, 11365110, 17301654, 26085581, 29057287, 32837617, 39609127, 44659126, 47613075, 56815808, 58232493, 63613165]
let cs = [60, 244, 26, 208, 138, 23, 124, 76, 223, 33, 223, 176, 18, 184, 78, 250, 217, 45, 102, 250, 212, 149, 240, 102, 109, 206, 105, 0, 125, 149, 234, 217, 10, 235, 39, 99, 117, 17, 55, 212]

for i, n in ns:
  let k = Q(n.initBigInt) mod 0x100.initBigInt
  stdout.write char(cs[i] xor toInt[int](k).get)
  stdout.flushFile()
