-- Create a google calendar event to audit a state change

function init()
  register_source('dbzoo.livebox.Controller:relay.1', "dosource")
end

function gcal(title,description)
   xap_send(string.format("xap-header\
{\
target=dbzoo.livebox.GoogleCal\
class=google.calendar\
}\
event\
{\
title=%s\
start=now\
description=%s\
}", title, description))
end
 
function dosource()
  if xAPMsg.class == "xAPBSC.event" then
	if xapmsg_getvalue("output.state","state") == "off" then
          gcal('Relay 1 is off','LUA monitored event')
        end
  end
end
