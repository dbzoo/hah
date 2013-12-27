--[[ 
    Arduino to xAP Endpoint mapping
    Use in conjunction with the singleLedControl.pde sketch
--]]

module(...,package.seeall)

require("xap")
require("xap.bsc")

info={
   version="2.0", description="Arduino control"
}
port="/dev/ttyUSB0"

function cmd(endpoint)
   xap.sendShort(string.format([[
xap-header
{
class=Serial.Comms
target=dbzoo.livebox.serial
}
Serial.Send
{
port=%s
data=led %s\n
}]], port, endpoint.state))
end

function init()
   local serialSetup = [[
xap-header
{
class=Serial.Comms
target=dbzoo.livebox.serial
}
Serial.Setup
{
      port=%s
      baud=9600
      stop=1
      databits=8
      parity=none
      flow=none
}]]
   xap.sendShort(string.format(serialSetup, port))

   bsc.Endpoint{ instance="arduino:led", 
		 direction=bsc.OUTPUT, 
		 type=bsc.BINARY, 
		 cmdCB=cmd
	      }
end
