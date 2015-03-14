module(...,package.seeall)
-- Update the NO-IP service with our internet facing IP address
-- Useful to keep a DDNS host setup.

local http = require("socket.http")
local ltn12 = require("ltn12")
require "xap"

-- Update this for your configuration
local noip={username="dbzoo-com",password="xxx",host="dbzoo.ddns.net"}
-- end user updates.

local info={version="1.0", description="no-ip updater feeder"}
local ipProviders={"http://wtfismyip.com/text","http://icanhazip.com","http://www.networksecuritytoolkit.org/nst/tools/ip.php","http://checkip.dyndns.org"}
local respTable = {
      good = "DNS hostname successfully updated",
      nochg = "IP address is current; no update performed",
      nochglocal = "IP address is current; no update performed",
      nohost = "Hostname supplied does not exist under specified account",
      badauth = "Invalid username password combination",
      badagent = "Client disabled - Nop-IP is no longer allowing requests from this update script",
      ["!donator"] = "An update request was sent including a feature that is not available",
      abuse = "Username is blocked due to abuse",
      ["911"] = "A fatal error on no-ip.com's side such as a database outage"
   }
local storedip

function myip()
   for _,url in pairs(ipProviders) do
      c = http.request(url)
      if c then
	 ip = string.sub(c, string.find(c, "%d+.%d+.%d+.%d"))
	 if ip then
	    return ip
	 end
      end
   end
   return nil
end

function noipUpdate(ip)
   local t={}
   r, c = http.request{url="https://dynupdate.no-ip.com/nic/update?hostname="..noip.host.."&myip="..ip,
		       sink=ltn12.sink.table(t),
		       headers={
			  Authorization = "Basic "..(mime.b64(noip.username..":"..noip.password)),
			  ["User-Agent"] = "noip-updaterApplet.lua/"..info.version.." admin@homeautomationhub.com"
		       }
		    }
   return table.concat(t), c
end

function timerUpdate()
   local newip = myip()
   if newip and newip ~= storedip then
      storedip = newip
      content, retcode = noipUpdate(newip)      
      response = string.sub(content,string.find(content,"%a+"))
      err = respTable[response]
      if err == nil then
	    err = "Could not understand the response"
      end
      xap.sendShort(string.format([[
xap-header
{
source=dbzoo.%s.noip
class=noip.update
}
output
{
ip=%s
response=%s
text=%s
}
]], xap.getDeviceID(), storedip, response, err))
   end
end

function init()
   -- Update NO-IP every 15min
   xap.Timer(timerUpdate, 60 *15):start(true)
end
