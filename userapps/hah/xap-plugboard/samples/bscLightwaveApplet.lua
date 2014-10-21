-- Create a BSC level endpoint that acts as a proxy for sending
-- UDP packets to a lightwave wifi controller.

module(...,package.seeall)
require "xap"
require "xap.bsc"

info={version="1.0", description="Lightwave to BSC adapter"}
-- A table of mappings
-- dbzoo.livebox.Plugboard:lw.light -> UDP ON/OFF
-- 
--Example: The command “000,!R1D1F1|” means room 1 ("R1"), device 1 ("D1"), 
-- switch on ("F1").  Conversely “000,!R1D1F0|” would mean switch off.
-- The "000" is a command reference, (so you increment it for each command in turn).


map={
   {source="lw.light1", on="000,!R1D1F1|", off="000,!R1D1F0|"},
}
HOST="lightwavewifihost"

function udpSend(packet)
--   print("sending "..packet)
   local udp = socket.udp()
   udp:sendto(packet, HOST, 9760)
end

function init()
   for _,v in pairs(map) do
      bsc.Endpoint{name=v.source, 
		   direction=bsc.OUTPUT,
		   type=bsc.BINARY, 
		   cmdCB=function(e)
			    udpSend(v[e.state])
			 end
		}
   end
end
