--[[ $Id$
   xAPBSC UI library for Corona
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local M = {}
local xap = require("hahlib.xap.init")
local toggle = require("hahlib.interface.toggle")
local ui = require("hahlib.interface.ui")
local widget = require "widget"

function bscSend(body)
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

-- Decorate the toggle UI to be BSC compliant
function newToggle( param )	
	param.onEvent = function(event)
						if event.phase == "release" then
							bscSend{target=event.target.xapSource, state="toggle"}
						end	         
					end
  	local btn = toggle.newToggle(param)
	btn.xapSource = param.xapSource

	local f = xap.Filter()
	f:add("xap-header","source",btn.xapSource)
	
	f:callback(function(frame)
					-- The toggle widget might be bound to an INPUT/OUTPUT endpoint (ie relay) or
					-- it might be an INPUT only endpoint (ie temp sensor)
					-- Check output first as this is the more common use case.
					for _, section in ipairs{"output.state","input.state"} do
						value = frame:getValue(section,"state")
						if value then
							-- If the Endpoint is ON then our button is PRESSED.
							btn:setState(value == "on")
							return
						end
					end					
				end		
				)				
	xap_server:addFilter(f)
	return btn
end

-- Decorate the UI button with BSC compliance
function newButton( param )
	local btn = ui.newButton(param)
	btn.xapSource = param.xapSource
	
	local f = xap.Filter()
	f:add("xap-header","source",btn.xapSource)
	f:callback(function(frame)
					btn:setText(frame:getValue("input.state","text"))
				end		
				)
	xap_server:addFilter(f)
	return btn
end

-- Creating a custom WIDGET
-- Using a ON/OFF switch widget we adorn so its becomes xAP aware.
function newSwitch(v)
	v.onPress = function(event)					
					bscSend{target=event.target.xapSource, state="toggle"}
				end
	local switch = widget.newSwitch(v)
	switch.xapSource = v.xapSource

	local f = xap.Filter()
	f:add("xap-header","source", switch.xapSource)
	
	f:callback(function(frame)
					switch:setState{isOn=frame:isValue("output.state","state","on"), isAnimated=true}
				end		
				)				
	xap_server:addFilter(f)
				
	return switch	
end

M.bscSend = bscSend
M.newButton = newButton
M.newToggle = newToggle
M.newSwitch = newSwitch

return M