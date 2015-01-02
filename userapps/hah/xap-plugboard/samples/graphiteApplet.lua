-- xAP graphite feeder
-- See http://dbzoo.com/livebox/graphite
module(...,package.seeall)
require "socket"
require "pl"
require "xap"

info={version="1.0", description="xap graphite feeder"}
-- Adjust with your graphite host IP
graphite={port=2003,host="192.168.1.30"}

class={}
class['xAPBSC.event'] = function(frame)
   source = frame:getValue("xap-header","source")
   if source then
      toGraphite(source:gsub(":","."), getBscDatum(frame))
   end
end

function sectionFeedFactory(section)
  return function(frame)
     source = frame:getValue('xap-header','source')
     for k,v in pairs(frame:getSection(section)) do
        toGraphite(source.."."..k, v)
     end
    end
end

class['pv.report'] = sectionFeedFactory('invertor')
class['weather.data'] = sectionFeedFactory('weather.report')

function toGraphite(path, value)
   value = value or "0"
   c = socket.connect(graphite.host, graphite.port)
   if c then
      c:send(path.." "..value.." "..os.time().."\n")
      c:close()
   end
end

function getBscDatum(frame)
   for _,s in ipairs{"input","output"} do
      for _,k in ipairs{"text","level","state"} do
	 bscVal = frame:getValue(s..".state",k)
	 if bscVal and bscVal ~= '?' then
	    if k == "state" then
	       bscVal = bscVal == "on" and "1" or "0"
	    end
	    return bscVal
	 end
      end
   end
   return nil
end

function init()
   for k in pairs(class) do
     f = xap.Filter()
     f:add("xap-header","class", k)
     f:callback(class[k])
   end
end
