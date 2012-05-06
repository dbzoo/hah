--[[
	Fetch weather from GOOGLE and provide a forecast message
--]]
module(...,package.seeall)

require("xap")
require("pl.xml")
local http = require("socket.http")

info={
   version="1.0", description="Weather forecast"
}
local location="glasgow,uk"

function math.round(number, decimals)
    decimals = decimals or 0
    return tonumber(("%."..decimals.."f"):format(number))
end
local tocelcius = function(f) return math.round((f-32)*5/9,1) end
local current={}
local forecast={}

function lookupWeather()
    local xmlstring = http.request("http://www.google.com/ig/api?weather="..location)
    if(xmlstring == nil) then return end

    local d = xml.parse(xmlstring)
    current = d:match [[
<weather>
  <current_conditions>
    <condition data='$condition'/>
    <temp_c data='$temp'/>
  </current_conditions>
</weather>
]]

    forecast = d:match [[
<weather>
   {{<forecast_conditions>
     <day_of_week data='$day'/>
     <low data='$low'/>
     <high data='$high'/>
     <condition data='$condition'/>
   </forecast_conditions>}}
</weather>
]]

    -- Damn google returns units in degrees F
    -- Other countries like Spain, Germany etc are in C?!
    for _,v in ipairs(forecast) do
	 v.high = tocelcius(v.high)
         v.low = tocelcius(v.low)
    end
end

function getWeatherBody()
    local msg = string.format([[
current
{
location=%s
temp=%s
condition=%s
}
]], location, current.temp, current.condition)

    for i,v in ipairs(forecast) do
    	msg = msg ..string.format([[
forecast.%d
{
low=%d
high=%d
day=%s
condition=%s
}
]], i, v.low, v.high, v.day, v.condition)
    end
    return msg
end

function sendWeather()
	xap.sendShort([[
xap-header
{
class=weather.forecast
source=dbzoo.weather.station
}
]] .. getWeatherBody())
end

function init()
   local expireImmediately = true
   -- lookup new weather forecast every hour.
   xap.Timer(lookupWeather, 60*60):start(expireImmediately)
   -- report every 2 minutes.
   xap.Timer(sendWeather, 2*60):start(expireImmediately)
end
