-- Examining and using cache entries
-- Demonstration of variable persistence across invocations

function init()
  register_source('dbzoo.livebox.Controller:relay.1', "dosource")
  state="?"
end

function recent_cache(source)
   local cache = xapcache_find(source,"","xAPBSC.event")
   -- otherwise fallback to an .info cache entry.
   if cache == nil then
      cache = xapcache_find(source,"","xAPBSC.info")
   end
  return cache
end

function dosource()
  local i
  if xAPMsg.class == "xAPBSC.event" then
	print("Previous state: " .. state)
	state = xapmsg_getvalue("output.state","state")
	print("Relay 1 state: " .. state)
	for i=2,4 do
	   local cache = recent_cache("dbzoo.livebox.Controller:relay." .. i)
	   print("Relay " .. i .. " state: " .. xapcache_getvalue(cache,"output.state","state"))
	end
  end
end
