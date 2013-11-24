--[[ $Id$
   Copyright (c) Brett England, Gary Williams 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]

local storyboard = require "storyboard"
local scene = storyboard.newScene()
local widget = require "widget"
local url = require("socket.url")

local xivelyFeed = 1358187855

function scene:createScene( event )
	local loading
	local view = self.view
	local graph
	local newGraphbtn = { x = 0, y = 0, width = 150, height = 40 }
	-- Position graph under the newGraphBtn
	local graphBounds = { x = 0, 
						  y = newGraphbtn.y + newGraphbtn.height, 
						  width = display.viewableContentWidth, 
						  height = display.viewableContentHeight - newGraphbtn.height - widget.theme.segmentedControl.height }

	-- Show something whilst network traffic is happening.
	local loading = display.newGroup()
	local r = display.newRect(graphBounds.x, graphBounds.y, graphBounds.width, graphBounds.height)
	r:setFillColor(128,128,128)	
	loading:insert(r)
	local spinner = widget.newSpinner{left = display.contentCenterX - widget.theme.spinner.width/2,
									  top = display.contentCenterY - widget.theme.spinner.height/2
   								      }	
	loading:insert(spinner)		
	loading.isVisible = false
	view:insert(loading)
	
	local function graphload(datastream, pickerGroup)
		loading.isVisible = true
		spinner:start()
		local file = system.pathForFile( "XivelyChart.png", system.TemporaryDirectory )
		os.remove( file )
		local datastreamNumber = datastream[1].index - 1
		local httpaddress= "http://api.xively.com/v2/feeds/"..xivelyFeed.."/datastreams/"..datastreamNumber..".png?duration="..datastream[2].value.."&w="..graphBounds.width.."&h="..graphBounds.height.."&title="..url.escape(datastream[1].value).."&b=true&g=true"
		httpaddress = string.gsub(httpaddress," ", "+")
		display.loadRemoteImage( httpaddress, "GET", 
								function(event)
									spinner:stop()
									loading.isVisible = false
									if event.isError == false then
										pickerGroup.isVisible = false										
										graph = event.target
										view:insert(graph)									    
									end
								end, 
								"XivelyChart.png", system.TemporaryDirectory, 
								graphBounds.x, graphBounds.y )
	end
	
	local function pickerWheelPopup()
		local streamIDs = { "Electric", "Utility Temp", "Boiler Status", "Outside Temp", "Security Alarm", 
						"Water Tank Bottom", "Wind", "Rain", "Thermostat", "Total Electric", "Water Tank Top", 
						"Front Room Temp", "Back Room Temp", "Landing Temp", "Smoke Alarm", "Media Temp", 
						"Ave House Temp", "Electric Daily History", "Boiler Daily History", "Pressure"   }

		local durationTimes = { "2hours", "6hours", "12hours", "24hours", "7days", "30days", "1month", "3months", "12months"}	
		local grp = display.newGroup()
		local columnData = 	{ 
							   { 
								  align = "left",
								  startIndex = 1,
								  width=180,
								  labels = streamIDs
							   },
							   { 
								  align = "left",
								  startIndex = 1,
								  width=display.contentWidth-180,
								  labels = durationTimes
							   }
							}

		myPicker = widget.newPickerWheel
		{
			left = 0, top = 0,	
			font = native.systemFontBold,
			fontSize = 17,
			columns = columnData
		}		
		grp:insert(myPicker)
		
		local btnWidth = 100
		selectButton = widget.newButton
			{
				left = display.contentCenterX-btnWidth/2, top = myPicker.height,
				width= btnWidth, height=40,		
				font="Arial", fontSize=18,
				label="Select", labelAlign="center",
				labelColor =  { default = {0,0,0}, over = {255,255,255} },
				onPress = function()
							grp.isVisible = false
							graphload(myPicker:getValues(), grp)
						end
			}
		grp:insert(selectButton)
		grp.isVisible = false -- we initialize hidden.
		return grp
	end

    -- Build the UI	
	local pickerGroup = pickerWheelPopup()
	view:insert(widget.newButton
			{
				left = newGraphbtn.x, top= newGraphbtn.y,
				width= newGraphbtn.width, height=newGraphbtn.height,		
				font="Arial", fontSize=18,
				label="New Graph", labelAlign="center",
				labelColor =  { default = {0,0,0}, over = {255,255,255} },
				onEvent = function()
							-- If there was a graph being displayed remove it.
							if graph then
								view:remove(graph)
								graph = nil
							end
							pickerGroup.isVisible = true
						  end
			}
	)	
	view:insert(pickerGroup)
end

scene:addEventListener( "createScene", scene )

return scene
