local math = require("math")
local str = require("string")

_G.message = "Hello, world!"

function main(arg)
    print(arg)
    return 0
end

local x <const> = 5
main(_G.message)
