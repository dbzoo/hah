--[[ 
Reboot a machine

Sample message to start a reboot

Encoding the IP as part of the address allows 
an individual machine to be rebooted.

xap-header
{
v=12
hop=1
uid=FF00AA00
source=dbzoo.acme.test
target=dbzoo.lua.reboot:192.168.1.1
class=reboot
}
host
{
}

Or to reboot all machines on a subnet we
use a wildcard pattern as part of the target.
target=dbzoo.lua.reboot:192.168.1.*

as UDP is broadcast on a subnet this would also suffice
target=dbzoo.lua.reboot:>
--]]

module(...,package.seeall)

require("xap")
require("socket")

info={
   version="1.0", description="Reboot service"
}

function init()
  f = xap.Filter()
  local myip=socket.dns.toip(socket.dns.gethostname())
  f:add("xap-header","target", "dbzoo.lua.reboot:"..myip)
  f:add("xap-header","class","reboot")
  f:add("host", NULL, xap.FILTER_ANY)
  f:callback(function() os.execute("/sbin/reboot") end)
end
