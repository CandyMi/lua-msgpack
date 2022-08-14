# lua-msgpack

  Lua MsgPack library written in C.

# Install

  1. Clone the code to the `3rd` directory.

  2. Run the `make build` command to start compiling.

  3. If there are no errors, the compilation is successful.

# Usage

```lua
local msgpack = require "msgpack"
```

## 1. encode

```lua
local msgpack = require "msgpack"

-- map
print(msgpack.encode { a = 1, b = 2, c = 3, list = { 'a', 'b', 'c' } })

-- array
print(msgpack.encode { true, false, null, 1, 2.2, "admin", list = { 1, 2 3}, map = { a = 1} })
```

## 2. decode

```lua
require "utils"
local msgpack = require "msgpack"

var_dump(msgpack.decode('\x82\xa1a\xc2\xa1b\xc3'))
var_dump(msgpack.decode('\x92\xc2\xc3'))
```

# LICENSE

  [MIT](https://github.com/CandyMi/lua-msgpack/blob/master/LICENSE)
