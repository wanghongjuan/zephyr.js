ZJS API for OCF
==================

* [Introduction](#introduction)
* [Web IDL](#web-idl)
* [OCF Object](#the-ocf-object)
* [OCF API Documentation](#ocf-api-documentation)
* [OCF Server](#ocf-server)
* [Server API Documentation](#server-api-documentation)
* [Server Samples](#server-samples)
* [OCF Client](#ocf-client)
* [Client API Documentation](#client-api-documentation)
* [Client Samples](#client-samples)

Introduction
------------
ZJS provides OCF Server API's which allow communication using the OCF networking
protocol.

Web IDL
-------
This IDL provides an overview of the interfaces for OCF common, OCF Server and
OCF Client; see below for documentation of specific API functions.

The OCF Object
--------------
The OCF object is the top level object containing either OCF Server,
OCF Client, or both, as well as device and platform information.

```javascript
// require returns an OCFObject
// var ocf = require('ocf');

interface OCFObject {
    Server server;         // OCF server object
    Client client;         // OCF client object
    Platform platform;     // OCF platform info
    Device device          // OCF device info
};

dictionary Platform {
    string id;
    string osVersion;
    string model;
    string manufacturerName;
    string manufacturerURL;
    string manufacturerDate;
    string platformVersion;
    string firmwareVersion;
    string supportURL;
}

dictionary Device {
    string uuid;
    string name;
    string dataModels;
    string coreSpecVersion;
}

```
The OCF device and platform objects can be set up after requiring 'ocf'. An
example of this can be found in [OCF Server sample](../samples/OcfServer.js).
The properties are registered to the system (and available during discovery)
once either `OCFServer.registerResource()` or `OCFClient.findResources()`
is called.

OCF API Documentation
---------------------

### OCFObject.setBleAddress
`void setBleAddress(string address);`

Sets the device's BLE MAC address. This function is only defined on
Zephyr boards with BLE capabilities (e.g. Arduino 101).

The `address` parameter should be a MAC address string in the format
`XX:XX:XX:XX:XX:XX` where each character is in HEX format (0-9, A-F).

Note: If the image was built with the `BLE_ADDR` flag, this API has
no effect. Using the `BLE_ADDR` hard codes the supplied address into
the image which cannot be changed.

### OCFObject.start
`void start(void)`

Start the OCF stack (iotivity-constrained). This should be called after all
resources have been registered. Any calls to `registerResource` after `start`
will have no effect.

OCF Server
----------
```javascript
// OCFServer is an EventEmitter
interface Server {
    Promise<OCFResource> register(ResourceInit init);
};

dictionary ResourceInit {
    string resourcePath;      // OCF resource path
    string[] resourceTypes;   // List of resource types
    string[] interfaces;      // List of interfaces for resource types
    boolean discoverable;     // Is resource discoverable
    boolean observable;       // Is resource observable
    boolean secure;           // Is resource security enabled
    boolean slow;             // Is resource a slow reader
    object properties;        // Dictionary of resource properties
};

interface Resource {
    string resourcePath;      // Path for this resource
    object properties;        // Application specific resource properties
};

interface Request {
    OCFResource target;       // Target/destination resource
    OCFResource source;       // Source/origin resource
    Promise<void> respond(object data);
};

// listener for onretrieve event
callback RetrieveCallback = void (Request request, boolean observe);
// listener for onupdate event
callback UpdateCallback = void (Request request);

```

Server API Documentation
-----------------
### Server.register
`Promise<OCFResource> register(ResourceInit init);`

Register a new resource with the server.

The `init` contains the resource initalization information.

Returns a promise which resolves to an `OCFResource`.

### Server.on('retrieve', RetrieveCallback listener)

Register an `onretrieve` listener. This event will be emitted when a remote
client retrieves this servers resource(s).

### Server.on('update', UpdateCallback listener)

Register an `onupdate` listener. This event will be emitted when a remote
client updates this servers resource(s).

### Request.respond
`Promise<void> respond(object data);`

Respond to an OCF `onretrieve` or `onupdate` event.

The `data` parameter should contain object property data for the resource. In
the case of an `onretrieve` event, `data` will be sent back to the client as
the retrieved property data.

Returns a promise which resolves successfully if there was no network error
sending out the data.

Server Samples
-----------
* [OCF Server sample](../samples/OcfServer.js)
* [OCF Sensor Server](../samples/OcfSensorServer.js)

OCF Client
----------
```javascript
// OCFClient is an EventEmitter
interface Client {
    Promise<Resource> findResources(ClientOptions options, optional FoundListener listener);
    Promise<Resource> retrieve(string deviceId, object options);
    Promise<Resource> update(Resource resource);
    Promise<Platform> getPlatformInfo(string deviceId);
    Promise<Device> getDeviceInfo(string deviceId);
};

dictionary ClientOptions {
    string deviceId;
    string resourceType;
    string resourcePath;
}

interface Resource {
    string resourcePath;      // Path for this resource
    object properties;        // Application specific resource properties
};

callback FoundListener = void (ClientResource);
```

Client API Documentation
------------------------
### Client.findResources
`Promise<ClientResource> findResources(ClientOptions options, optional FoundListener listener);`

Find remote resources matching `options` filter.

The `options` parameter should contain a filter of resource options. Only
resources matching these options will be found.

The `listener` parameter is an optional event listener callback. This
callback will be called if a resource is found (`onfound` event).

Returns a promise which resolves with a `ClientResource` object if a resource
was found.

### Client.retrieve
`Promise<Resource> retrieve(string deviceId, object options);`

Retrieve (GET) a remote resource.

The `deviceId` parameter is the device ID of the resource you are retrieving.
This ID must match a resource which has been found with `findResources()`.

The `options` object properties contain flag information for this GET request
e.g. `observable=true`.

Returns a promise which resolves to a `ClientResource` containing the resource
properties.

### Client.update
`Promise<Resource> update(Resource resource);`

Update remote resource properties.

The `resource` parameter should contain a `deviceId` for the resource to
update. The `properties` parameter will be sent to the resource and updated.

Returns a promise which resolves to a resource `Resource` containing the
updated properties.

### Client.getPlatformInfo
`Promise<Platform> getPlatformInfo(string deviceId);`

Get `Platform` information for a resource.

The `deviceId` parameter should be the ID for a resource found with
`findResources()`.

Returns a promise which resolves to a `Platform` containing the platform
information for the resource.

### Client.getDeviceInfo
`Promise<Device> getDeviceInfo(string deviceId);`

Get `Device` information for a resource.

The `deviceId` parameter should be the ID for a resource found with
`findResources()`.

Returns a promise which resolves to a `Device` containing the device
information for the resource.

Client Samples
------------------
* [OCF Client sample](../samples/OcfClient.js)
* [OCF Sensor Client](../samples/OcfSensorClient.js)
