-- Lifted from http://developer.coronalabs.com/code/toggle
-- And modified by Brett England
local M = {}

-----------------
-- Helper function for newToggle utility function below
local function newToggleHandler( self, event )
	local result = true

	local default = self[1]
	local on = self[2]
	
	-- General "onEvent" function overrides onPress and onRelease, if present
	local onEvent = self._onEvent
	
	local buttonEvent = { value = self.value, target = self }
	if (self._id) then
		buttonEvent.id = self._id
	end

	local phase = event.phase
	if "began" == phase then

		if onEvent then
			buttonEvent.phase = "press"
			result = onEvent( buttonEvent )
		end

		-- Subsequent touch events will target button even if they are outside the stageBounds of button
		display.getCurrentStage():setFocus( self, event.id )
		self.isFocus = true
		
	elseif self.isFocus then
		local bounds = self.stageBounds
		local x,y = event.x,event.y
		local isWithinBounds = 
			bounds.xMin <= x and bounds.xMax >= x and bounds.yMin <= y and bounds.yMax >= y

		if "moved" == phase then
			
		elseif "ended" == phase or "cancelled" == phase then 
			
			if "ended" == phase then
				-- Only consider this a "click" if the user lifts their finger inside button's stageBounds
				if isWithinBounds then				    
					self.value = self.value == false
					buttonEvent.value = self.value

					default.isVisible = buttonEvent.value == true
					on.isVisible = buttonEvent.value == false
					if onEvent then
						buttonEvent.phase = "release"
						result = onEvent( buttonEvent )
					end
				end
			end			
			-- Allow touch events to be sent normally to the objects they "hit"
			display.getCurrentStage():setFocus( self, nil )
			self.isFocus = false
		end
	end

	return result
end


---------------
-- Button class

function newToggle( params )
	local button, default, on, offset	
	local sizeDivide = 1
 	local sizeMultiply = 1
 
    if display.contentScaleX < 1.0 or display.contentScaleY < 1.0 then
                sizeMultiply = 2
                sizeDivide = 0.5                
        end

	if params.default then
		button = display.newGroup()
		default = display.newImage( params.default )
		button:insert( default, true )
	end
	
	if params.on then
		on = display.newImage( params.on )
		on.isVisible = false
		button:insert( on, true )
	end
	
	if (params.onEvent and ( type(params.onEvent) == "function" ) ) then
		button._onEvent = params.onEvent
	end

	if params.readOnly ~= true then
	   -- Set button as a table listener by setting a table method and adding the button as its own table listener for "touch" events
   	   button.touch = newToggleHandler
	   button:addEventListener( "touch", button )
	end
	
	if params.x then
		button.x = params.x
	end
	
	if params.y then
		button.y = params.y
	end
	
	if params.id then
		button._id = params.id
	end

	button.value = 0

    -- Public methods
    function button:setState(s)
		self.value = s == false
		self[1].isVisible = self.value == true
		self[2].isVisible = self.value == false	
	end
	
    function button:setText( newText )        
		local labelText = self.text
		if ( labelText ) then
				labelText:removeSelf()
				self.text = nil
		end
		
		if ( params.size and type(params.size) == "number" ) then size=params.size else size=20 end
		size = size * sizeMultiply
		if ( params.font ) then font=params.font else font=native.systemFontBold end
		if ( params.textColor ) then textColor=params.textColor else textColor={ 255, 255, 255, 255 } end
													  
		labelText = display.newText( newText, 0, 0, font, size )
		labelText:setTextColor( textColor[1], textColor[2], textColor[3], textColor[4] )
		button:insert( labelText, true )                
		self.text = labelText
		
		labelText.xScale = sizeDivide; labelText.yScale = sizeDivide
    end
        
    if params.text then
         button:setText( params.text )
    end
	
	return button
end

M.newToggle = newToggle
return M