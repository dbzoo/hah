-- Testing message parsing with independant Frames.
require("xap")

msg = "xap-header\
{\
target=dbzoo.livebox.Controller:relay.1\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
state=off\
}\
"

xap.init("dbzoo.lua.frame","FF00AA00","br0")

f = xap.Frame(msg)
print(f)  -- Equiv to print(tostring(f))
print(string.format("Packet size: %d bytes",f.len))

assert(f:getType() == xap.MSG_ORDINARY, "getType failure")
assert(f:getValue("xap-header","class") == "xAPBSC.cmd")
assert(f:isValue("output.state.1","state","off"), "state is not off")

-- Expand short xAP format
newmsg = xap.fillShort(msg)
print(newmsg)

msg = "xap-header\
{\
target=I\
class=udp.data\
}\
query\
{\
}"

-- Handling of an empty Sections
f = xap.Frame(msg)
print("Dumping a short frame")
print(f)
assert(f:getValue("query","x") == nil)
assert(f:getValue("query",NULL) == xap.FILTER_ANY)
assert(f:isValue("query",NULL, xap.FILTER_ANY))

print("All tests completed")
