-- Keep relay 2 in sync with relay 1
-- Effectively bind these two together.

function init()
  register_source("dbzoo.livebox.Controller:relay.1", "dorelay")
end

function cmd(relay)
   local state = xapmsg_getvalue("output.state","state")
   xap_send(string.format("xap-header\
{\
target=dbzoo.livebox.Controller:relay.%s\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
state=%s\
}", relay, state))
end

function dorelay()
  if xAPMsg.class == "xAPBSC.event" then
	cmd(2)
  end
end
