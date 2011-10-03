--[[ 
    JeeNode to xAP Endpoint mapping
--]]

--_DEBUG=1
module(...,package.seeall)

monitor = require("xap.jeenode").monitor
RoomNode = require("xap.roomnode").RoomNode
OutputNode = require("xap.outputnode").OutputNode
IRNode = require("xap.irnode").IRNode

info={
   version="2.0", description="JeeNode"
}

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
   [2] = RoomNode{base="dbzoo.livebox.jeenode:attic", endpoints={temp=1,light=1}, ttl=360},
   [3] = RoomNode{base="dbzoo.livebox.jeenode:basement", endpoints={temp=1,lobat=1}, ttl=900},
   [4] = OutputNode{base="dbzoo.livebox.jeenode:bedroom",endpoints={p1="light",p2="heater",p3="amp",p4=0}},
   [5] = IRNode{base="dbzoo.livebox.jeenode:ir"},
}

function init()
   monitor(jeemon, nodes)
end
