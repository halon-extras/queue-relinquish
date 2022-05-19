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

### queue_relinquish(fields, ttl [, options])

**Params**

- fields `array` the fields combination as an associative array (**required**)
- ttl `number` the duration in seconds that the fields combination should be enabled for (**required**)
- options `array` 
    - update `boolean` update the `ttl` if the fields combination already exists (default `true`)

**Returns**

Nothing is returned.

**Example**

```
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
queue_relinquish_enabled();
/*
[
    ["localip" => "192.168.0.1", "remotemx" => "mx1.example.com"],
    ["localip" => "192.168.0.1", "remotemx" => "mx2.example.com"]
]
*/
```