-- Auto turn relay 1 off after 10 seconds of the ON event

function init()
  register_source("dbzoo.livebox.Controller:relay.1", "relay_auto_off")
end

function auto_off()
   xap_send("xap-header\
{\
target=dbzoo.livebox.Controller:relay.1\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
state=off\
}\
")
end

function relay_auto_off()
  if xAPMsg.class == "xAPBSC.event" then
	if xapmsg_getvalue("output.state","state") == "on" then
           start_timer("t1",10,"auto_off")
        else
	   -- This will also fire on the XAP AUTO-OFF cmd but 
	   -- that's ok it will have no effect.
           stop_timer("t1")
        end
  end
end
