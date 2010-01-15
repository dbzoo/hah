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
   -- Using a jump table (A bit ugly but you get the idea)
  local v={
    ["relay 1 on"] = function() cmd("relay",1,"on") end,
    ["relay 1 off"] = function() cmd("relay",1,"off") end,
    ["relay 2 on"] = function() cmd("relay",2,"on") end,
    ["relay 2 off"] = function() cmd("relay",2,"off") end,
    ["relay 3 on"] = function() cmd("relay",3,"on") end,
    ["relay 3 off"] = function() cmd("relay",3,"off") end,
    ["relay 4 on"] = function() cmd("relay",4,"on") end,
    ["relay 4 off"] = function() cmd("relay",4,"off") end,
    ["rf 1 on"] = function() cmd("rf",1,"on") end,
    ["rf 1 off"] = function() cmd("rf",1,"off") end,
    ["rf 2 on"] = function() cmd("rf",2,"on") end,
    ["rf 2 off"] = function() cmd("rf",2,"off") end,
    ["rf 3 on"] = function() cmd("rf",3,"on") end,
    ["rf 3 off"] = function() cmd("rf",3,"off") end,
    ["rf 4 on"] = function() cmd("rf",4,"on") end,
    ["rf 4 off"] = function() cmd("rf",4,"off") end,
  }
  action = v[xapmsg_getvalue("command","text")]
  if action then action() end
end
