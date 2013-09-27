--[[ $Id$
   Demonstrate an xAP engine with a filter selecting a value and displaying it.
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]

local xap = require("hahlib.xap.init")
local widget = require "widget"

xap_server = xap.TCPServer{host="home.dbzoo.com"}
xap_server:start()

local outside = widget.newButton {
    top=display.contentCenterY,
    left=display.contentCenterX,
    defaultFile = "temp-icon.png",    
    fontSize = 26,	
    width = 100,
    height = 100,
	emboss=true,
	isEnabled=false -- No touch events.
}

local f = xap.Filter()
f:add("xap-header","source","dbzoo.livebox.Controller:1wire.1")
f:callback(function(frame)				
				outside:setLabel("  "..frame:getValue("input.state","text").."\nOutside")
			end			
			)
xap_server:addFilter(f)
