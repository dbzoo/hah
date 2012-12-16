--[[
   XAP library testing
--]]

module(...,package.seeall)

require("xap")
require("pl.test")
require("pl.pretty")
require("pl.stringx").import()

info = {version="0.01", description="Validate LUA xap library"}

function test10_Frame()
   local msg=[[
xap-header
{
v=13
hop=1
uid=FF.9192:00
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}
output.state
{
state=off
text=hello world
}]]

  f = xap.Frame(msg)
  test.asserteq(f:getType(), xap.MSG_ORDINARY)
  test.asserteq(f:getValue("xap-header","class"),"xAPBSC.event")
  test.asserteq(f:isValue("output.state","state","off"), true)
  test.asserteq(f:isValue("output.state","text","hello world"), true)
end

function test15_EmptyFrame()
   local msg=[[
xap-header
{
v=13
hop=1
uid=FF.9192:00
class=xAPBSC.query
source=dbzoo.lua.test
}
query
{
}]]

  f = xap.Frame(msg)
  test.asserteq(f:getValue("query","x"), nil)
  test.asserteq(f:getValue("query",nil), xap.FILTER_ANY)
end

function test20_StartEndMessageSpaces()
   local msg="xap-header\
	{\
	class=xAPBSC.cmd\
	target=dbzoo.livebox.Controller:lcd\
	}\
	output.state\
	{\
	id=*\
	text=hello world   \
	}\
	"
   frame = xap.Frame(msg)
   test.asserteq("hello world",frame:getValue("output.state","text"))
   test.asserteq("xAPBSC.cmd",frame:getValue("xap-header","class"))
end

function test30_ExpandShortMsg()
   local msg=[[
xap-header
{
class=xAPBSC.cmd
target=dbzoo.livebox.Controller:lcd
hop=2
}
output.state
{
id=*
text=hello
}]]

   xap.init("dbzoo.lua.test","FF00123400")
   f = xap.Frame(xap.expandShortMsg(msg))
   test.asserteq(f["xap-header"].hop, "2")
   test.asserteq(f["xap-header"].v, "12")
   test.asserteq(f["xap-header"].uid, "FF00123400")
   test.asserteq(f["xap-header"].source, "dbzoo.lua.test")
end

function test32_ExpandShortMsgXapHbeat()
   local msg=[[
xap-hbeat
{
interval=60
}
]]

   xap.init("dbzoo.lua.test","FF00123400")
   f = xap.Frame(xap.expandShortMsg(msg,"xap-hbeat"))
   test.asserteq(f["xap-hbeat"].hop, "1")
   test.asserteq(f["xap-hbeat"].v, "12")
   test.asserteq(f["xap-hbeat"].uid, "FF00123400")
   test.asserteq(f["xap-hbeat"].source, "dbzoo.lua.test")
   test.asserteq(f["xap-hbeat"].interval, "60")
end

function test40_Filter()
   local ok = false
   local frame=xap.Frame[[
xap-header
{
v=13
hop=1
uid=FF.9192:00
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}
output.state
{
state=OFF
text=92220
}]]

  filter = xap.Filter()
  filter:add("xap-header","class","xapbsc.event")
  filter:add("xap-header","source","ukusa.*.mx12usa")
  filter:add("output.state","state","off")
  filter:callback(function() ok=true end)
  filter:dispatch(frame)
  test.asserteq(ok, true)
end

function test41_FilterTableConstructor()
   local ok = false
   local filter = xap.Filter {["xap-header"]={
				 class="*.event",
				 source="ukusa.>"
			      },
			      ["output.state"]={
				 state="off"
			      }
			   }
   filter:callback(function() ok=true end)
   filter:dispatch(xap.Frame[[
xap-header
{
v=13
hop=1
uid=FF.9192:00
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}
output.state
{
state=OFF
text=92220
}]])
  test.asserteq(ok, true)
end

function test42_FilterTableNeg()
   local ok = true
   local filter = xap.Filter{["xap-header"]={class="*.cmd"}}
   filter:callback(function() ok=false end)
   filter:dispatch(xap.Frame[[
xap-header
{
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}]])
  test.asserteq(ok, true)
end

function test43_FilterTableANY()
   local ok = false
   -- Callback if "class" has any value
   local filter = xap.Filter{["xap-header"]={class=xap.FILTER_ANY}}
   filter:callback(function() ok=true end)
   filter:dispatch(xap.Frame[[
xap-header
{
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}]])
  test.asserteq(ok, true)
end

function test44_FilterTableANYNeg()
   local ok = false
   -- Callback if "absent" has any value
   local filter = xap.Filter{["xap-header"]={absent=xap.FILTER_ANY}}
   filter:callback(function() ok=true end)
   filter:dispatch(xap.Frame[[
xap-header
{
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}]])
  test.asserteq(ok, false)
end

function test45_FilterTableABSENT()
   local ok = false
   -- Callback when "absent" is not in the message.
   local filter = xap.Filter{["xap-header"]={absent=xap.FILTER_ABSENT}}
   filter:callback(function() ok=true end)
   filter:dispatch(xap.Frame[[
xap-header
{
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}]])
  test.asserteq(ok, true)
end

function test46_FilterTableABSENTNeg()
   local ok = false
   -- Callback when "class" is not in the message.
   local filter = xap.Filter{["xap-header"]={class=xap.FILTER_ABSENT}}
   filter:callback(function() ok=true end)
   filter:dispatch(xap.Frame[[
xap-header
{
class=xAPBSC.event
source=UKUSA.MOBOTIX.MX12USA
}]])
  test.asserteq(ok, false)
end

function pairsByKeys (t, f)
   local a = {}
   for n in pairs(t) do table.insert(a, n) end
   table.sort(a, f)
   local i = 0
   local iter = function ()
		   i = i + 1
		   if a[i] == nil then return nil
		   else return a[i], t[a[i]]
		   end
      end
   return iter
end

function init()
   for k,v in pairsByKeys(_M) do
      if k:find("^test") then
	 io.write("...."..string.ljust(k,40).."    ")
	 v()
	 print("[ok]")
      end
   end
end

