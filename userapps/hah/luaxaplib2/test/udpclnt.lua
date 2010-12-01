-- UDP to xAP client
-- Send data to our gateway

local socket = require("socket")
host = host or "localhost"
port = port or 6666
if arg then
    host = arg[1] or host
    port = arg[2] or port
end
host = socket.dns.toip(host)
udp = assert(socket.udp())
assert(udp:setpeername(host, port))
print("Using remote host '" ..host.. "' and port " .. port .. "...")
print("Enter text to sent to the UDP/xAP gateway, blank line will exit.")
while 1 do
        line = io.read()
        if not line or line == "" then os.exit() end
        assert(udp:send(line))
end
