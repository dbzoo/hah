--[[
   Update the LCD every 5 seconds with the current time
--]]

module(...,package.seeall)

require("xap")

info = {version="0.04", description="HAH LCD Clock"}

function init()
	xap.Timer(doclock, 5):start()
end

function doclock()
	xap.sendShort(string.format([[
xap-header
{
class=xAPBSC.cmd
target=dbzoo.livebox.Controller:lcd
}
output.state
{
id=*
text=%s
}]],os.date("%H:%M:%S")))
end
