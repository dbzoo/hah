--[[ 
    JeeNode to xAP Endpoint mapping
--]]

--_DEBUG=1
module(...,package.seeall)

monitor = require("xap.jeenode").monitor
RoomNode = require("xap.roomnode").RoomNode
OutputNode = require("xap.outputnode").OutputNode

info={
   version="1.0", description="JeeNode"
}

-- Make sure the xap-serial daemon is running!
local jeemon={
      port="/dev/ttyUSB0",
      baud=57600,
      stop=1,
      databits=8,
      parity="none",
      flow="none"
}

-- Keyed by NODE ID
local nodes = {
   [2] = RoomNode("dbzoo.livebox.jeenode:attic")
   [3] = OutputNode("dbzoo.livebox.jeenode:attic","light","heater","amp","tv")
   [4] = RoomNode("dbzoo.livebox.jeenode:basement")
}


function init()
   monitor(jeemon, nodes)
end
