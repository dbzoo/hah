--[[ $Id$
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
   
   Demonstrate building a user interface to control
   4 relay endpoints.  Pressing the button will generate
   an xAPBSC.cmd and the button's label will reflect the
   state of the xAP endpoint.  "relay 1 on" or "relay 1 off"
--]]
local widget = require("widget")
local xap = require("hahlib.xap.init")

xap_server = xap.TCPServer{host="home.dbzoo.com", timeout=3}

-- Helper function to send an xAPBSC.cmd message
local function send(body)
   local msg = string.format([[
xap-header
{
target=%s
class=xAPBSC.cmd
}
output.state.1
{
id=*
]], body.target)
  body.target=nil
  for k,v in pairs(body) do
    msg = msg .. k.."="..v.."\n"
  end
  msg = msg .. "}"
  xap_server:sendShort(msg)
end

-- Callback when a button is pressed.
local toggleHandler = function(event)
     send{target=event.target.xapSource, state="toggle"}
end

-- Start the xAP engine
xap_server:start()

-- Build an interface of 4 text buttons.
for i=1,4 do
    local buttonWidth = 130
    local baseLabel = "Relay "..i
	local relay = widget.newButton{    
		left = buttonWidth*(i-1)+20,
		top = 0,
		size=18,
		width=buttonWidth,
		height=50,
		label = baseLabel,
		onRelease = toggleHandler
	}
	relay.xapSource = "dbzoo.livebox.Controller:relay."..i	

	-- Attach a filter to update the relays label
	-- when an xAP event happens to it.
	local f = xap.Filter()
	f:add("xap-header","source",relay.xapSource)
	f:callback(function(frame)		
					local state = frame:getValue("output.state","state")
					relay:setLabel(baseLabel.." "..state)
				end		
				)
	xap_server:addFilter(f)
end
