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
xap_server:start()

function levelController(opt)
  local previousValue = 0  
  return function(event)
	  -- A level device can be set to a specific level at its native resolution using
	  -- Level=45 for example (no % sign) or Level= 64/1023
	  -- We will use discrete values of the range 0-100

	  -- Quantize the level into the ranges available
		local idx = 1
		if event.value > 0 then
			idx = math.ceil(event.value / (100/#opt.states))
		end

		-- Only if the slider has moved into the next quantile
		if previousValue ~= idx then
			previousValue = idx
			opt.listener(opt.states[idx])
			local lvl = (idx-1)*(100/(#opt.states-1))
			--print(string.format("bsc.sendLevel(%s, %s)", opt.xapTarget, lvl))
			if event.synthetic ~= true then
			  bscSend{target=opt.xapTarget, level=lvl}
			end
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
	              local level = tonumber(frame:getValue("output.state.1","level"))
				  horizontalSlider:setValue( level )
				  -- Synthetic event to update internal state and fire text feedback.
				  controller{value=level, synthetic=true}
			   end			
			)
	xap_server:addFilter(f)	

    local grp = display.newGroup()									
	grp:insert(horizontalSlider)
	grp:insert(textFeedback)
	return grp	
end

---- MAIN ----
-- The text is use for feedback purposes
state5={"OFF","25%","50%","75%","FULL"}
state3={"OFF","DIM","ON"}

newHorizSliderWithFeedback {
  top=10,
  left=10,
  width = display.actualContentWidth - 20,
  states = state5,
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
