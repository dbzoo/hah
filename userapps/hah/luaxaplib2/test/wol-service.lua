--[[ 
Standalone Wake-on-LAN service

To wake up a server send an xAP message like

xap-header
{
v=12
hop=1
uid=FF00AA00
source=dbzoo.acme.test
target=dbzoo.lua.wol
class=wol
}
wake
{
mac=00:0a:e4:86:6b:83
}
--]]

require("socket")
require("string")
require("xap")

function wakeOnLan(hwaddr)
        print("Wakeup "..hwaddr)
        local mac = ""
        for n=1,18,3 do
	mac = mac .. string.char(tonumber(hwaddr:sub(n,n+1),16))
        end
        local packet = string.rep(string.char(0xFF), 6) .. string.rep(mac, 16)
	
	local udp = socket.udp()
	udp:setoption('broadcast',true)
	udp:sendto(packet, "255.255.255.255", 47808)
	udp:close()
end

xap.init("dbzoo.lua.wol","FF00CC00")
f = xap.Filter()
f:add("xap-header","target",xap.getSource())
f:add("xap-header","class","wol")
f:add("wake","mac",xap.FILTER_ANY)
f:callback(function() wakeOnLan(xap.getValue("wake","mac")) end)
xap.process()
