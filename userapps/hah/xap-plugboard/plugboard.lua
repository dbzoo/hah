--[[ $Id$
   
   Bootstrapper to load the applets.
--]]

require("xap")
require("lfs")

-- For script backward compatibilty

function register_thing(thing, filter, func)
  f = xap.Filter()
  f:add("xap-header","source",filter)
  f:callback(_G[func])
end

function register_source(filter, func)
  register_thing("source", filter, func)
end

function register_target(filter, func)
  register_thing("target", filter, func)
end

function register_class(filter, func)
  register_thing("class", filter, func)
end

xAPMsg={}
xAPMsg.mt={__index = function(t, k) return xap.getValue("xap-header",k) end}
setmetatable(xAPMsg, xAPMsg.mt)

function xapmsg_getvalue(section, value)
  return xap.getValue(section, value)
end

function xap_send(msg)
  xap.send(xap.fillShort(msg))
end

oldTimers={}

function start_timer(name, seconds, func)
  oldTimers[name] = xap.Timer(_G[func], seconds)
end

function stop_timer(name)
  oldTimers[name]:stop()
end

-- Plugboard

function writeif(x)
  if x then io.write(x," ") end
end

function loadApplet(file)
      io.write(string.format("Loading %-40s",file))
      file = file:sub(1,-5) -- remove .lua extension
      local applet = require(file)
      if applet.info then
         io.write("[ ")
	 writeif(applet.info.description)
         io.write(']')
      end
      io.write('\n')
      applet.init()
end

function loadAppletDir(path)
  for file in lfs.dir(path) do
    if file:sub(-10) == "Applet.lua" then
      loadApplet(file)
    end
  end
end

-- MAIN
if arg then
    appletdir = arg[1] or "/etc/plugboard"
    nic = arg[2] or "br0"
end

package.path = package.path..";"..appletdir.."/?.lua"

xap.init("dbzoo.livebox.Plugboard","FF00D800", nic)
loadAppletDir(appletdir)
print("Running...")
xap.process()
