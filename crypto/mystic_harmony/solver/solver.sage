import random
import output
import copy

human_world_size = 64
spirit_world_size_param = 32
disharmony_count = 16
t = spirit_world_size_param // 2
n = human_world_size + spirit_world_size_param

# ref: https://www.jstage.jst.go.jp/article/essfr/4/3/4_3_183/_pdf
R.<x, y> = PolynomialRing(GF(2))
# K.<a> = GF(2^8, name='x', modulus=x^8+x^4+x^3+x^2+1)
size = 2^8
K.<alpha> = GF(size, modulus=x^8+x^4+x^3+x^2+1)

def solve_sigma(S):
    def make_mat(S, k):
        mat = [[0 for i in range(k)] for i in range(k)]
        for i in range(k):
            for j in range(k):
                mat[i][j] = S[k-j-1+i]
        return matrix(mat)

    mat = None
    error_cnt = None
    for k in range(t):
        print("progress:", k)
        temp = make_mat(S, k+1)
        if temp.det() == 0:
            error_cnt = k
            break
        mat = temp
        error_cnt = k+1

    vec = []
    for i in range(error_cnt):
        vec.append(S[error_cnt + i])

    temp = mat.solve_right(vec)
    sigma = 1
    for i in range(len(temp)):
        sigma += temp[i] * x^(i+1)

    return sigma, error_cnt

def find_x(alpha_map):
    S = [alpha_map[0][i] for i in range(2*t)]
    sigma, error_cnt = solve_sigma(S)

    error_pos = []
    for i in range(n):
        if sigma(alpha^(size-i-1), 0) == 0:
            error_pos.append(i)

    mat = [[0 for i in range(error_cnt)] for i in range(error_cnt)]
    vec = [0 for i in range(error_cnt)]
    for i in range(error_cnt):
        for j in range(error_cnt):
            mat[i][j] = alpha^(error_pos[j] * (i+1))
        vec[i] = S[i]
    mat = matrix(mat)
    vec = vector(vec)

    temp = mat.solve_right(vec)
    print(temp)

    return error_pos, temp

def find_y(alpha_map):
    S = [alpha_map[i][0] for i in range(2*t)]
    sigma, _ = solve_sigma(S)
    error_pos = []
    for i in range(n):
        if sigma(alpha^(size-i-1), 0) == 0:
            error_pos.append(i)

    return error_pos

def find_xy(alpha_map):
    S = [alpha_map[i][i] for i in range(2*t)]

    sigma, _ = solve_sigma(S)
    error_pos = []
    for i in range(2*n):
        if sigma(alpha^(size-i-1), 0) == 0:
            error_pos.append(i)

    return error_pos

def witchmap_to_alphamap(map):
    res = []
    for i in range(len(map)):
        row = []
        for j in range(len(map[i])):
            if map[i][j] == None:
                row.append(0)
            else:
                row.append(alpha^map[i][j])
        res.append(row)
    return res

alpha_map = witchmap_to_alphamap(output.witch_map)
print(alpha_map)
x_pos, error_value = find_x(alpha_map)
y_pos = find_y(alpha_map)
xy_pos = find_xy(alpha_map)
print("x_pos=", x_pos)
print("y_pos=", y_pos)
print("xy_pos=", xy_pos)
x_pos_index = {}
for i in range(len(x_pos)):
    x_pos_index[x_pos[i]] = i

cand = []
xmemo = {}
for xp in x_pos:
    for yp in y_pos:
        if xp+yp in xy_pos:
            if xp not in xmemo:
                xmemo[xp] = []
            xmemo[xp].append(yp)

yprev = {}
xyprev = {}
for yp in y_pos:
    yprev[yp] = 0
for xyp in xy_pos:
    xyprev[xyp] = 0
ycnt = [copy.deepcopy(yprev)]
xycnt = [copy.deepcopy(xyprev)]

for xp in reversed(x_pos):
    ytemp = copy.deepcopy(yprev)
    xytemp =copy.deepcopy(xyprev) 
    for yp in xmemo[xp]:
        ytemp[yp] += 1
        xytemp[xp+yp] += 1
    ycnt.append(ytemp)
    xycnt.append(xytemp)
    yprev = copy.deepcopy(ytemp)
    xyprev = copy.deepcopy(xytemp)

ycnt = list(reversed(ycnt))
xycnt = list(reversed(xycnt))
for xy in xycnt:
    print(xy)

def dfs(x_pos, xmemo, index, error_value, expected_E, res, ycnt, xycnt, yused, xyused):
    if index == len(x_pos):
        res.append(expected_E)
        return
    xp = x_pos[index]

    cand = set()
    for yp in xmemo[xp]:
        # もう次以降でなくて、まだ一回も使われていないなら、それを使わなければならない
        if (ycnt[index+1][yp] == 0 and yused[yp] == 0) or (xycnt[index+1][xp+yp] == 0 and xyused[xp+yp] == 0):
            cand.add(yp)
    print("--------------------------------------------------")
    print(xp)
    print(yused)
    print(cand)
    if len(cand) >= 2:
        return
    if len(cand) == 0:
        cand = xmemo[xp]

    for yp in cand:
        temp = x^xp * y^yp * error_value[index] / alpha^yp
        expected_E += temp
        yused[yp] += 1
        xyused[xp+yp] += 1
        dfs(x_pos, xmemo, index+1, error_value, expected_E, res, ycnt, xycnt, yused, xyused)
        xyused[xp+yp] -= 1
        yused[yp] -= 1
        expected_E -= temp

res_cnt = 1
for _, value in xmemo.items():
    res_cnt *= len(value)
print("res_cnt:", res_cnt)
if res_cnt > 10000000:
    exit(1)

expected_E = 0
res = []
yused = {}
xyused = {}
for yp in y_pos:
    yused[yp] = 0
for xyp in xy_pos:
    xyused[xyp] = 0

dfs(x_pos, xmemo, 0, error_value, expected_E, res, ycnt, xycnt, yused, xyused)
print("res cnt", len(res))

import Crypto.Cipher.AES as AES
from Crypto.Util.number import long_to_bytes
import hashlib

def make_key(D):
    key_seed = b""
    for pos, value in sorted(list(D.dict().items())):
        print(pos)
        x = pos[0]
        y = pos[1]
        power = discrete_log(value, alpha, size-1)
        key_seed += long_to_bytes(x) + long_to_bytes(y) + long_to_bytes(power)
    m = hashlib.sha256()
    m.update(key_seed)
    return m.digest()

cand = []
for i in range(len(res)):
    print(i, "/", len(res))
    r = res[i]

    key = make_key(r.numerator())
    cipher = AES.new( key, AES.MODE_ECB )
    flag = cipher.decrypt(output.treasure_box)
    cand.append(flag)
    if b"SECCON{" in flag:
        print(cand)
        print(flag)
        break
print(cand)