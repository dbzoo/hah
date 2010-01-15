-- Update the LCD every minute with the current time
function init()
  start_timer("t1",60,"doclock")
end

function doclock()
   xap_send(string.format("xap-header\
{\
target=dbzoo.livebox.Controller:lcd\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
text=%s\
}\
"),os.date("%d %b %H:%M"))
  start_timer("t1",60,"doclock")
end
