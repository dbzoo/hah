-- Simple heartbeat snooper in LUA
require("xap")

function snoop()
   local source = xap.getValue("xap-hbeat","source")
   local class = xap.getValue("xap-hbeat","class")
   print(string.format("Source %s Class %s", source, class))
end

xap.init("dbzoo.lua.test","FF00CC00")
f = xap.Filter()
f:add("xap-hbeat","source",xap.FILTER_ANY)
f:callback(snoop)
xap.process()
