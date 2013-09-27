--[[ $Id$
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local xap = require("hahlib.xap.init")
local bsc = require("hahlib.interface.bsc")

xap_server = xap.TCPServer{host="home.dbzoo.com"}

if not xap_server:start() then
   print "Startup failure" 
   return
end

for i in pairs{1,2,3,4} do
	local relay = bsc.newToggle{    
		default = "glossy_button_red.png",    
		on = "glossy_button_green.png",    
		x = 80*i,
		y = 0,
		size=18,
		text = "Relay "..i,
		xapSource = "dbzoo.livebox.Controller:relay."..i
	}
end
