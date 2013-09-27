--[[ $Id$
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local storyboard = require "storyboard"
local scene = storyboard.newScene()
local widget = require "widget"
local xap = require "hahlib.xap.init"

function scene:createScene( event )
    local group = self.view

	-- Use a slider widget as a feedback mechanism...
	-- A little lame as the slider can't be made read-only so the user
	-- still can interact with the Object.  Its only a demo.
	function newSlider(v)

		local toplbl = display.newText(v.text, 0, 0, native.systemFont, 10)
		toplbl:setTextColor(0,0,0)

		v.left=0
		v.orientation="vertical"
		v.height=100
		v.top=toplbl.height
		local slider = widget.newSlider(v)

		local btmlbl = display.newText("", 1+slider.width/2, slider.height+12, native.systemFont, 12)
		btmlbl:setTextColor(0,0,0)

		local f = xap.Filter()
		f:add("xap-header","source", v.xapSource)		
		f:callback(function(frame)
						local temp = tonumber(frame:getValue("input.state","text")) or 0
						-- Scale the temperature to 0-100% for the slider
						-- We use the range 0 to 50 (Adjust if you are colder/hotter)						
						local pct = temp/50*100
						slider:setValue(pct)
						btmlbl.text = temp.." C"
					end		
					)
		xap_server:addFilter(f)
				
		local grp = display.newGroup()
		grp:insert(toplbl)
		grp:insert(slider)	
		grp:insert(btmlbl)
		grp.x = v.x
		grp.y = v.y		
		
		return grp	
	end
	
	local bg = display.newRect( 0, 0, display.contentWidth, display.contentHeight )
	bg:setFillColor( 128 )
   
	group:insert(bg)	
	group:insert(newSlider{text="Outside", xapSource="dbzoo.livebox.jeenode:outside.temp2", x=10, y=0})
	group:insert(newSlider{text="Lounge", xapSource="dbzoo.nanode.jeenode:4.temp", x=60, y=0})
	group:insert(newSlider{text="Office", xapSource="dbzoo.livebox.controller:1wire.1", x=110, y=0})
	group:insert(newSlider{text="Store", xapSource="dbzoo.livebox.jeenode:store.temp", x=160, y=0})	
	group:insert(newSlider{text="Master", xapSource="dbzoo.nanode.jeenode:5.temp", x=210, y=0})
	group:insert(newSlider{text="Family", xapSource="dbzoo.nanode.jeenode:3.temp", x=260, y=0})
end
scene:addEventListener( "createScene", scene )

return scene