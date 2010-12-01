-- Full duplex TCP to xAP gateway
--
-- Listen on an IP/PORT and rebroadcasting the incoming data as an xAP message
-- Acccept incoming data packets and forward to the IP/PORT
-- Useful for connecting ethernet/serial hardware devices: Lantronix, Perle et al.

require("xap")
require("socket")

function fromSocket()
   local line, error = c:receive()
   local ip,port = c:getpeername()
   print("Received '"..line.."' from "..ip..":"..port)
   local msg = string.format("xap-header\
{\
class=tcp.data\
}\
tcp.rx\
{\
ip=%s\
port=%s\
data=%s\
}", ip,port,line)
   xap.send(xap.fillShort(msg))
end

function toSocket()
  local line = xap.getValue("tcp.tx","data")
  print("Sending "..line)
  c:send(line)
end

host = host or "localhost"
port = port or 6666
if arg then
    host = arg[1] or host
    port = arg[2] or port
end

xap.init("dbzoo.lua.tcpgw","FF00CC00")
host = socket.dns.toip(host)

print("Connecting to host '" ..host.. "' and port " ..port)
c = assert(socket.connect(host,port))
xap.Select(fromSocket, c:getfd())

f = xap.Filter()
f:add("xap-header","target",xap.getSource())
f:add("xap-header","class","tcp.data")
f:add("tcp.tx","ip",host)
f:add("tcp.tx","port",port)
f:add("tcp.tx","data",xap.FILTER_ANY)
f:callback(toSocket)

print("Running...")
xap.process()
