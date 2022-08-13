#include "msgpack.h"

int msgpack_dec_map(lua_State *L, int level, const char *buffer, size_t bsize);
int msgpack_dec_array(lua_State *L, int level, const char *buffer, size_t bsize);

/* 解码`Nil` */
int msgpack_dec_nil(lua_State *L) {
  lua_pushlightuserdata(L, NULL);
  return 1;
}

/* 解码`Boolean` */
int msgpack_dec_boolean(lua_State *L, bool ok) {
  lua_pushboolean(L, ok);
  return 1;
}

int msgpack_dec_number(lua_State *L, size_t bit, const char* buffer, size_t bsize) {
  if (bsize < bit)
    return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_float`.(%d, %d)", bit, bsize);
  if (bit == 8)
  {
    xrio_u64_t v = {.i = *(uint64_t*)buffer};
    xrio_ntoh64((uint64_t*)v.ptr);
    if (isnan(v.n) || isinf(v.n))
      return luaL_error(L, "[msgpack decode]: `msgpack_dec_float64` has got `INF` or `NaN` key.");
    lua_pushnumber(L, v.n);
    return 8;
  }
  else
  {
    xrio_u32_t v = {.i = *(uint32_t*)buffer};
    xrio_ntoh32((uint32_t*)v.ptr);
    if (isnan(v.n) || isinf(v.n))
      return luaL_error(L, "[msgpack decode]: `msgpack_dec_float32` has got `INF` or `NaN` key.");
    lua_pushnumber(L, v.n);
    return 4;
  }
}

/* 解码`fixint` */
int msgpack_dec_fixint(lua_State *L, uint8_t v) {
  if (v >= 0xe0)
  {
    lua_pushinteger(L, v - 0x100);
    return 1;
  }
  else if(v <= 0x7f)
  {
    lua_pushinteger(L, v);
    return 1;
  }
  return 0;
}

/* 解码`int` */
int msgpack_dec_int(lua_State *L, size_t bit, const char *buffer, size_t bsize) {
  if (bit == 1) {
    if (bsize == 0)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_int8`.(%d)", bsize);
    lua_pushinteger(L, *(int8_t*)buffer);
    return 1;
  }
  if (bit == 2) {
    if (bsize < bit)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_int16`.(%d)", bsize);
    uint16_t len = *(uint16_t*)buffer; xrio_ntoh16(&len);
    lua_pushinteger(L, (int16_t)len);
    return 2;
  }
  if (bit == 4) {
    if (bsize < bit)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_int32`.(%d)", bsize);
    uint32_t len = *(uint32_t*)buffer; xrio_ntoh32(&len);
    lua_pushinteger(L, (int32_t)len);
    return 4;
  }
  if (bsize < 8)
    return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_int64`.(%d)", bsize);
  uint64_t len = *(uint64_t*)buffer; xrio_ntoh64(&len);
  lua_pushinteger(L, len);
  return 8;
}

/* 解码`uint` */
int msgpack_dec_uint(lua_State *L, size_t bit, const char *buffer, size_t bsize) {
  if (bit == 1) {
    if (bsize == 0)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_uint8`.(%d)", bsize);
    lua_pushinteger(L, *(uint8_t*)buffer);
    return 1;
  }
  if (bit == 2) {
    uint16_t len = *(uint16_t*)buffer; xrio_ntoh16(&len);
    if (bsize < bit)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_uint16`.(%d)", bsize);
    lua_pushinteger(L, (uint16_t)len);
    return 2;
  }
  if (bit == 4) {
    if (bsize < bit)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_uint32`.(%d)", bsize);
    uint32_t len = *(uint32_t*)buffer; xrio_ntoh32(&len);
    lua_pushinteger(L, (uint32_t)len);
    return 4;
  }
  if (bsize < 8)
    return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_uint64`.(%d)", bsize);
  uint64_t len = *(uint64_t*)buffer; xrio_ntoh64(&len);
  lua_pushinteger(L, len);
  return 8;
}

/* 解码`Bin`/`Str` */
int msgpack_dec_string(lua_State *L, size_t bit, const char *buffer, size_t bsize) {
  if (bit == 1) {
    uint8_t len = *(uint8_t*)buffer;
    if (bsize < len)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_string`.");
    lua_pushlstring(L, buffer + bit, len);
    return len + bit;
  }
  if (bit == 2) {
    uint16_t len = *(uint16_t*)buffer; xrio_ntoh16(&len);
    if (bsize < len)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_string`.");
    lua_pushlstring(L, buffer + bit, len);
    return len + bit;
  }
  /* fixstr */
  if (bit >= 0xa0 && bit <= 0xbf) {
    uint8_t len = bit - 0xa0;
    if (bsize < len)
      return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_string`.");
    lua_pushlstring(L, buffer, len);
    return len;
  }
  /* Bin 32 or Str 32 */
  uint32_t len = *(uint32_t*)buffer; xrio_ntoh32(&len);
  if (bsize < len)
    return luaL_error(L, "[msgpack decode]: string buffers not enough in `msgpack_dec_string`.");
#if !defined(USE_MSGPACK_STR24)
  if (len >= 16777216)
    return luaL_error(L, "[msgpack decode]: The string exceeds the parse length.");
#endif
  lua_pushlstring(L, buffer + bit, len);
  return len + bit;
}

int msgpack_dec_array(lua_State *L, int level, const char *buffer, size_t bsize) {
  uint32_t len = 0; size_t expend = bsize;
  uint8_t type = *buffer;

  if (type == MSG_TYPE_ARR16 || type == MSG_TYPE_ARR32)
  {
    if (type == MSG_TYPE_ARR16)
    {
      if (bsize < 3)
        return luaL_error(L, "[msgpack decode]: Insufficient remaining byte array for array16.");
      uint16_t l = *(uint16_t*)(buffer + 1); xrio_ntoh16(&l); len = l;
      buffer += 3; bsize -= 3;
    }
    else // type == MSG_TYPE_ARR32
    {
      if (bsize < 5)
        return luaL_error(L, "[msgpack decode]: Insufficient remaining byte array for array32.");
      uint32_t l = *(uint32_t*)(buffer + 1); xrio_ntoh32(&l); len = l;
      buffer += 5; bsize -= 5;
    }
  }
  else if (type >= 0x90 && type <= 0x9f) // fix array
  {
    buffer++; bsize--; len = type - 0x90;
  }
  else
    return luaL_error(L, "[msgpack decode]: unknown byte type.");

  if (level >= USE_MSGPACK_MAX_STACK)
    return luaL_error(L, "[msgpack error]: The maximum user-defined parsing depth was exceeded.");

  size_t offset; size_t idx = 1;
  // xrio_log("array 长度为: %u\n", len);
  lua_createtable(L, len, 0);
  while (len--)
  {
    uint8_t vt = *buffer;
    switch (vt)
    {
      case MSG_TYPE_NIL:
        offset = msgpack_dec_nil(L);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_TRUE: case MSG_TYPE_FALSE:
        offset = msgpack_dec_boolean(L, vt == MSG_TYPE_TRUE);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_BIN8: case MSG_TYPE_BIN16: case MSG_TYPE_BIN32:
        offset = msgpack_dec_string(L, 1 << (vt - MSG_TYPE_BIN8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_STR8: case MSG_TYPE_STR16: case MSG_TYPE_STR32:
        offset = msgpack_dec_string(L, 1 << (vt - MSG_TYPE_STR8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_FLOAT32: case MSG_TYPE_FLOAT64:
        offset = msgpack_dec_number(L, 4 << (vt - MSG_TYPE_FLOAT32), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_UINT8: case MSG_TYPE_UINT16: case MSG_TYPE_UINT32: case MSG_TYPE_UINT64:
        offset = msgpack_dec_uint(L, 1 << (vt - MSG_TYPE_UINT8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_INT8: case MSG_TYPE_INT16: case MSG_TYPE_INT32: case MSG_TYPE_INT64:
        offset = msgpack_dec_int(L, 1 << (vt - MSG_TYPE_INT8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_ARR16: case MSG_TYPE_ARR32:            /* 定长数组 */
        offset = msgpack_dec_array(L, level + 1, buffer, bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_MAP16: case MSG_TYPE_MAP32:            /* 定长字典 */
        offset = msgpack_dec_map(L, level + 1, buffer, bsize);
        buffer += offset; bsize -= offset;
        break;
      default:
        if (vt <= 0x7f || vt > 0xe0) {     /* fix uint8 */
          offset = msgpack_dec_fixint(L, vt);
          buffer += offset; bsize -= offset;
          break;
        }
        else if (vt >= 0xa0 && vt <= 0xbf) /* fix str */
        {
          offset = msgpack_dec_string(L, vt, ++buffer, --bsize);
          buffer += offset; bsize -= offset;
          break;
        }
        else if (vt >= 0x90 && vt <= 0x9f) /* fix array */
        {
          offset = msgpack_dec_array(L, level + 1, buffer, bsize);
          buffer += offset; bsize -= offset;
          break;
        }
        else if (vt >= 0x80 && vt <= 0x8f) /* fix map */
        {
          offset = msgpack_dec_map(L, level + 1, buffer, bsize);
          buffer += offset; bsize -= offset;
          break;
        }
        return luaL_error(L, "[msgpack decode]: The array value type is not supported.(%d)", vt);
    }
    lua_rawseti(L, -2, idx++);
  }
  luaL_setmetatable(L, "lua_List");
  // xrio_log("end size = %zu\n", bsize);
  return expend - bsize;
}

int msgpack_dec_map(lua_State *L, int level, const char *buffer, size_t bsize) {
  uint8_t type = *buffer;
  
  /* 如果是数组类型 */
  if (type == MSG_TYPE_ARR16 || type == MSG_TYPE_ARR32)
    return msgpack_dec_array(L, level, buffer, bsize);
  else if (type >= 0x90 && type <= 0x9f)
    return msgpack_dec_array(L, level, buffer, bsize);

  uint32_t len = 0; size_t expend = bsize;
  if (type == MSG_TYPE_MAP16 || type == MSG_TYPE_MAP32) {
    if (type == MSG_TYPE_MAP16)
    {
      if (bsize < 3)
        return luaL_error(L, "[msgpack decode]: Insufficient remaining byte array for map16.");
      uint16_t l = *(uint16_t*)(buffer + 1); xrio_ntoh16(&l); len = l;
      buffer += 3; bsize -= 3;
    }
    else // type == MSG_TYPE_MAP32
    {
      if (bsize < 5)
        return luaL_error(L, "[msgpack decode]: Insufficient remaining byte array for map32.");
      uint32_t l = *(uint32_t*)(buffer + 1); xrio_ntoh32(&l); len = l;
      buffer += 5; bsize -= 5;
    }
  }
  else if (type >= 0x80 && type <= 0x8f)  // type == `fixmap`
  {
    buffer++; bsize--; len = type - 0x80;
  }
  else
    return luaL_error(L, "[msgpack decode]: unknown byte type.");

  if (level >= USE_MSGPACK_MAX_STACK)
    return luaL_error(L, "[msgpack error]: The maximum user-defined parsing depth was exceeded.");

  size_t offset;
  // xrio_log("map 长度为: %u\n", len);
  lua_createtable(L, 0, len);
  while (len--)
  {
    
    /* key type */
    uint8_t kt = *buffer;
    switch (kt)
    {
      /* 注意: 为了安全、性能、稳定, 不建议字符串`key`的数量大于`65535` */
      case MSG_TYPE_BIN8: case MSG_TYPE_STR8:
        offset = msgpack_dec_string(L, 1, ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_BIN16: case MSG_TYPE_STR16:
        offset = msgpack_dec_string(L, 2, ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
#if defined(USE_MSGPACK_KEY32) /* 当然, 也可以要求主动开启4字节的超大`KEY`支持 */
      case MSG_TYPE_BIN32: case MSG_TYPE_STR32:
        offset = msgpack_dec_string(L, 4, ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
#endif
      case MSG_TYPE_FLOAT32: case MSG_TYPE_FLOAT64:
        offset = msgpack_dec_number(L, 4 << (kt - MSG_TYPE_FLOAT32), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_UINT8: case MSG_TYPE_UINT16: case MSG_TYPE_UINT32: case MSG_TYPE_UINT64:
        offset = msgpack_dec_uint(L, 1 << (kt - MSG_TYPE_UINT8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      /* 注意: 某些编程语言的`number`会转换为`str`, 但是`Lua`内则不转换. */
      default:
        if (kt <= 0x7f || kt > 0xe0) {     /* fix uint8 */
          offset = msgpack_dec_fixint(L, kt);
          buffer += offset; bsize -= offset;
          break;
        }
        else if (kt >= 0xa0 && kt <= 0xbf) /* fix str */
        {
          offset = msgpack_dec_string(L, kt, ++buffer, --bsize);
          buffer += offset; bsize -= offset;
          break;
        }
#if !defined(USE_MSGPACK_KEY32) /* 如果不支持32位key, 增加一些友好提示. */
        if (kt == MSG_TYPE_BIN32 || kt == MSG_TYPE_STR32)
          return luaL_error(L, "[msgpack decode]: The 32-bit map key is not supported.");
#endif
        return luaL_error(L, "[msgpack decode]: The map key type is not supported.(%d)", kt);
    }

    /* value type */
    uint8_t vt = *buffer;
    switch (vt)
    {
      case MSG_TYPE_NIL:
        offset = msgpack_dec_nil(L);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_TRUE: case MSG_TYPE_FALSE:
        offset = msgpack_dec_boolean(L, vt == MSG_TYPE_TRUE);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_BIN8: case MSG_TYPE_BIN16: case MSG_TYPE_BIN32:
        offset = msgpack_dec_string(L, 1 << (vt - MSG_TYPE_BIN8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_STR8: case MSG_TYPE_STR16: case MSG_TYPE_STR32:
        offset = msgpack_dec_string(L, 1 << (vt - MSG_TYPE_STR8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_FLOAT32: case MSG_TYPE_FLOAT64:
        offset = msgpack_dec_number(L, 4 << (vt - MSG_TYPE_FLOAT32), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_UINT8: case MSG_TYPE_UINT16: case MSG_TYPE_UINT32: case MSG_TYPE_UINT64:
        offset = msgpack_dec_uint(L, 1 << (vt - MSG_TYPE_UINT8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_INT8: case MSG_TYPE_INT16: case MSG_TYPE_INT32: case MSG_TYPE_INT64:
        offset = msgpack_dec_int(L, 1 << (vt - MSG_TYPE_INT8), ++buffer, --bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_ARR16: case MSG_TYPE_ARR32:            /* 定长数组 */
        offset = msgpack_dec_array(L, level + 1, buffer, bsize);
        buffer += offset; bsize -= offset;
        break;
      case MSG_TYPE_MAP16: case MSG_TYPE_MAP32:            /* 定长字典 */
        offset = msgpack_dec_map(L, level + 1, buffer, bsize);
        buffer += offset; bsize -= offset;
        break;
      default:
        if (vt <= 0x7f || vt > 0xe0) {     /* fix uint8 */
          offset = msgpack_dec_fixint(L, vt);
          buffer += offset; bsize -= offset;
          break;
        }
        else if (vt >= 0xa0 && vt <= 0xbf) /* fix str */
        {
          offset = msgpack_dec_string(L, vt, ++buffer, --bsize);
          buffer += offset; bsize -= offset;
          break;
        }
        else if (vt >= 0x90 && vt <= 0x9f) /* fix array */
        {
          offset = msgpack_dec_array(L, level + 1, buffer, bsize);
          buffer += offset; bsize -= offset;
          break;
        }
        else if (vt >= 0x80 && vt <= 0x8f) /* fix map */
        {
          offset = msgpack_dec_map(L, level + 1, buffer, bsize);
          buffer += offset; bsize -= offset;
          break;
        }
        return luaL_error(L, "[msgpack decode]: The map value type is not supported.(%d)", vt);
    }
    lua_rawset(L, -3);
  }
  // xrio_log("end size = %zu\n", bsize);
  return expend - bsize;
}

int msgpack_decode_init(lua_State *L) {
  size_t bsize;
  const char *buffer = luaL_checklstring(L, 1, &bsize);
  if (!buffer || bsize < 1)
    return luaL_error(L, "[msgpack error]: decode buffer was empty");
  msgpack_dec_map(L, 1, buffer, bsize);
  return 1;
}

int lmsgpack_decode(lua_State *L){
  size_t bsize;
  const char *buffer = luaL_checklstring(L, 1, &bsize);
  if (!buffer || bsize < 1)
    return luaL_error(L, "[msgpack error]: decode buffer was empty");
  lua_settop(L, 1);
  /* 使用保护模式调用 */
  lua_pushcfunction(L, msgpack_decode_init);
  lua_pushvalue(L, 1);
  if (LUA_OK == lua_pcall(L, 1, 1, 0))
    return 1;
  lua_pushboolean(L, 0);
  lua_pushvalue(L, -2);
  return 2;
}