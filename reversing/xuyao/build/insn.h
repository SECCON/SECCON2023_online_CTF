#ifndef __HEADER_INSN__
#define __HEADER_INSN__
#include <sys/mman.h>
#include <stdlib.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
struct var_t {
  u8 *scope;
  u32 offset;
  u32 size;
};

/* Function */
#define MAX_VARSIZE 0x1000
#define func_begin()                            \
  u32 varpos = 0;                               \
  u8 *varbase = (u8*)mmap(                      \
    NULL, MAX_VARSIZE, PROT_READ | PROT_WRITE,  \
    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0          \
  );                                            \
  if (varbase == MAP_FAILED) exit(1);
#define func_end()                              \
  munmap(varbase, MAX_VARSIZE);

/* Local variable */
#define def(NAME, SIZE)                         \
  const struct var_t var_ ## NAME = {           \
    .scope = varbase,                           \
    .offset = varpos,                           \
    .size = ( SIZE )                            \
  };                                            \
  varpos += ( SIZE );

#define def_array(NAME, SIZE, LEN)              \
  struct var_t var_ ## NAME [( LEN )];          \
  for (int i = 0; i < ( LEN ); i++) {           \
    var_ ## NAME [i] . scope = varbase;         \
    var_ ## NAME [i] . offset = varpos;         \
    var_ ## NAME [i] . size = ( SIZE );         \
    varpos += ( SIZE );                         \
  }

/* Pass argument */
#define defarg(NAME) struct var_t var_ ## NAME
#define arg(NAME) var_ ## NAME

/* VM to real-world */
#define op_load(NAME1, NAME2)                                        \
  if (var_ ## NAME2 . size == 1)                                     \
    NAME1 = *(u8*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);  \
  else if (var_ ## NAME2 . size == 2)                                \
    NAME1 = *(u16*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset); \
  else if (var_ ## NAME2 . size == 4)                                \
    NAME1 = *(u32*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset); \
  else if (var_ ## NAME2 . size == 8)                                \
    NAME1 = *(u64*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset); \
  else                                                               \
    __builtin_trap();

/* Real-world to VM */
#define op_store(NAME1, NAME2)                                        \
  if (var_ ## NAME1 . size == 1)                                      \
    *(u8*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) = NAME2;   \
  else if (var_ ## NAME1 . size == 2)                                 \
    *(u16*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) = NAME2;  \
  else if (var_ ## NAME1 . size == 4)                                 \
    *(u32*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) = NAME2;  \
  else if (var_ ## NAME1 . size == 8)                                 \
    *(u64*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) = NAME2;  \
  else                                                                \
    __builtin_trap();

/* Copy variable with zero extend */
#define op_movz(NAME1, TYPE1, NAME2, TYPE2)                           \
  if (var_ ## NAME1 . size != sizeof(TYPE1)                           \
      || var_ ## NAME2 . size != sizeof(TYPE2))                       \
    __builtin_trap();                                                 \
  *(TYPE1*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) =         \
    (TYPE1)*(TYPE2*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);

/* Operation with VM variable */
#define op_generic_vm(NAME1, NAME2, OP)                         \
  if (var_ ## NAME1 . size != var_ ## NAME2 . size)             \
    __builtin_trap();                                           \
  if (var_ ## NAME1 . size == 1)                                \
    *(u8*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) OP   \
      *(u8*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);   \
  else if (var_ ## NAME1 . size == 2)                           \
    *(u16*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) OP  \
      *(u16*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);  \
  else if (var_ ## NAME1 . size == 4)                           \
    *(u32*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) OP  \
      *(u32*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);  \
  else if (var_ ## NAME1 . size == 8)                           \
    *(u64*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset) OP  \
      *(u64*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);  \
  else                                                          \
    __builtin_trap();

#define op_mov(NAME, CONST) op_generic_vm(NAME, CONST, =)
#define op_add(NAME, CONST) op_generic_vm(NAME, CONST, +=)
#define op_sub(NAME, CONST) op_generic_vm(NAME, CONST, -=)
#define op_and(NAME, CONST) op_generic_vm(NAME, CONST, &=)
#define op_or(NAME, CONST)  op_generic_vm(NAME, CONST, |=)
#define op_xor(NAME, CONST) op_generic_vm(NAME, CONST, ^=)
#define op_shl(NAME, CONST) op_generic_vm(NAME, CONST, <<=)
#define op_shr(NAME, CONST) op_generic_vm(NAME, CONST, >>=)

/* Operation with VM variable (no update) */
#define op_calc_vm(NAME1, NAME2, OP)                                    \
  ({                                                                    \
    u64 retval;                                                         \
    if (var_ ## NAME1 . size != var_ ## NAME2 . size)                   \
      __builtin_trap();                                                 \
    if (var_ ## NAME1 . size == 1)                                      \
      retval = *(u8*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset)   \
        OP *(u8*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);      \
    else if (var_ ## NAME1 . size == 2)                                 \
      retval = *(u16*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset)  \
        OP *(u16*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);     \
    else if (var_ ## NAME1 . size == 4)                                 \
      retval = *(u32*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset)  \
        OP *(u32*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);     \
    else if (var_ ## NAME1 . size == 8)                                 \
      retval = *(u64*)(var_ ## NAME1 . scope + var_ ## NAME1 . offset)  \
        OP *(u64*)(var_ ## NAME2 . scope + var_ ## NAME2 . offset);     \
    else                                                                \
      __builtin_trap();                                                 \
    retval;                                                             \
  })

#define op_eq(NAME, CONST) op_calc_vm(NAME, CONST, ==)
#define op_ne(NAME, CONST) op_calc_vm(NAME, CONST, !=)
#define op_lt(NAME, CONST) op_calc_vm(NAME, CONST, <)
#define op_le(NAME, CONST) op_calc_vm(NAME, CONST, <=)
#define op_gt(NAME, CONST) op_calc_vm(NAME, CONST, >)
#define op_ge(NAME, CONST) op_calc_vm(NAME, CONST, >=)

/* Operation with immediate value */
#define op_generic_imm(NAME, CONST, OP)                                 \
  if (var_ ## NAME . size == 1)                                         \
    *(u8*)(var_ ## NAME . scope + var_ ## NAME . offset) OP (u8)( CONST ); \
  else if (var_ ## NAME . size == 2)                                    \
    *(u16*)(var_ ## NAME . scope + var_ ## NAME . offset) OP (u16)( CONST ); \
  else if (var_ ## NAME . size == 4)                                    \
    *(u32*)(var_ ## NAME . scope + var_ ## NAME . offset) OP (u32)( CONST ); \
  else if (var_ ## NAME . size == 8)                                    \
    *(u64*)(var_ ## NAME . scope + var_ ## NAME . offset) OP (u64)( CONST ); \
  else                                                                  \
    __builtin_trap();

#define op_movi(NAME, CONST) op_generic_imm(NAME, CONST, =)
#define op_addi(NAME, CONST) op_generic_imm(NAME, CONST, +=)
#define op_subi(NAME, CONST) op_generic_imm(NAME, CONST, -=)
#define op_andi(NAME, CONST) op_generic_imm(NAME, CONST, &=)
#define op_ori(NAME, CONST)  op_generic_imm(NAME, CONST, |=)
#define op_xori(NAME, CONST) op_generic_imm(NAME, CONST, ^=)
#define op_shli(NAME, CONST) op_generic_imm(NAME, CONST, <<=)
#define op_shri(NAME, CONST) op_generic_imm(NAME, CONST, >>=)

/* Operation with constant (no update) */
#define op_calc_imm(NAME, CONST, OP)                                    \
  ({                                                                    \
    u64 retval;                                                         \
    if (var_ ## NAME . size == 1)                                       \
      retval = *(u8*)(var_ ## NAME . scope + var_ ## NAME . offset)     \
        OP (u8)( CONST );                                               \
    else if (var_ ## NAME . size == 2)                                  \
      retval = *(u16*)(var_ ## NAME . scope + var_ ## NAME . offset)    \
        OP (u16)( CONST );                                              \
    else if (var_ ## NAME . size == 4)                                  \
      retval = *(u32*)(var_ ## NAME . scope + var_ ## NAME . offset)    \
        OP (u32)( CONST );                                              \
    else if (var_ ## NAME . size == 8)                                  \
      retval = *(u64*)(var_ ## NAME . scope + var_ ## NAME . offset)    \
        OP (u64)( CONST );                                              \
    else                                                                \
      __builtin_trap();                                                 \
    retval;                                                             \
  })

#define op_eqi(NAME, CONST) op_calc_imm(NAME, CONST, ==)
#define op_nei(NAME, CONST) op_calc_imm(NAME, CONST, !=)
#define op_lti(NAME, CONST) op_calc_imm(NAME, CONST, <)
#define op_lei(NAME, CONST) op_calc_imm(NAME, CONST, <=)
#define op_gti(NAME, CONST) op_calc_imm(NAME, CONST, >)
#define op_gei(NAME, CONST) op_calc_imm(NAME, CONST, >=)

/* Other operations */
#define ROL32(X, N) (u32)(((u32)(X) << ((N) & 31)) | ((u32)(X) >> ((-N) & 31)))
#define op_rol32(NAME, CONST)                                           \
  if (var_ ## NAME . size == 4)                                         \
    *(u32*)(var_ ## NAME . scope + var_ ## NAME . offset) =             \
      ROL32(*(u32*)(var_ ## NAME . scope + var_ ## NAME . offset),      \
            ( CONST ));                                                 \
  else                                                                  \
    __builtin_trap();
#define op_be32(NAME)                                                   \
  if (var_ ## NAME . size == 4)                                         \
    *(u32*)(var_ ## NAME . scope + var_ ## NAME . offset) =             \
      __builtin_bswap32(                                                \
        *(u32*)(var_ ## NAME . scope + var_ ## NAME . offset)           \
      );                                                                \
  else                                                                  \
    __builtin_trap();

#endif
