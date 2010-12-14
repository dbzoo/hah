--[[ 
Alias interpreter

Use to perform actions based on xAP alias class messages emitted by
both xap-twitter and xap-googlecal
--]]

module(...,package.seeall)

require("xap")
require("string")
rex = require("rex_posix")

info={
   version="1.0", description="Alias Interpreter"
}

pat={
    [rex.new("(relay|rf) ([1-4]) (on|off)")]=function(m) rfRelayCmd(m) end,
    [rex.new("tweet (.*)")]=function(m) tweet(unpack(m)) end
}

function sendBscCmd(target,body)
  xap.sendShort(string.format([[xap-header
{
target=%s
class=xAPBSC.cmd
}
output.state.1
{
id=*
%s
}]], target, body))
end

function tweet(msg)
  sendBscCmd("dbzoo.livebox.Twitter","text="..msg)
end

function rfRelaycmd(t)
  local addr1,addr2,state = unpack(t)
  sendBscCmd(string.format("dbzoo.livebox.Controller:%s.%s",addr1,addr2),"state="..state)
end

function aliasEngine()
  local alias = xap.getValue("command","text")
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
  f = xap.Filter()
  f:add("xap-header","class","alias")
  f:add("command","text",xap.FILTER_ANY)
  f:callback(aliasEngine)
end
