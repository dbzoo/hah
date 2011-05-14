--[[
   Ticker applet
   Monitor a variety of xAP sources and scroll information over the LCD
--]]

module(...,package.seeall)
require("xap")

info={
   version="1.0", description="LCD ticker"
   }

scrollPos = 1
msg = "No data available"
sep = " : "

resultTbl={}

filterMap={
   [xap.Filter{["xap-header"]= { 
		  source="dbzoo.livebox.controller:1wire.1",
		  class="xAPBSC.event"
	       }
	 }] = function(f) 
		 return string.format("%s C",f["input.state"].text)
	      end,
   [xap.Filter{["xap-header"]= { 
		  source="dbzoo.livebox.controller:relay.1",
		  class="xAPBSC.event"
	       }
	 }] = function(f)
		 return "R1 "..f["output.state"].state
	      end,
   [xap.Filter{["xap-header"]= { 
		  source="dbzoo.livebox.controller:relay.2",
		  class="xAPBSC.event"
	       }
	 }] = function(f)
		 return "R2 "..f["output.state"].state
	      end,
   [xap.Filter{["xap-header"]= { 
		  source="dbzoo.livebox.controller:relay.3",
		  class="xAPBSC.event"
	       }
	 }] = function(f)
		 return "R3 "..f["output.state"].state
	      end,
   [xap.Filter{["xap-header"]= { 
		  source="dbzoo.livebox.controller:relay.4",
		  class="xAPBSC.event"
	       }
	 }] = function(f)
		 return "R4 "..f["output.state"].state
	      end
}

function scrollResults()
   if scrollPos >= #msg + #sep then
      scrollPos = 1
   end

   local newMsg = ""
   for _, v in pairs(resultTbl) do
      newMsg = newMsg .. v .. sep
   end
   if #newMsg > 0 then
      msg = newMsg:sub(1,-#sep-1)
   end

   local scrollMsg = msg .. sep .. msg
   scrollMsg = scrollMsg:sub(scrollPos,scrollPos+16)
   displayLCD(scrollMsg)
   scrollPos = scrollPos + 1
end

function displayLCD(msg)
   local msg=string.format([[
xap-header
{
class=xAPBSC.cmd
target=dbzoo.livebox.Controller:lcd
}
output.state.1
{
id=*
state=on
text=%s
}]], msg)
   xap.sendShort(msg)
end

function init()
   for k in pairs(filterMap) do
      k:callback(function(frame, filterMapKey)
		    resultTbl[filterMapKey] = filterMap[filterMapKey](frame)
		 end, k)
   end
   xap.Timer(scrollResults, 10):start()
end
