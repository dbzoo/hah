function init()
   register_class("alias","doalias")
end

function cmd(key,subkey,state)
   xap_send(string.format("xap-header\
{\
target=dbzoo.livebox.Controller:%s.%s\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
state=%s\
}", key, subkey, state))
end

function doalias()
	alias = xapmsg_getvalue("command","text")
	k,r,s = rex.match(alias,"(relay|rf) ([1-4]) (on|off)")
	if k then cmd(k, r, s) end
end
