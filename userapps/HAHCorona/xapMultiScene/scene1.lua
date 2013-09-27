--[[ $Id$
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]

local bsc = require "hahlib.interface.bsc"
local storyboard = require "storyboard"
local scene = storyboard.newScene()

function scene:createScene( event )

	-- GRID helper functions
	function toggle(v, xpos, ypos)
		v.x = xpos
		v.y = ypos
		v.size = v.size or 20
		v.default = v.default or "assets/glossy_button_red.png"
		v.on = v.on or "assets/glossy_button_green.png"
		return bsc.newToggle(v)
	end
	
	function button(v, xpos, ypos)
		return bsc.newButton {
				defaultSrc = v.icon, defaultX = 80, defaultY = 80, -- img, width, height
				x=xpos, y=ypos, -- position
				size=30, -- font size
				offset=-18,
				label={text=v.label, offset=20, size=18, textColor={ 0, 0, 0, 255 }},
				textColor={ 0, 0, 0, 255 }, -- r,g,b,a
				xapSource=v.xapSource,
		}
	end
	
	-- A Label is a button that is does not accept user press events.
	function label(v, xpos, ypos)
		return bsc.newButton {
				defaultSrc = v.icon, defaultX = 80, defaultY = 80, -- img, width, height
				x=xpos, y=ypos, -- position
				size=30, -- font size
				offset=-18,
				label={text=v.label, offset=20, size=18, textColor={ 0, 0, 0, 255 }},
				textColor={ 0, 0, 0, 255 }, -- r,g,b,a
				xapSource=v.xapSource,
				readOnly=true -- Disable user press events.
		}
	end

    local group = self.view

    local bg = display.newRect( 0, 0, _W, _H )
    bg:setFillColor( 225 )
    group:insert(bg)
   
	local grid = {
        { -- ROW 1
		 {c=toggle, text="Relay 1", xapSource="dbzoo.livebox.Controller:relay.1", size=18, readOnly=true},
		 {c=toggle, text="Relay 2", xapSource="dbzoo.livebox.Controller:relay.2", size=18},
		 {c=toggle, text="Relay 3", xapSource="dbzoo.livebox.Controller:relay.3", size=18},
		 {c=toggle, text="Relay 4", xapSource="dbzoo.livebox.Controller:relay.4", size=18},
		},
        { -- ROW 2
		 {c=toggle, text="RF 1", xapSource="dbzoo.livebox.Controller:rf.1"},
		 {c=toggle, text="RF 2", xapSource="dbzoo.livebox.Controller:rf.2"},
		 {c=toggle, text="RF 3", xapSource="dbzoo.livebox.Controller:rf.3"},
		 {c=toggle, text="RF 4", xapSource="dbzoo.livebox.Controller:rf.4"},
		},			 
		{ -- ROW 3
		 {c=label, icon="assets/outside-temp-icon.png", label="Outside", xapSource='dbzoo.livebox.jeenode:outside.temp2'},
		 {c=label, icon="assets/inside-temp-icon.png", label="Lounge", xapSource='dbzoo.nanode.jeenode:4.temp'},
		 {c=label, icon="assets/inside-temp-icon.png", label="Office", xapSource='dbzoo.livebox.controller:1wire.1'},
		 {c=label, icon="assets/inside-temp-icon.png", label="Store", xapSource='dbzoo.livebox.jeenode:store.temp'},
		},
		{ -- ROW 4
		 {c=label, icon="assets/lightbulb.png", label="Sunshine", xapSource='dbzoo.livebox.jeenode:outside.light'},
		 {c=label, icon="assets/lightbulb.png", label="Lounge", xapSource='dbzoo.nanode.jeenode:4.light'},
		 {},
		 {c=label, icon="assets/lightbulb.png", label="Store", xapSource='dbzoo.livebox.jeenode:store.light'},
		},
		{ -- ROW 5
		 {c=toggle, text="lobat", xapSource="dbzoo.livebox.jeenode:outside.lobat", readOnly=true},
		 {c=toggle, text="lobat", xapSource="dbzoo.livebox.nanode:4.lobat", readOnly=true},
		 {},
		 {},
		}
    }   
   
	-- Add grid
	local ypos = 40, xpos
	for _,r in ipairs(grid) do		
		xpos=40
		for _,v in pairs(r) do
			if v.c then
				group:insert(v.c(v, xpos, ypos))
			end
			 xpos = xpos + 80
		end
		ypos = ypos + 80
	end
   
end

scene:addEventListener( "createScene", scene )
return scene