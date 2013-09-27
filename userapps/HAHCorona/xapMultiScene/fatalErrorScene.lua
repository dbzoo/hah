--[[ $Id$
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local storyboard = require "storyboard"
local ui = require "hahlib.interface.ui"
local widget = require "widget"
local scene = storyboard.newScene()

-- Title in event.params.title
-- Error message in event.params.msg
function scene:createScene(event)	
	local group = self.view
	local fontSize = event.params.fontSize or 16	
	local bg = display.newRect( 0, 0, _W, _H )
	bg:setFillColor( 0 )
	group:insert(bg)

	local titleHeight = 100
	local title = ui.newLabel{ bounds={0,0,_W,titleHeight}, 
					 text=event.params.title, 
					 align="center", 
					 size=16}
	group:insert(title)	
	
	
	-- Explain why this App won't run.
	local msg = ui.newLabel{ bounds={10, titleHeight, (_W-10)*2, _H*2 - titleHeight}, 
					 text=event.params.msg, 
					 align="left", 
					 size=fontSize}					 
	group:insert(msg)	
	
	-- Create QUIT button	
	local quitBtn = event.params.needQuitButton
	-- Only an explicit setting of 'false' will remove it.
	if quitBtn == nil or quitBtn == true then
		group:insert(
			widget.newButton
			{
				left = 30, top=_H-40,
				width=_W-60, height=40,		
				font="Arial", fontSize=18,
				label="Quit", labelAlign="center",
				labelColor =  { default = {0,0,0}, over = {255,255,255} },
				onEvent = function() native.requestExit() end
			}
		)
	end
end
scene:addEventListener( "createScene", scene )

return scene