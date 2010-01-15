pat={
    [rex.new("(relay|rf) ([1-4]) (on|off)")]=function(f) cmd(f) end,
    [rex.new("tweet (.*)")]=function(f) tweet(f) end,
    ["reboot"]=function() os.execute("/sbin/reboot") end
}

function init()
  register_class("alias","doalias")
end

function cmd(t)
   key,subkey,state = unpack(t)
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

function tweet(t)
   msg = unpack(t)
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

function doalias()
  local alias = xapmsg_getvalue("command","text")

  for r,f in pairs(pat) do
	if type(r) == "string" then
           if r == alias then
	      f()
           end
        else
	  p={r:match(alias)}
	  if #p > 0 then 
              f(p)
          end
        end
  end
end
