--[[
        Fetch sunrise/sunset for an Earth location
	and report for applets that consume our services.
--]]
module(...,package.seeall)
require("xap")
local xml=require("pl.xml")
local http=require("socket.http")
local cjson=require("cjson")

info={
   version="1.1", description="Report Sunrise and Sunset service"
}
local source = xap.buildXapAddress{instance="Earthtools"}

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
   local url = string.format("http://api.sunrise-sunset.org/json?lat=%s&lng=%s&date=today",latitude,longitude
                          )
   local response,statuscode = http.request(url)
   print (response)
   if response == nil or statuscode ~= 200 then return end

   t = cjson.decode(response)
   if t.status ~= 'OK' then return end

   sunrise = t.results.sunrise
   sunset = t.results.sunset
   daylight = t.results.day_length
end

function sendSunriseSunset()
   if sunrise == nil or sunset == nil then
      return 
   end

   local df = Date.Format(nil)
   local Tsunrise = df:parse(sunrise)
   local Tsunset = df:parse(sunset)
   local now = Date()

   -- A time between sunrise and sunset is daylight
   local daylight = now.time > Tsunrise.time and now.time < Tsunset.time                 

   xap.sendShort(string.format([[
xap-header
{
class=sun.forecast
source=%s
}
forecast
{
sunrise=%s
sunset=%s
daylight=%s
}
]], source, sunrise, sunset, daylight and "yes" or "no"))
end

function init()
   -- lookup new sunrise/sunset forecast every hour.
   xap.Timer(lookupSun, 60*60):start()
   -- report every 2 minutes.
   xap.Timer(sendSunriseSunset, 2*60):start()

   -- Do one now.
   lookupSun()
   sendSunriseSunset()
end
