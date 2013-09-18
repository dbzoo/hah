--[[
        Fetch sunrise/sunset for an Earth location
	and report for applets that consume our services.
--]]
module(...,package.seeall)
require("xap")
local xml=require("pl.xml")
local http=require("socket.http")

info={
   version="1.1", description="Report Sunrise and Sunset service"
}
-- London
--local latitude=51.505697
--local longitude=-0.120678

-- Adelaide
local latitude=-34.945051
local longitude=138.66666

local sunrise=nil
local sunset=nil

function lookupSun()
   local now=os.date("*t")
   -- use '99' as the timezone in order to automatically work out the 
   -- timezone based on the given latitude/longitude.
   local url = string.format("http://www.earthtools.org/sun/%s/%s/%s/%s/99/%s",
			     latitude, longitude, now.day, now.month, 
			     now.isdst and 1 or 0
			  )
   local xmlstring = http.request(url)
   if(xmlstring == nil) then return end
   
   -- print(xmlstring)
    local d = xml.parse(xmlstring)
    sunrise = d:match [[
<morning>
<sunrise>$sunrise</sunrise>
</morning>
]].sunrise

    sunset = d:match [[
<evening>
<sunset>$sunset</sunset>
</evening>
]].sunset
end

function sendSunriseSunset()
   if sunrise == nil or sunset == nil then
      return 
   end

   local df = Date.Format("HH:MM:SS")
   local Tsunrise = df:parse(sunrise)
   local Tsunset = df:parse(sunset)
   local now = Date()

   -- A time between sunrise and sunset is daylight
   local daylight = now.time > Tsunrise.time and now.time < Tsunset.time                 

   xap.sendShort(string.format([[
xap-header
{
class=sun.forecast
source=dbzoo.livebox.Earthtools
}
forecast
{
sunrise=%s
sunset=%s
daylight=%s
}
]], sunrise, sunset, daylight and "yes" or "no"))
end

function init()
   local expireImmediately = true
   -- lookup new sunrise/sunset forecast every hour.
   xap.Timer(lookupSun, 60*60):start(expireImmediately)
   -- report every 2 minutes.
   xap.Timer(sendSunriseSunset, 2*60):start(expireImmediately)
end
