--[[
        Fetch weather from Weather2.com and provide a forecast message
--]]
module(...,package.seeall)
require("xap")
local xml=require("pl.xml")
local http=require("socket.http")

info={
   version="1.0", description="Weather forecast"
}
local location="wa5"
local apikey="<InsertKey>"
local current={}
local forecast={}
local source = xap.buildXapAddress{instance="Plugboard:Weather"}

function lookupWeather()
    local xmlstring = http.request("http://www.myweather2.com/developer/forecast.ashx?uac="..apikey.."&output=xml&query="..location)
 
   if(xmlstring == nil) then return end
    local d = xml.parse(xmlstring)
    current = d:match [[
<weather>
<curren_weather>
<temp>$temp</temp>
<wind><speed>$speed</speed></wind>
<humidity>$humidity</humidity>
<pressure>$pressure</pressure>
</curren_weather>
</weather>
    ]]
  forecast = d:match [[	
<weather>
{{<forecast>
<date>$day</date>
<day_max_temp>$high</day_max_temp>
<night_min_temp>$low</night_min_temp>
</forecast>}}
</weather>
]]
end

function getWeatherBody()
    local msg = string.format([[
current
{
location=%s
temp=%d
wind=%d
humidity=%d
pressure=%d
}
]], location, current.temp, current.speed, current.humidity, current.pressure)
    for i,v in ipairs(forecast) do
        msg = msg ..string.format([[
forecast.%d
{
low=%d
high=%d
day=%s
}
]], i, v.low, v.high, v.day)
     end
    return msg
end
function sendWeather()
        xap.sendShort(string.format([[
xap-header
{
class=weather.forecast
source=%s
}
]], source) .. getWeatherBody())
end

function init()
   local expireImmediately = true
   -- lookup new weather forecast every hour.
   xap.Timer(lookupWeather, 60*60):start(expireImmediately)
   -- report every 2 minutes.
   xap.Timer(sendWeather, 2*60):start(expireImmediately)
end
