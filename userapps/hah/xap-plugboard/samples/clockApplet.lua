--[[
   Update the LCD every 5 seconds with the current time
--]]

module(...,package.seeall)

require("xap")
require("xap.bsc")

info = {version="0.04", description="HAH LCD Clock"}

function init()
	xap.Timer(doclock, 5):start()
end

function doclock()
  	bsc.sendText("dbzoo.livebox.Controller:lcd", os.date("%H:%M:%S"))
end
