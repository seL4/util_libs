<!--
  Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause

  @TAG(DATA61_BSD)
-->

# Interface registration interface

`ps_interface_registration_ops_t` is an interface for registering additional
interfaces within a `ps_io_ops_t` instance. These interfaces may then be used
by future users of the `ps_io_ops_t` instance for handling more complex
operations than those provided in the statically defined interfaces.

<https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/interface_registration.h>

## Usage

This interface provides a way for registering interface instances, and for
locating such registered interface instances.

```c
int ps_interface_register(ps_interface_registration_ops_t *interface_registration_ops, ps_interface_type_t interface_type, void *interface_instance, char **properties)
int ps_interface_unregister(ps_interface_registration_ops_t *interface_registration_ops, ps_interface_type_t interface_type, void *interface_instance)
```

Registering an interface involves providing an initialised interface,
`interface_instance`, of type `interface_type`. A `ps_interface_type_t` is an
enumeration of interfaces type IDs.  Each value of this enumeration refers to a
specific interface and indicates how the type of `interface_instance` is
interpreted. A user of the interface would need to know about the interface
type referred to in order to correctly interpret the interface.  The
`properties` argument given to `ps_interface_register` contains a list of
`key=value` strings that can be used to identify an interface instance when it
is being selected by a user.

An interface can be unregistered by calling `ps_interface_unregister` with the
same arguments.  An interface may be unregistered if it only supports a single
user and the user of the interface unregisters it while it is being used. There
is no way for an interface that is unregistered to be taken away from any
existing users of it.  This would need to be handled by the interface instance
itself.

```c
int ps_interface_find(ps_interface_registration_ops_t *interface_registration_ops, ps_interface_type_t interface_type, ps_interface_search_handler_t handler, void *handler_data)
int (*ps_interface_search_handler_t)(void *handler_data, void *interface_instance, char **properties)
```

When searching for an interface, `ps_interface_find` is given an
`interface_type` and a handler to be called for each interface instance it has
of a given type.  The handler will be called with the `interface_instance` and
`properties` parameters that were provided when the interface was registered.
This allows the caller of `ps_interface_find` to select a valid interface
instance.

## Implementation details

The contents of `properties` that are given to `ps_interface_register` need to
be copied by the implementation so that it has its own record. This allows the
caller to free the `properties` once the interface has been registered.  When
`ps_interface_unregister` is called, the implementation is allowed to free its
copy of properties. If an interface instance is destroyed, it is up to that
interface instance to handle cleaning up its resources, including the memory
that `void *interface_instance` refers to.

`ps_interface_type_t` represents a global namespace for different interface
types. Interface types will need to coordinate to ensure that multiple
interface types do not end up with the same enum value.

