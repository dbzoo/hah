--[[
   Twitter support module
--]]

-- Send a tweet via the xap-twitter service
function tweet(msg)
   xap.send(string.format("xap-header\
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
