#include "msgpack.h"

LUAMOD_API int luaopen_lmsgpack(lua_State *L) {
  luaL_checkversion(L);
  if (USE_MSGPACK_MAX_STACK > LUA_MINSTACK)
    lua_checkstack(L, USE_MSGPACK_MAX_STACK);
  /* 元表 */
  luaL_newmetatable(L, "lua_Table");
  luaL_newmetatable(L, "lua_List");

  luaL_Reg msgpack_libs[] = {
    {"encode", lmsgpack_encode},
    {"decode", lmsgpack_decode},
    {NULL, NULL}
  };
  luaL_newlib(L, msgpack_libs);

  /* 关联`empty_array`表 */
  lua_newtable(L);
  luaL_setmetatable(L, "lua_List");
  lua_setfield (L, -2, "empty_array");

  /* 设置版本号 */
  lua_pushnumber(L, 0.1);
  lua_setfield (L, -2, "version");
  return 1;
}