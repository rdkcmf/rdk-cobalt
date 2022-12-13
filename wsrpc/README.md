# cobalt-wsrpc
Cobalt starter application that starts cobalt and exposes a websocket RPC interface for minimal control.

## Running cobalt-wsrpc
All parameters passed to cobalt-wsrpc are passed to cobalt main. To start for example with youtube kids:
> ./cobalt-wsrpc --url=https://www.youtube.com/tv/kids

By default the websocket server will listen on TCP port **10111**. Set environment var "COBALT_WS_PORT" to change that default port.



## JSON websocket interface
JSON interface schema and docs: see ./docs/ subdir. 
### example: Register and receive events
> wscat -c ws://127.0.0.1:10111/jsonrpc  -x '{"jsonrpc":"2.0","id":1,"method":"org.rdk.cobalt.1.register", "params": { "event": "StateEvent", "id": "events.1"  } }' -w 1000

### example: Suspend/Resume/Stop cobalt
> wscat -c ws://127.0.0.1:10111/jsonrpc  -x '{"jsonrpc":"2.0","id":1,"method":"org.rdk.cobalt.1.suspend" }
> 
> wscat -c ws://127.0.0.1:10111/jsonrpc  -x '{"jsonrpc":"2.0","id":1,"method":"org.rdk.cobalt.1.resume" }
> 
> wscat -c ws://127.0.0.1:10111/jsonrpc  -x '{"jsonrpc":"2.0","id":1,"method":"org.rdk.cobalt.1.stop" }
> 