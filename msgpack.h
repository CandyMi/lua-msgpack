/*
**  LICENSE: BSD
**  Author: CandyMi[https://github.com/candymi]
*/

#define LUA_LIB

#include <core.h>
#include <stdbool.h>

/*
  以下`3`个宏可以改变一些默认行为:
    USE_MSGPACK_STR24     : 默认不会解析超出`24`位大小的字符串, 定义了此宏则会解析.
    USE_MSGPACK_KEY32     : 默认不会解析`32`位大小的字符串`key`, 定义了此宏则会解析.
    USE_MSGPACK_MAX_STACK : 自定义最大栈大小与最大递归解析深度, 超出则会自动结束解析.
*/

#if !defined(USE_MSGPACK_MAX_STACK)
  #define USE_MSGPACK_MAX_STACK LUA_MINSTACK
#endif

#define MSG_TYPE_NIL          0xc0
#define MSG_TYPE_FALSE        0xc2
#define MSG_TYPE_TRUE         0xc3

#define MSG_TYPE_BIN8         0xc4
#define MSG_TYPE_BIN16        0xc5
#define MSG_TYPE_BIN32        0xc6

#define MSG_TYPE_EXT8         0xc7
#define MSG_TYPE_EXT16        0xc8
#define MSG_TYPE_EXT32        0xc9

#define MSG_TYPE_FLOAT32      0xca
#define MSG_TYPE_FLOAT64      0xcb

#define MSG_TYPE_UINT8        0xcc
#define MSG_TYPE_UINT16       0xcd
#define MSG_TYPE_UINT32       0xce
#define MSG_TYPE_UINT64       0xcf

#define MSG_TYPE_INT8         0xd0
#define MSG_TYPE_INT16        0xd1
#define MSG_TYPE_INT32        0xd2
#define MSG_TYPE_INT64        0xd3

#define MSG_TYPE_FIXEXT1      0xd4
#define MSG_TYPE_FIXEXT2      0xd5
#define MSG_TYPE_FIXEXT4      0xd6
#define MSG_TYPE_FIXEXT8      0xd7
#define MSG_TYPE_FIXEXT16     0xd8

#define MSG_TYPE_STR8         0xd9
#define MSG_TYPE_STR16        0xda
#define MSG_TYPE_STR32        0xdb

#define MSG_TYPE_ARR16        0xdc
#define MSG_TYPE_ARR32        0xdd

#define MSG_TYPE_MAP16        0xde
#define MSG_TYPE_MAP32        0xdf

#ifndef xrio_malloc
  #define xrio_malloc malloc
#endif

#ifndef xrio_realloc
  #define xrio_realloc realloc
#endif

#ifndef xrio_free
  #define xrio_free free
#endif

/* 64bit */
typedef union xrio_u64 {
  int64_t   i;
  double    n;
  char ptr[8];
} xrio_u64_t;

/* 32bit */
typedef union xrio_u32 {
  int32_t   i;
  float     n;
  char ptr[4];
} xrio_u32_t;

/* Buffer 实现 */
#define xrio_buffer_size (4096)

typedef struct xrio_Buffer {
  char* b; lua_State *L;
  size_t bidx; size_t blen;
  char ptr[xrio_buffer_size];
} xrio_Buffer;

#define xrio_buffgetidx(B)                (B->bidx)
#define xrio_buffreset(B, idx)            ({B->bidx = idx;})

#define xrio_pushliteral(B, s)            xrio_addlstring((B), (s), strlen(s))
#define xrio_pushstring(B, s)             xrio_addlstring((B), (s), strlen(s))
#define xrio_pushlstring(B, s, l)         xrio_addlstring((B), (s), l)
#define xrio_pushfstring(B, fmt, ...)     ({xrio_addstring(B, lua_pushfstring(B->L, fmt, ##__VA_ARGS__)); lua_pop(L, 1);})

void  xrio_buffinit(lua_State *L, xrio_Buffer *B);
void* xrio_buffinitsize(lua_State *L, xrio_Buffer *B, size_t rsize);
void  xrio_pushresult(xrio_Buffer *B);

void  xrio_addchar(xrio_Buffer *B, char c);
void  xrio_addstring(xrio_Buffer *B, const char *b);
void  xrio_addlstring(xrio_Buffer *B, const char *b, size_t l);

int lmsgpack_encode(lua_State *L);
int lmsgpack_decode(lua_State *L);


/* 字节序交换 */
static inline uint16_t xrio_swap16(uint16_t number) {
  return (__uint16_t) ((((number) >> 8) & 0xff) | (((number) & 0xff) << 8));
}

static inline uint32_t xrio_swap32(uint32_t number) {
  return (((number) & 0xff000000u) >> 24) | (((number) & 0x00ff0000u) >> 8) | (((number) & 0x0000ff00u) << 8) | (((number) & 0x000000ffu) << 24);
}

static inline uint64_t xrio_swap64(uint64_t number) {
  return  (((number) & 0xff00000000000000ull) >> 56)
        | (((number) & 0x00ff000000000000ull) >> 40)
        | (((number) & 0x0000ff0000000000ull) >> 24)
        | (((number) & 0x000000ff00000000ull) >>  8)
        | (((number) & 0x00000000ff000000ull) <<  8)
        | (((number) & 0x0000000000ff0000ull) << 24)
        | (((number) & 0x000000000000ff00ull) << 40)
        | (((number) & 0x00000000000000ffull) << 56);
}

/* `网络字节序`转换`主机字节序` */
static inline void xrio_ntoh16(uint16_t *number) {
#if BYTE_ORDER == LITTLE_ENDIAN
  *number = xrio_swap16(*number);
#else
  (void)number;
#endif
}

static inline void xrio_ntoh32(uint32_t *number) {
#if BYTE_ORDER == LITTLE_ENDIAN
  *number = xrio_swap32(*number);
#else
  (void)number;
#endif
}

static inline void xrio_ntoh64(uint64_t *number) {
#if BYTE_ORDER == LITTLE_ENDIAN
  *number = xrio_swap64(*number);
#else
  (void)number;
#endif
}

/* `主机字节序`转换`网络字节序` */
static inline void xrio_hton16(uint16_t *number) {
#if BYTE_ORDER == LITTLE_ENDIAN
  *number = xrio_swap16(*number);
#else
  (void)number;
#endif
}

static inline void xrio_hton32(uint32_t *number) {
#if BYTE_ORDER == LITTLE_ENDIAN
  *number = xrio_swap32(*number);
#else
  (void)number;
#endif
}

static inline void xrio_hton64(uint64_t *number) {
#if BYTE_ORDER == LITTLE_ENDIAN
  *number = xrio_swap64(*number);
#else
  (void)number;
#endif
}