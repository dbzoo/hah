--[[ 
    JeeNode to xAP Endpoint mapping
--]]

--_DEBUG=1
module(...,package.seeall)

monitor = require("xap.jeenode").monitor
RoomNode = require("xap.roomnode")
OutputNode = require("xap.outputnode")
IRNode = require("xap.irnode")
BoilerNode = require("xap.boilernode")
BlueNode = require("xap.bluenode")

info={
   version="4.0", description="JeeNode"
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
   [2] = RoomNode{instance="jeenode:attic", endpoints={temp=1,light=1}, ttl=360},
   [3] = RoomNode{instance="jeenode:basement", endpoints={temp=1,lobat=1}, ttl=900},
   [4] = OutputNode{instance="jeenode:bedroom",endpoints={p1="light",p2="heater",p3="amp",p4=0}},
   [5] = IRNode{instance="jeenode:ir"},
   [6] = BoilerNode{instance="jeenode:boiler", endpoints={temp=1}, ttl=360},
   [7] = BlueNode{instance="bluetooth:presence"}
}

function init()
   monitor(jeemon, nodes)
end
