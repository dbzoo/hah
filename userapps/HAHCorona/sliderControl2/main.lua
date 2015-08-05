--[[ $Id: main.lua 499 2013-11-29 00:49:30Z dbzoo.com $
   Demonstrate using a slider to control an xAPBSC text endpoint.
   Copyright (c) Brett England, 2014

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]

local xap = require("hahlib.xap.init")
local bscSend = require("hahlib.interface.bsc").bscSend
local widget = require "widget"

--xap_server = xap.TCPServer{host="livebox.local"}
xap_server = xap.UDPServer()
xap_server:start()

function levelController(opt)
  return function(event)
	    -- Normalize the slider 0-100 value into the range 0->(states-1)
		local idx = 0
		if event.value > 0 then
		   idx = math.ceil(event.value/ (100/#opt.states))-1
		end
		local temperature=opt.states[idx+1]
		opt.listener(temperature)
		if event.phase == "ended" then
		  bscSend{target=opt.xapTarget, text=temperature, state="on"}
		  --print(string.format("bsc.sendText(%s, %s)", opt.xapTarget, temperature))
		end
	end
end

function newHorizSliderWithFeedback(opt)
	local textFeedback = display.newText{
	   text = string.format(opt.msg, "?"),
	   x=opt.width / 2,
	   y=opt.top + 40,
	   fontSize=30
	}
	
	function updateFeedback(desc)
	   textFeedback.text = string.format(opt.msg, desc)
	end
	
	local controller = levelController{ states = opt.states, 
										listener = updateFeedback,
										xapTarget = opt.xapTarget 
									}
									
	local horizontalSlider = widget.newSlider {
		top=opt.top,
		left=opt.left,
		width = opt.width,
		orientation = "horizontal",
		listener = controller
	}
	
	-- Monitor so a change will update the slider
	local f = xap.Filter()
	f:add("xap-header","source",opt.xapTarget)
	f:add("xap-header","class","xAPBSC.event")
	f:callback(function(frame)
	              local temp = frame:getValue("output.state","text")				  
				  local sliderlevel = ((temp-opt.states[1]) / #opt.states) * 100				  
				  if sliderLevel < 0 then
				      sliderLevel = 0
			      end
				  horizontalSlider:setValue( sliderlevel )
				  -- Synthetic event to update internal state and fire text feedback.
				  controller{value=sliderlevel}
			   end			
			)
	xap_server:addFilter(f)	

    local grp = display.newGroup()									
	grp:insert(horizontalSlider)
	grp:insert(textFeedback)
	return grp	
end

---- MAIN ----
-- Define the temperature range the slider will operate in.
stateTemp={}
for i=15,25 do table.insert(stateTemp,i) end

margin=20
newHorizSliderWithFeedback {
  top=0,
  left=margin,
  width = display.actualContentWidth-margin*2,
  states = stateTemp,
  msg = "Heating at %sC",
  xapTarget="dbzoo.livebox.Plugboard:Thermostat"
}
