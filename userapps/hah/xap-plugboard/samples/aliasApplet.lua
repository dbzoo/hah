--[[ 
Alias interpreter

Use to perform actions based on xAP alias class messages emitted by
both xap-twitter and xap-googlecal
--]]

module(...,package.seeall)

require("xap")
require("xap.bsc")
require("string")
rex = require("rex_posix")

info={
   version="1.1", description="Alias Interpreter"
}

pat={
    [rex.new("(relay) ([1-4]) (on|off)")]=function(m) rfRelayCmd(m) end,
    [rex.new("(rf) ([0-9]+) (on|off)")]=function(m) rfRelayCmd(m) end,
    [rex.new("tweet (.*)")]=function(m) tweet(m) end
}

function tweet(m)
  local msg = unpack(m)
  bsc.sendText("dbzoo.livebox.Twitter",msg)
end

function rfRelayCmd(t)
  local addr1,addr2,state = unpack(t)
  bsc.sendState(string.format("dbzoo.livebox.Controller:%s.%s",addr1,addr2),state)
end

function aliasEngine(frame)
  local alias = frame:getValue("command","text")
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

function init()
  local f = xap.Filter()
  f:add("xap-header","class","alias")
  f:add("command","text",xap.FILTER_ANY)
  f:callback(aliasEngine)
end
