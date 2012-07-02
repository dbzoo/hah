--[[ 
    xAP Caching web server

   To pull a variable from the cache construct a URL of the form
      http://IP:8080/<source>/<section>/<key>

   The cache may be examined too
      http://IP:8080/
--]]

module(...,package.seeall)

require("xap")
require("pl.stringx").import()
require("pl.pretty")
url = require("socket.url")

info={
   version="1.5", description="xAP Caching web servlet"
}

local client

local vfs = {
   filter={
      xap.Filter{["xap-header"] = { 
		    source="dbzoo.arduino.temp",
		    class="xAPBSC.event" }
	      },
      xap.Filter{["xap-header"] = { 
		    source="dbzoo.livebox.controller:relay.*",
		    class="xAPBSC.event" }
	      },
      xap.Filter{["xap-header"] = { 
		    source="dbzoo.livebox.controller:relay.*",
		    class="xAPBSC.info" }
	      },
   },
   data={}
}

function sendError( client, status, str )
    message = "HTTP/1.0 " .. status .. " OK\r\n" ;
    message = message .. "Server:  xapCacheWebserverApplet" .. info.version .. "\r\n";
    message = message .. "Content-type: text/html\r\n";
    message = message .. "Connection: close\r\n\r\n" ;
    message = message .. "<html><head><title>Error</title></head>" ;
    message = message .. "<body><h1>Error</h1" ;
    message = message .. "<p>" .. str .. "</p></body></html>" ;
    client:send(message);
    client:close()
 end

function handleVFSroot()
   local msg = "<html><h2>Cached xap-header source keys</h2><ul>\n"
   for k in pairs(vfs.data) do
      msg = msg .. "<li><a href=\"".. url.escape(k) .."\">"..k.."</a></li>\n"
   end
   msg = msg .. "</ul></html>"
   return msg
end

function handleVFSsource(source)
   local msg = "<html><h2>".. source .."</h2>\n"
   local frame = vfs.data[source]
   if frame then
      msg = msg .. "<pre>".. tostring(frame).."</pre>"
      --msg = msg .. "<pre>".. pretty.write(frame).."</pre>"
   else
      msg = msg .. "Not in the cache"
   end
   msg = msg .. "</html>"
   return msg
end

function service(request)
   local source,section,key
   _, _, method, path, major, minor  = request:find("([A-Z]+) (.+) HTTP/(%d).(%d)");
   
   if method ~= "GET" then
      error = "Method not implemented"
      if method == nil then
	 error = error .. "."
      else
	 error = error .. ": ".. url.escape(method)
      end
      sendError(client, 501, error)
      return
   end

   path = url.unescape( path )

   local message = "HTTP/1.0 200 OK\r\n"
   message = message .. "Server: xapCacheWebserverApplet" .. info.version .. "\r\n";
   message = message .. "Content-Type: text/html\r\n"
   message = message .. "Connection: close\r\n\r\n"
   
   _,source,section,key = path:splitv('/')
   
   if source and section and key then
      local frame = vfs.data[source]
      local data = ""
      if frame then
	 data = frame:getValue(section, key) or ""
      end
      message = message .. data
   elseif source and section then
      message = message .. "Unhandled URL: " ..  path
   elseif source then
      message = message .. handleVFSsource(source)
   else
      message = message .. handleVFSroot()
   end

   client:send(message)
   client:close()
end

function updateCache(frame)
   vfs.data[frame["xap-header"].source] = frame
end

function handleConnection(server)
   client = server:accept()
   client:settimeout(60)
   local request, err = client:receive()
   if not err then
      service(request)
   end
end

function init()
   local server = socket.bind("*", 8080)
   xap.Select(handleConnection, server)

   for _,k in pairs(vfs.filter) do
      k:callback(updateCache)
   end
end
