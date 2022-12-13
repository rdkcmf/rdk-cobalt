<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.cobalt-wsrpc_API"></a>
# cobalt-wsrpc API

**Version: 0.0.1**

### Table of Contents

- [Description](#head.Description)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Description"></a>
# Description

cobalt-wsrpc JSON RPC 2.0 interface.

<a name="head.Methods"></a>
# Methods

The following methods are provided by the org.rdk.cobalt interface:

| Method | Description |
| :-------- | :-------- |
| [stop](#method.stop) | Stops cobalt |
| [suspend](#method.suspend) | Suspends cobalt |
| [resume](#method.resume) | Resumes cobalt |
| [getState](#method.getState) | Gets cobalt state |
| [getUrl](#method.getUrl) | Gets cobalt url |
| [deepLink](#method.deepLink) | Sets cobalt deeplink |
| [register](#method.register) | Registers event listener |
| [unregister](#method.unregister) | Registers event listener |
| [getListeners](#method.getListeners) | Returns information about registered listeners |


<a name="method.stop"></a>
## *stop <sup>method</sup>*

Stops cobalt.

Also see: [StateEvent](#event.StateEvent)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | null | Always null |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.stop",
    "params": null
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": null
}
```

<a name="method.suspend"></a>
## *suspend <sup>method</sup>*

Suspends cobalt.

Also see: [StateEvent](#event.StateEvent)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | null | Always null |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.suspend",
    "params": null
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": null
}
```

<a name="method.resume"></a>
## *resume <sup>method</sup>*

Resumes cobalt.

Also see: [StateEvent](#event.StateEvent)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | null | Always null |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.resume",
    "params": null
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": null
}
```

<a name="method.getState"></a>
## *getState <sup>method</sup>*

Gets cobalt state.

Also see: [StateEvent](#event.StateEvent)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | null | Always null |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | State Info |
| result.pid | integer | Pid of cobalt |
| result.state | integer | State of cobalt. 0 - started, 1- stopped, 2 - suspended (background), 99 - unknown |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.getState",
    "params": null
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "pid": 1001,
        "state": 0
    }
}
```

<a name="method.getUrl"></a>
## *getUrl <sup>method</sup>*

Gets cobalt url.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | null | Always null |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Url Info |
| result.url | string | Url passed to cobalt. Can be empty string when default is used |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.getUrl",
    "params": null
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "url": "https://www.youtube.com/tv"
    }
}
```

<a name="method.deepLink"></a>
## *deepLink <sup>method</sup>*

Sets cobalt deeplink.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | Deeplink to set |
| params.data | string | Deeplink to pass to cobalt |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.deepLink",
    "params": {
        "data": ""
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": null
}
```

<a name="method.register"></a>
## *register <sup>method</sup>*

Registers event listener.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.event | string | Event type (must be one of the following: *StateEvent*) |
| params.id | string | Id of the listener |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | integer | Value 0 on success |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| -32602 | ```INVALID_PARAMS: Invalid method parameters (invalid name and/or type) recognised: Wrong parameters``` |  |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.register",
    "params": {
        "event": "StateEvent",
        "id": "events.1"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 0
}
```

<a name="method.unregister"></a>
## *unregister <sup>method</sup>*

Registers event listener.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.event | string | <sup>*(optional)*</sup> Event type (must be one of the following: *StateEvent*) |
| params.id | string | Id of the listener |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | integer | Value 0 on success |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| -32602 | ```INVALID_PARAMS: Invalid method parameters (invalid name and/or type) recognised: Wrong parameters``` |  |
| -1 | ```Registration info not found``` |  |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.unregister",
    "params": {
        "event": "StateEvent",
        "id": "events.1"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 0
}
```

<a name="method.getListeners"></a>
## *getListeners <sup>method</sup>*

Returns information about registered listeners.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.listeners | array |  |
| result.listeners[#] | object | Listener Info |
| result.listeners[#]?.event | string | <sup>*(optional)*</sup> Event type (must be one of the following: *StateEvent*) |
| result.listeners[#]?.id | string | <sup>*(optional)*</sup> Id of the listener |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "org.rdk.cobalt.1.getListeners"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "listeners": [
            {
                "event": "StateEvent",
                "id": "events.1"
            }
        ]
    }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. 

The following events are provided by the org.rdk.cobalt interface:

| Event | Description |
| :-------- | :-------- |
| [StateEvent](#event.StateEvent) | Cobalt state changed event |


<a name="event.StateEvent"></a>
## *StateEvent <sup>event</sup>*

Cobalt state changed event.

### Description

Notification of changed state of cobalt

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.pid | integer | Pid of cobalt |
| params.state | integer | State of cobalt. 0 - started, 1- stopped, 2 - suspended (background), 99 - unknown |
| params.code | integer | Exit code of cobalt |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.StateEvent",
    "params": {
        "pid": 1001,
        "state": 0,
        "code": 0
    }
}
```

