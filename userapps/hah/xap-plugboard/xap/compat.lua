--[[
   xAP library
   Copyright (c) Brett England, 2011
   For script backward compatibilty

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local _G = _G
local utils = require 'pl.utils'

module("compat", package.seeall)

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

function import()
   utils.import(_G.xap.compat, _G)
end
