-- Report when any relay changes state

function init()
  register_source('dbzoo.livebox.Controller:relay.*', "dosource")
end

function dosource()
  if xAPMsg.class == "xAPBSC.event" then
	state = xapmsg_getvalue("output.state","state")
	relay = string.gsub(xAPMsg.source,".*(%d)","%1")
        print(string.format('Relay %s is %s', relay, state))
  end
end
