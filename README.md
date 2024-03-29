# Queue relinquish plugin

This plugin allows you to relinquish delivery of mail when picked up from the active queue.

## Installation

Follow the [instructions](https://docs.halon.io/manual/comp_install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-queue-relinquish
```

### RHEL

```
yum install halon-extras-queue-relinquish
```

## Exported functions

These functions needs to be [imported](https://docs.halon.io/hsl/structures.html#import) from the `extras://queue-relinquish` module path.

### queue_relinquish(fields, ttl [, options])

**Params**

- fields `array` the fields combination as an associative array (**required**)
- ttl `number` the duration in seconds that the fields combination should be enabled for (**required**)
- options `array` 
    - update `boolean` update the `ttl` if the fields combination already exists (default `true`)
    - return `any` the return value that can be retreived from the [Pre-delivery](https://docs.halon.io/hsl/predelivery.html) script using `$arguments["queue"]["plugin"]["return"]`

**Returns**

Nothing is returned.

**Example**

```
import { queue_relinquish } from "extras://queue-relinquish";
queue_relinquish([
    "localip" => $arguments["attempt"]["connection"]["localip"],
    "remotemx" => $arguments["attempt"]["connection"]["remotemx"]
], 60);
```

### queue_relinquish_enabled()

**Returns**

An array of the currently enabled queue relinquish items.

**Example**

```
import { queue_relinquish_enabled } from "extras://queue-relinquish";
queue_relinquish_enabled();
/*
[
    ["localip" => "192.168.0.1", "remotemx" => "mx1.example.com"],
    ["localip" => "192.168.0.1", "remotemx" => "mx2.example.com"]
]
*/
```