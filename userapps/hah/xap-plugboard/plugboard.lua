--[[
   $Id$
   Bootstrapper to load the applets.
--]]
require "xap"
require "pl.config"
require "pl.stringx"
local spec = [[
  Various flags and option types
    -h,--help
     <appletdir> (default /etc/plugboard)
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

function require_path(p)
    if not path.isabs(p) then
        p = path.join(lfs.currentdir(),p)
    end
    if p:sub(-1,-1) ~= path.sep then
        p = p..path.sep
    end
    local lsep = package.path:find '^;' and '' or ';'
    package.path = ('%s?.lua;%s?%sinit.lua%s%s'):format(p,p,path.sep,lsep,package.path)
end

-- MAIN
if args.help then
      print(spec)
      exit()
end

collectgarbage("setpause",150) -- default 200%
local scriptdir = args.appletdir

local ini = config.read("/etc/xap.d/plugboard.ini")
local uid = nil
if ini and ini['plugboard'] then
   uid = ini.plugboard['uid']
   if uid then uid = tostring(uid) end
   if ini.plugboard['scriptdir'] then scriptdir = ini.plugboard['scriptdir'] end
end
if uid == nil then uid="D8" end

require_path(scriptdir)
xap.init{instance="Plugboard",uid="FF"..stringx.rjust(uid,4,'0').."00"}
local files = dir.getfiles(scriptdir,"*Applet.lua")
table.sort(files)
tablex.foreach(files, loadApplet)
print("Running...")
xap.process()
