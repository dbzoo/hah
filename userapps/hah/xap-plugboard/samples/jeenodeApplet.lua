--[[ 
    JeeNode to xAP Endpoint mapping
--]]

--_DEBUG=1
module(...,package.seeall)

monitor = require("xap.jeenode").monitor
RoomNode = require("xap.roomnode").RoomNode

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
}

function init()
   monitor(jeemon, nodes)
end
