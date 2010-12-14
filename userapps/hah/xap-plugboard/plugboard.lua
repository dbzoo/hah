--[[
   $Id$
   Bootstrapper to load the applets.
--]]
require "xap"

local spec = [[
  Various flags and option types
    -i,--interface  (default eth0)   Network interface
    -d,--debug      (default 0)      xAP debug range 0..7
    -h,--help
    <appletdir>     (default /etc/plugboard)
]]
local args = require("pl.lapp")(spec)
require "pl"

function writeif(x)
  if x then io.write(x," ") end
end

function loadApplet(file)
      io.write(string.format("Loading %-40s",file))
      local appletName = path.basename(file):sub(1,-5)
      local applet = require(appletName)
      if applet.info then
         io.write("[ ")
	 writeif(applet.info.description)
         io.write(']')
      end
      io.write('\n')
      applet.init()
end

-- MAIN
if args.help then
      print(spec)
      exit()
end
if args.debug then
      xap.setLoglevel(args.debug)
end

app.require_here()
xap.init("dbzoo.livebox.Plugboard","FF00D800",args.interface)
tablex.foreach(dir.getfiles(args.appletdir,"*Applet.lua"), loadApplet)
print("Running...")
xap.process()
