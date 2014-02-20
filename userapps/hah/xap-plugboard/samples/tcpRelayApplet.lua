--[[ 
 Full duplex TCP to xAP gateway

 Listen on an IP/PORT and rebroadcasting the incoming data as an xAP message
 Acccept incoming data packets and forward to the IP/PORT
 Useful for connecting ethernet/serial hardware devices: Lantronix, Perle et al.
--]]

module(...,package.seeall)

require("xap")
require("socket")
info={
   version="1.0", description="TCP to xAP gateway"
}

-- Array of hosts and ports to listen on.
listen = {["192.168.11.199"]=3001}
local c

function fromSocket()
   local line, error = c:receive()
   local ip,port = c:getpeername()
   --print("Received '"..line.."' from "..ip..":"..port)
   local msg = string.format([[xap-header
{
class=tcp.data
}
tcp.rx
{
ip=%s
port=%s
data=%s
}]], ip,port,line)
   xap.sendShort(msg)
end

function toSocket(frame)
  local line = frame:getValue("tcp.tx","data")
  --print("Sending "..line)
  c:send(line)
end

function init()
   local f
   for host,port in pairs(listen) do
      io.write("\tConnecting to " ..host.. ":" ..port.."  ")
      c = socket.connect(host,port)
      if c then
	 print("OK")
	 xap.Select(fromSocket, c)
	 
	 f = xap.Filter()
	 f:add("xap-header","target",xap.getSource())
	 f:add("xap-header","class","tcp.data")
	 f:add("tcp.tx","ip",host)
	 f:add("tcp.tx","port",port)
	 f:add("tcp.tx","data",xap.FILTER_ANY)
	 f:callback(toSocket)
      else
	 print("FAIL")
      end
   end
end
