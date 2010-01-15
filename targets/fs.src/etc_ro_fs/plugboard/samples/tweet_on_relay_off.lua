-- Tweet when relay 1 is detected to turn OFF

function init()
  register_source('dbzoo.livebox.Controller:relay.1', "dosource")
end

function tweet(msg)
   xap_send(string.format("xap-header\
{\
target=dbzoo.livebox.Twitter\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
text=%s\
}", msg))
end
 
function dosource()
  if xAPMsg.class == "xAPBSC.event" then
	if xapmsg_getvalue("output.state","state") == "off" then
          tweet('Relay 1 is off')
        end
  end
end
