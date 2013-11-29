--[[ $Id$
   Demonstrate using a slider to control an xAPBSC level endpoint.
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]

local xap = require("hahlib.xap.init")
local bscSend = require("hahlib.interface.bsc").bscSend
local widget = require "widget"

xap_server = xap.TCPServer{host="livebox.local"}
--xap_server = xap.UDPServer()
xap_server:start()

function levelController(opt)
  return function(event)
	    -- Normalize the slider 0-100 value into the range 0->(states-1)
		local idx = 0
		if event.value > 0 then
		   idx = math.ceil(event.value / (100/#opt.states))-1
		end		
		opt.listener(opt.states[idx+1])
		if event.phase == "ended" then
   		  local lvl = string.format("%s/%s", idx, #opt.states-1)
		  bscSend{target=opt.xapTarget, level=lvl}
		  print(string.format("bsc.sendLevel(%s, %s)", opt.xapTarget, lvl))
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
	              local xapLevel = frame:getValue("output.state.1","level")
				  local step, range = xapLevel:match("(%d+)/(%d+)")
				  local sliderlevel = (step / range) * 100				  
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
-- The text is used for feedback purposes
state3={"OFF","DIM","ON"}
state5={"OFF","25%","50%","75%","FULL"}
state15={"OFF",1,2,3,4,5,6,7,8,9,10,11,12,13,"FULL"}

newHorizSliderWithFeedback {
  top=10,
  left=10,
  width = display.actualContentWidth - 20,
  states = state15,
  msg = "Lounge lights are %s",
  xapTarget="xap.dbzoo.Plugboard:LoungeLights"
}

newHorizSliderWithFeedback {
  top=70,
  left=10,
  width = display.actualContentWidth/2,
  states = state3,
  msg = "Kitchen lights are %s",
  xapTarget="xap.dbzoo.Plugboard:KitchenLights"
}
