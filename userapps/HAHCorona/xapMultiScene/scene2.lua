--[[ $Id$
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local storyboard = require "storyboard"
local widget = require "widget"
local xap = require "hahlib.xap.init"
local ui = require "hahlib.interface.ui"
local bsc = require "hahlib.interface.bsc"
local scene = storyboard.newScene()

function scene:createScene( event ) 
   
   -- Adorn the BSC compliant switch with label
   function switch(v)
        -- We work in 0,0 coordinate space and everything is relative to this.
		-- Later the entire GROUP will be shifted into its xpos,ypos location.
		v.style = v.style or "onOff"
		local switch = bsc.newSwitch{left=0, top=0, style=v.style, xapSource=v.xapSource}
		local lbl = display.newText(v.text, 15, switch.height, native.systemFont, 16)		
		lbl:setTextColor(0,0,0)		

		local grp = display.newGroup()
		grp:insert(switch)		
		grp:insert(lbl)		
		grp.x = v.left
		grp.y = v.top		
		return grp	
   end

    -- Example of how to use your own ImageSheet to create a custom Switch
	-- the sprite sheet was built using Adobe Flash CS6
	-- Manual converted the .json coord file into LUA
	--
	-- note: You can resize the background/overlay when you create your widget
	-- but the handle can't be resized ?!  That's sucky.
	local options = { 
		   frames = {{x=300,y=0,width=61,height=61}, -- 1: switch handle
					 {x=361,y=0,width=61,height=61}, -- 2: switch handle over
					 {x=0,y=61,width=166,height=62}, -- 3: switch overlay
					 {x=0,y=0,width=300,height=52}   -- 4: background
					}
	}
	local imageSheet = graphics.newImageSheet("assets/customSwitch.png", options)

	function imageSheetSwitch(v)		
		local switch = bsc.newSwitch{width=80, height=30, 
			                         left=0, top=0, 
									 xapSource=v.xapSource,
									 sheet=imageSheet,
									 onOffHandleDefaultFrame=1,
									 onOffHandleOverFrame=2,
									 onOffOverlayFrame=3, onOffOverlayWidth=83, onOffOverlayHeight=31,
									 onOffBackgroundFrame=4, onOffBackgroundWidth=165, onOffBackgroundHeight=31,
									 onOffMask="assets/customSwitch_onOff_mask.png" -- 84x32
									 }
		local text = v.text or ""
		local lbl = display.newText(text, 15, switch.height, native.systemFont, 16)		
		lbl:setTextColor(0,0,0)		

		local grp = display.newGroup()
		grp:insert(switch)		
		grp:insert(lbl)		
		grp.x = v.left
		grp.y = v.top
		return grp
	end

   local group = self.view
   local bg = display.newRect( 0, 0, _W, _H )
   bg:setFillColor( 192 )   
   group:insert(bg)

    -- A row of switches
   	for i=1,4 do
	  group:insert(switch{text="Relay "..i, top=0, left=(i-1)*80, xapSource="dbzoo.livebox.Controller:relay."..i})
	end
    
	-- A row of Checkboxs
	-- bounds = {left,top,width,height}
	group:insert(ui.newLabel{bounds={0,70,200,100},text="Relays 1..4",textColor={0,0,0,255},size=16,align="left"})	
	for i=1,4 do
	   group:insert(bsc.newSwitch{ left=50+i*30, top=60, xapSource="dbzoo.livebox.Controller:relay."..i, style="checkbox"})
	end
		
	-- A column of Radio buttons
	local y = 80
	for i=1,4 do
	   local radio = bsc.newSwitch{ left=0, top=y, xapSource="dbzoo.livebox.Controller:relay."..i, style="radio"}
	   group:insert(radio)
   	   group:insert(ui.newLabel{bounds={radio.width,y+8,200,radio.height},text="Relay "..i,
	                            textColor={255/i,0,0,255},size=16,align="left"})	
	   y = y + 30
	end
	
	-- Custom switches
	y = y + 50
   	for i=1,3 do
	  group:insert(imageSheetSwitch{text="Relay "..i, top=y, left=10+(i-1)*110, 
									xapSource="dbzoo.livebox.Controller:relay."..i})
	end
end
scene:addEventListener( "createScene", scene )

return scene