--[[

   Use in conjuction with the IRNode JeeNode sketch.
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
module("xap.irnode", package.seeall)

jeenode = require("xap.jeenode")
class = require("pl.class")

class.IRNode()

function IRNode:_init(config)
   self.base = config.base
   self.uid = config.uid or "FF08DBFE"   
end

function IRNode:process(data)
--[[
-- From the IRNode.pde Sketch
struct {
    int type;
    int bits;
    unsigned long value;
} payload;
--]]
   local type,bits,value = jeenode.bitslicer(data,8,8,32)

   xap.sendShort(string.format([[
xap-header
{
class=IR.Comms
source=%s
uid=%s
}
IR.Received
{
type=%s
bits=%s
value=%s
}]], self.base, self.uid, type, bits, value))
end

return IRNode
