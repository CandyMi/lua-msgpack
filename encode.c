#include "msgpack.h"

int msgpack_enc_map(lua_State *L, xrio_Buffer *B);
int msgpack_enc_array(lua_State *L, xrio_Buffer *B);

/* 编码`Nil` */
void msgpack_enc_nil(xrio_Buffer *B) {
  xrio_addchar(B, MSG_TYPE_NIL);
}

/* 编码`Boolean` */
void msgpack_enc_boolean(xrio_Buffer *B, bool op) {
  xrio_addchar(B, op == true ? MSG_TYPE_TRUE : MSG_TYPE_FALSE);
}

/* 编码`Number` */
void msgpack_enc_number(xrio_Buffer *B, lua_Number n) {
  float f = (float)n;
  // printf("%.8f, %.8f, %s\n", f, n, (double)f == n ? "true" : "false");
  if ((double)f == n || sizeof(lua_Number) == sizeof(f))
  {
    xrio_hton32((uint32_t*)&f);
    xrio_addchar(B, MSG_TYPE_FLOAT32);
    xrio_addlstring(B, (char*)&f, 4);
  }
  else
  {
    xrio_hton64((uint64_t*)&n);
    xrio_addchar(B, MSG_TYPE_FLOAT64);
    xrio_addlstring(B, (char*)&n, 8);
  }
}

/* 编码`Integer` */
void msgpack_enc_integer(xrio_Buffer *B, lua_Integer i) {
  if (i >= 0)
  {
    if (i <= INT8_MAX) {
      xrio_addchar(B, (uint8_t)i);
      return;
    }
    if (i <= UINT8_MAX) {
      xrio_addchar(B, MSG_TYPE_UINT8);
      xrio_addchar(B, (uint8_t)i);
      return;
    }
    if (i <= UINT16_MAX) {
      uint16_t data = i; 
      xrio_hton16((uint16_t*)&data);
      xrio_addchar(B, MSG_TYPE_UINT16);
      xrio_addlstring(B, (char*)&data, 2);
      return;
    }
    if (i <= UINT32_MAX) {
      uint32_t data = i; 
      xrio_hton32((uint32_t*)&data);
      xrio_addchar(B, MSG_TYPE_UINT32);
      xrio_addlstring(B, (char*)&data, 4);
      return;
    }
  }
  else
  {
    if (i >= -32) {
      xrio_addchar(B, (uint8_t)i);
      return;
    }
    if (i >= INT8_MIN) {
      xrio_addchar(B, MSG_TYPE_INT8);
      xrio_addchar(B, (int8_t)i);
      return;
    }
    if (i >= INT16_MIN) {
      uint16_t data = i;
      xrio_hton16((uint16_t*)&data);
      xrio_addchar(B, MSG_TYPE_INT16);
      xrio_addlstring(B, (char*)&data, 2);
      return;
    }
    if (i >= INT32_MIN) {
      uint32_t data = i;
      xrio_hton32((uint32_t*)&data);
      xrio_addchar(B, MSG_TYPE_INT32);
      xrio_addlstring(B, (char*)&data, 4);
      return;
    }
  }
  uint64_t data = i;
  xrio_hton64((uint64_t*)&data);
  xrio_addchar(B, MSG_TYPE_INT64);
  xrio_addlstring(B, (char*)&data, 8);
  return;
}

int msgpack_enc_string(lua_State *L, xrio_Buffer *B, const char*buffer, size_t bsize) {
  if (bsize <= 31) {
    xrio_addchar(B, 0xa0 + bsize); /* fixstr start position + 数量 */
    xrio_addlstring(B, buffer, bsize);
    return bsize + 1;
  }
  if (bsize <= UINT8_MAX) {
    xrio_addchar(B, MSG_TYPE_STR8);
    xrio_addchar(B, (uint8_t)bsize);
    xrio_addlstring(B, buffer, bsize);
    return bsize + 2;
  }
  if (bsize <= UINT16_MAX) {
    xrio_addchar(B, MSG_TYPE_STR16);
    return bsize + 3;
  }
  if (bsize <= UINT32_MAX) {
    xrio_addchar(B, MSG_TYPE_STR32);
    return bsize + 5;
  }
  return luaL_error(L, "[msgpack encode]: string was too long(%zu).", bsize);
}

/* 编码`Array` */
int msgpack_enc_array(lua_State *L, xrio_Buffer *root) {
  int kt; int vt; size_t count = 0;
  xrio_Buffer B; xrio_buffinit(L, &B);
  lua_pushnil(L);
  while (lua_next(L, -2))
  {
    kt = lua_type(L, -2);
    if (kt != LUA_TNUMBER || lua_tointeger(L, -2) - count != 1) {
      lua_pop(L, 2); B.L = NULL; xrio_pushresult(&B);
      return -1; /* 并非纯数组 */
    }

    /* 获取`Value`字段类型 */
    vt = lua_type(L, -1);
    switch (vt)
    {
      case LUA_TBOOLEAN:
        msgpack_enc_boolean(&B, lua_toboolean(L, -1));
        break;
      case LUA_TLIGHTUSERDATA:
        msgpack_enc_nil(&B);
        break;
      case LUA_TSTRING:
        {
          size_t bsize; const char* buffer = luaL_checklstring(L, -1, &bsize);
          msgpack_enc_string(L, &B, buffer, bsize);
        }
        break;
      case LUA_TNUMBER:
        if (lua_isinteger(L, -1))
          msgpack_enc_integer(&B, lua_tointeger(L, -1));  
        else
          msgpack_enc_number(&B, lua_tonumber(L, -1));
        break;
      case LUA_TTABLE:
        msgpack_enc_map(L, &B);
        break;
      default:
        B.L = NULL; xrio_pushresult(&B);
        return luaL_error(L, "[msgpack encode]: Unsupported array value type `%s`.", lua_typename(L, vt));
    }
    lua_pop(L, 1);
    count++;
  }

  if (count <= 15)
    xrio_addchar(root, 0x90 + count);
  else if(count <= UINT16_MAX)
    xrio_addchar(root, MSG_TYPE_ARR16);
  else if(count <= UINT32_MAX)
    xrio_addchar(root, MSG_TYPE_ARR32);
  else {
    B.L = NULL; xrio_pushresult(&B);
    return luaL_error(L, "[msgpack encode]: Invalid array items `%zu`.", count);
  }

  if (count && B.bidx)
    xrio_addlstring(root, B.b, B.bidx);

  B.L = NULL; xrio_pushresult(&B);
  return 0;
}

/* 编码`Map` */
int msgpack_enc_map(lua_State *L, xrio_Buffer *root) {
  int kt; int vt; size_t count = 0;
  if (lua_rawlen(L, -1) > 0) {
    if (!msgpack_enc_array(L, root))
      return 0;
  } else if (lua_getmetatable(L, -1) == 1 && luaL_getmetatable(L, "lua_List")) {
    int is_list = lua_rawequal(L, -1, -2); lua_pop(L, 2);
    if (is_list && !msgpack_enc_array(L, root))
      return 0;
  }
  lua_pushnil(L);
  xrio_Buffer B; xrio_buffinit(L, &B);
  while (lua_next(L, -2))
  {
    /* 获取`Key`字段类型 */
    kt = lua_type(L, -2);
    switch (kt)
    {
      case LUA_TSTRING:
        {
          size_t bsize; const char* buffer = luaL_checklstring(L, -2, &bsize);
          msgpack_enc_string(L, &B, buffer, bsize);
        }
        break;
      case LUA_TNUMBER:
        if (lua_isinteger(L, -2))
          msgpack_enc_integer(&B, lua_tointeger(L, -2));  
        else
          msgpack_enc_number(&B, lua_tonumber(L, -2));
        break;
      default:
        B.L = NULL; xrio_pushresult(&B);
        return luaL_error(L, "[msgpack encode]: Invalid map key type `%s`.", lua_typename(L, kt));
    }
    /* 获取`Value`字段类型 */
    vt = lua_type(L, -1);
    switch (vt)
    {
      case LUA_TBOOLEAN:
        msgpack_enc_boolean(&B, lua_toboolean(L, -1));
        break;
      case LUA_TLIGHTUSERDATA:
        msgpack_enc_nil(&B);
        break;
      case LUA_TSTRING:
        {
          size_t bsize; const char* buffer = luaL_checklstring(L, -1, &bsize);
          msgpack_enc_string(L, &B, buffer, bsize);
        }
        break;
      case LUA_TNUMBER:
        if (lua_isinteger(L, -1))
          msgpack_enc_integer(&B, lua_tointeger(L, -1));  
        else
          msgpack_enc_number(&B, lua_tonumber(L, -1));
        break;
      case LUA_TTABLE:
        msgpack_enc_map(L, &B);
        break;
      default:
        B.L = NULL; xrio_pushresult(&B);
        return luaL_error(L, "[msgpack encode]: Unsupported map value type `%s`.", lua_typename(L, vt));
    }
    lua_pop(L, 1);
    count++;
  }

  if (count <= 15)
    xrio_addchar(root, 0x80 + count);
  else if(count <= UINT16_MAX)
    xrio_addchar(root, MSG_TYPE_MAP16);
  else if(count <= UINT32_MAX)
    xrio_addchar(root, MSG_TYPE_MAP32);
  else {
    B.L = NULL; xrio_pushresult(&B);
    return luaL_error(L, "[msgpack encode]: Invalid map items `%zu`.", count);
  }

  if (count && B.bidx)
    xrio_addlstring(root, B.b, B.bidx);

  B.L = NULL; xrio_pushresult(&B);
  return 0;
}

int lmsgpack_encode(lua_State *L) {
  if (!lua_istable(L, 1))
    return luaL_error(L, "[msgpack error]: encode need a lua table.");
  lua_settop(L, 1);

  xrio_Buffer root;
  xrio_buffinit(L, &root);
  msgpack_enc_map(L, &root);
  xrio_pushresult(&root);
  return 1;
}