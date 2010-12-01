-- UDP to xAP gateway
--
-- Demonstrates listening on a socket and
-- rebroadcasting the incoming data as an xAP message

require("xap")
require("socket")

function handleSocket()
   dgram, ip, port = udp:receivefrom()
   print("Received '"..dgram .. "' from "..ip..":"..port)
   msg = string.format("xap-header\
{\
class=udp.data\
}\
body\
{\
ip=%s\
port=%s\
data=%s\
}", ip, port, dgram)
   xap.send(xap.fillShort(msg))
end


host = host or "localhost"
port = port or 6666
if arg then
    host = arg[1] or host
    port = arg[2] or port
end

print("Binding to host '" ..host.. "' and port " ..port.. "...")

xap.init("dbzoo.lua.socket","FF00CC00")
host = socket.dns.toip(host)
udp = assert(socket.udp())
assert(udp:setsockname(host, port))

xap.Select(handleSocket, udp:getfd())

ip, port = udp:getsockname()
print("Waiting for packets on " .. ip .. ":" .. port .. "...")

xap.process()
