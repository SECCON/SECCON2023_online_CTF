#include <stdio.h>
#include <stdlib.h>
#include "insn.h"

static const u8 enc[] = "\xfe\x60\xa8\xc0\x3b\xfe\xbc\x66\xfc\x9a\x9b\x31\x9a\xd8\x03\xbb\xa9\xe1\x56\xfc\xfc\x11\x9f\x89\x5f\x4d\x9f\xe0\x9f\xae\x2a\xcf\x5e\x73\xcb\xec\x3f\xff\xb9\xd1\x99\x44\x1b\x9a\x79\x79\xec\xd1\xb4\xfd\xea\x2b\xe2\xf1\x1a\x70\x76\x3c\x2e\x7f\x3f\x3b\x7b\x66\xa3\x4b\x1b\x5c\x0f\xbe\xdd\x98\x5a\x5b\xd0\x0a\x3d\x7e\x2c\x10\x56\x2a\x10\x87\x5d\xd9\xb9\x7f\x3e\x2e\x86\xb7\x17\x04\xdf\xb1\x27\xc4\x47\xe2\xd9\x7a\x9a\x48\x7c\xdb\xc6\x1d\x3c\x00\xa3\x21";
static const u8 sbox[256] = {
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
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
  0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const u32 fish[4] = {
  0xff324600, 0x4f9a25b8, 0x3cc7477c, 0x0c0b9ecd
};

static const u32 cat[0x20] = {
  0xec656287, 0xd9a22031, 0x01c7bca8, 0xabe7033b,
  0x313fe5dc, 0x940ffad0, 0x176edeb8, 0x7c61b20e,
  0x9ead452f, 0x80e2c15b, 0xba500d7b, 0xa2c0449f,
  0xbc0e774f, 0x3e393763, 0x43d46b3f, 0x2adef404,
  0xca884b87, 0x3c953c45, 0x7cdbde63, 0x6e995945,
  0xb6cf3655, 0x8d60396a, 0x9a496b38, 0x9d87d81b,
  0x36fedbc9, 0x79882953, 0x10611e15, 0x0030ab3e,
  0x12503487, 0x187e21ff, 0x6d85127e, 0xdf42c76c
};

void tnls(const defarg(x), defarg(y)) {
  func_begin();
  int j;
  def(b, sizeof(u8));
  def(t, sizeof(u32));

  op_movi(y, 0);
  for (int i = 0; i < 4; i++) {
    op_mov(t, x);
    op_shri(t, i*8);
    op_andi(t, 0xff);
    op_load(j, t);
    op_movi(t, sbox[j]);
    op_shli(t, i*8);
    op_or(y, t);
  }

  func_end();
}

void kls(const defarg(x), defarg(y)) {
  func_begin();
  def(t, sizeof(u32));

  op_mov(y, x);
  op_mov(t, x);
  op_rol32(t, 11);
  op_xor(y, t);
  op_mov(t, x);
  op_rol32(t, 25);
  op_xor(y, t);

  func_end();
}

void els(const defarg(x), defarg(y)) {
  func_begin();
  def(t, sizeof(u32));

  op_mov(y, x);
  op_mov(t, x);
  op_rol32(t, 3);
  op_xor(y, t);
  op_mov(t, x);
  op_rol32(t, 14);
  op_xor(y, t);
  op_mov(t, x);
  op_rol32(t, 15);
  op_xor(y, t);
  op_mov(t, x);
  op_rol32(t, 9);
  op_xor(y, t);

  func_end();
}

void ks(const defarg(x), defarg(y)) {
  func_begin();
  def(t, sizeof(u32));

  tnls(arg(x), arg(t));
  kls(arg(t), arg(y));

  func_end();
}

void es(const defarg(x), defarg(y)) {
  func_begin();
  def(t, sizeof(u32));

  tnls(arg(x), arg(t));
  els(arg(t), arg(y));

  func_end();
}

void r(const defarg(x[4]), const defarg(rk), defarg(y)) {
  func_begin();
  def(t, sizeof(u32));

  op_mov(t, x[1]);
  op_xor(t, x[2]);
  op_xor(t, x[3]);
  op_xor(t, rk);
  es(arg(t), arg(y));
  op_xor(y, x[0]);

  func_end();
}

void encrypt_block(const defarg(in[4]), const defarg(k[32]), defarg(out[4])) {
  func_begin();
  def(t, sizeof(u32));
  def_array(x, sizeof(u32), 4);

  for (int i = 0; i < 4; i++) {
    op_mov(x[i], in[i]);
    op_be32(x[i]);
  }

  for (int i = 0; i < 32; i++) {
    r(arg(x), arg(k[i]), arg(t));
    op_mov(x[0], x[1]);
    op_mov(x[1], x[2]);
    op_mov(x[2], x[3]);
    op_mov(x[3], t);
  }

  for (int i = 0; i < 4; i++) {
    op_mov(out[i], x[3 - i]);
    op_be32(out[i]);
  }

  func_end();
}

void encrypt(const defarg(flag[0x70]), const defarg(len), const char *key,
             defarg(enc_flag[0x70])) {
  func_begin();
  int i;
  def(temp, sizeof(u32));
  def(i, sizeof(u32));
  def(c, sizeof(u32));
  def_array(plaintext, sizeof(u32), 4);
  def_array(ciphertext, sizeof(u32), 4);
  def(t, sizeof(u32));
  def(subkey, sizeof(u32));
  def_array(rk, sizeof(u32), 4);
  def_array(expanded_key, sizeof(u32), 32);

  // Expand key
  for (int i = 0; i < 4; i++) {
    op_movi(t, *(u32*)(key + i*4));
    op_be32(t);
    op_xori(t, fish[i]);
    op_mov(rk[i], t);
  }
  for (int i = 0; i < 32; i++) {
    // t = rk[0] - key_sub(rk[1] ^ rk[2] ^ rk[3] ^ ck[i]);
    op_mov(t, rk[1]);
    op_xor(t, rk[2]);
    op_xor(t, rk[3]);
    op_xori(t, cat[i]);
    ks(arg(t), arg(subkey));
    op_mov(t, rk[0]);
    op_xor(t, subkey);
    op_mov(expanded_key[i], t);
    // Rotate round key
    op_mov(rk[0], rk[1]);
    op_mov(rk[1], rk[2]);
    op_mov(rk[2], rk[3]);
    op_mov(rk[3], t);
  }

  op_movi(i, 0);
  while (op_lt(i, len)) {
    // Setup block
    op_load(i, i);
    for (int j = 0; j < 4; j++) {
      op_movi(temp, 0);
      for (int k = 0; k < 4; k++) {
        op_movz(c, u32, flag[i+j*4+3-k], u8);
        op_shli(temp, 8);
        op_or(temp, c);
      }
      op_mov(plaintext[j], temp);
    }
    // Encrypt block
    encrypt_block(arg(plaintext), arg(expanded_key), arg(ciphertext));
    // Write cipher
    for (int j = 0; j < 4; j++) {
      op_mov(temp, ciphertext[j]);
      for (int k = 0; k < 4; k++) {
        op_mov(c, temp);
        op_andi(c, 0xff);
        op_movz(enc_flag[i+j*4+k], u8, c, u32);
        op_shri(temp, 8);
      }
    }

    op_addi(i, 0x10);
  }

  func_end();
}

int main() {
  func_begin();
  char flag[0x70];
  def_array(flag, sizeof(u8), 0x70);
  def_array(enc_flag, sizeof(u8), 0x70);
  def(len, sizeof(u32));
  def(padlen, sizeof(u32));
  def(temp, sizeof(u32));
  def(pad, sizeof(u8));

  // Input flag
  printf("Message: ");
  if (fgets(flag, 100, stdin) == NULL) exit(1);

  // Copy flag to VM world
  op_movi(len, 0);
  for (int i = 0; flag[i]; i++) {
    op_store(flag[i], flag[i]);
    op_addi(len, 1);
  }
  // Padding
  op_mov(padlen, len);
  op_addi(padlen, 0x10);
  op_andi(padlen, 0xfffffff0);
  op_mov(temp, padlen);
  op_sub(temp, len)
  op_movz(pad, u8, temp, u32);  
  while (op_lt(len, padlen)) {
    int i;
    op_load(i, len);
    op_mov(flag[i], pad);
    op_addi(len, 1);
  }

  encrypt(arg(flag), arg(padlen), "SECCON CTF 2023!", arg(enc_flag));

  for (int i = 0; i < 0x70; i++) {
    if (op_nei(enc_flag[i], enc[i]))
      goto fail;
  }

  puts("Correct! I think you got the flag now :)");
  goto out;

 fail:
  puts("Wrong...");

 out:
  func_end();
  return 0;
}
