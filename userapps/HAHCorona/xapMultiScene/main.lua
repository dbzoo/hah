--[[ $Id$
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local xap = require "hahlib.xap.init"
local widget = require "widget" 
local storyboard = require "storyboard"

_W = display.viewableContentWidth
_H = display.viewableContentHeight

-- Setup GLOBAL xap_server variable
-- note: Establishing the object does not make any connections until the engine is started.
xap_server = xap.TCPServer{host="livebox.local", source="dbzoo.corona.demo", timeout=5}

-- Old style connection (still works)
--xap_server = xap.TCPServer("home.dbzoo.com")
--xap_server:setSource("dbzoo.corona.demo")

-- Wrap the engine:start()
function startXap()
	-- We can't start the engine show why and allow the user to quit.
	if not xap_server:start() then
		local options = {
							params = {
								title="xAP startup failure",
								msg=xap_server:getErrorMessage()								
							}
						}
		storyboard.gotoScene("fatalErrorScene", options)
		return false
	end	
	return true
end

function setupInterface()
	-- Callback when the segment button is pressed.
	-- We load a LUA file (scene) based on the buttons position
	local function onControlPress( event )
	   local target = event.target   
	   if target.segmentLabel == "Quit" then
		  native.requestExit()
		  return
	   end
	   storyboard.gotoScene( "scene"..target.segmentNumber, {effect = "crossFade", time=200} )
	end
	
	-- Setup the segment buttons
	-- 3 labels means we need scene1.lua to scene3.lua
	-- Quit is special
	labels = { "Page 1", "Page 2", "Page 3", "Page 4", "Quit" }
	local segmentedControl = widget.newSegmentedControl
	{
	   left = 0,
	   height=30,
	   top = _H-30, -- bottom of screen
	   segments = labels,
	   segmentWidth = _W/#labels, -- Size the number of labels to fill the screen width.
	   defaultSegment = 1,
	   onPress = onControlPress
	}

	-- Synthetic event to get defaultSegment scene loaded.
	onControlPress{target=segmentedControl}
end

-- Display stack trace on the screen if something bad happens
-- Searching the LOGS is too hard.
local unhandledErrorListener = function( event )
	local options = {
						params = {
						    fontSize=12,
							title="Unhandled Error",
							msg=event.errorMessage.."\n"..event.stackTrace,
							needQuitButton=false
						}
					}	
	storyboard.gotoScene("fatalErrorScene", options)
	return true  -- we handled this.
end
Runtime:addEventListener( "unhandledError", unhandledErrorListener )

-- Capture application events		
local onSystem = function( event )
    if event.type == "applicationExit" then
        xap_server:stop()
    elseif event.type == "applicationStart" then
	    if startXap() then
			setupInterface()
		end
	elseif event.type == "applicationSuspend" then
	   xap_server:stop()
	elseif event.type == "applicationResume" then
	   startXap()
    end
end
Runtime:addEventListener( "system", onSystem )


