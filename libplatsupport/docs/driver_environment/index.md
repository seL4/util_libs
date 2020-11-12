<!--
  Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause

  @TAG(DATA61_BSD)
-->

# Driver environment

The driver environment provides a series of interfaces for writing hardware
device drivers. These interfaces consist of hardware access interfaces and
driver interfaces.

## Usage

The goal of this environment is to make it possible to implement hardware
device drivers that can be used in various projects that may have different
system environments.  By providing a valid implementation of this driver
environment, different projects should be able to reuse compatible device
drivers in other projects without requiring a large porting effort.
Additionally, by relying on well-defined driver interfaces, new drivers can be
implemented that depend on other without compatibility issues. 

Most of the interfaces described below can be accessed via a handle to the
`ps_io_ops_t` interface.  When a system environment is initialised, functions
required for implementation of drivers will be initialised and registered to
the `ps_io_ops_t` interface via sub-interfaces.  There is a core set of
interfaces that provide functionality needed for implementing basic drivers,
and a possibility for additional interfaces to be registered in order to extend
the driver environment for building more complicated drivers. This allows
drivers that depend on other drivers to be created, as they can find interfaces
to other driver systems inside `ps_io_ops_t`.

The following core interfaces provide access to hardware resources and are
expected to be available for any driver to use:

- [`ps_dma_man_t`][1]: Direct Memory Access (DMA) management interface for
  allocating and managing memory that is used in DMA transactions with
  hardware.
- [`ps_io_mapper_t`][2]: Memory Mapped I/O (MMIO) interface for creating
  virtual memory mappings to device registers.
- [`ps_io_port_ops_t`][3]: Architecture-specific I/O operations interface to
  facilitate any hardware I/O that is not possible through MMIO.
- [`ps_irq_ops_t`][4] Hardware interrupt handling interface for registering
  interrupt handlers against hardware interrupts.
- [`ps_malloc_ops_t`][5]: Memory allocation interface for performing anonymous
  memory allocation.
- [`ps_io_fdt_t`][6]: Device tree interface for providing access to a flattened
  device tree (FDT) used to query platform information.
- [`ps_interface_registration_ops_t`][7]: Interface for allowing the
  registration and locating of specific driver interfaces.

These interfaces should be sufficient for creating a typical device driver
where other devices aren't required to enable the device to be used.

[1]:ps_dma_man_t.md
[2]:ps_io_mapper_t.md
[3]:ps_io_port_ops_t.md
[4]:ps_irq_ops_t.md
[5]:ps_malloc_ops_t.md
[6]:fdt.md
[7]:ps_interface_registration_ops_t.md

Many drivers require another device to perform some sort of operation to allow
their device to function correctly. One example is an UART driver that needs to
communicate with a clock driver to configure its input frequency. Access to
other drivers are going to depend on the system configuration as many systems
will try to isolate drivers from each other for better fault-tolerance and
access control policies. Therefore many driver interfaces would not exist in
certain software components.  The `ps_interface_registration_ops_t` interface
is used for co-ordinating variable interface availability for a particular
driver environment. If a driver has a dependency on another driver, then it can
try to find the interface instance within the list of registered driver
interfaces. If the environment is configured correctly, the driver interface
should be available. It is also expected that once a driver has initialised it
registers its own interface in the `ps_interface_registration_ops_t` for future
users to find it.

The initialisation process refers to how a driver is initialised. The driver's entry point
or constructor is how the driver environment passes the initial `ps_io_ops_t` reference
and any initial configuration for the driver to know how to initialise itself.  This entry point
function can be registered under a compatibility string in a known location so that the
driver environment can call it if it wishes to instantiate a driver for a particular device.

```c
int imx6_uart_init(ps_io_ops_t *io_ops, const char* device_path)
```

In the above function, a i.MX6 UART driver can be created by calling
`imx6_uart_init` with a `ps_io_ops_t` and a string to a UART device in the FDT.
These initial variables are enough for the driver to locate the device node in
the FDT, create MMIO mappings, register an interrupt handler, configure the
input clock frequency, take the device out of reset before initialising the
device registers, and finally registering a serial driver interface with
`ps_io_ops_t`.

Driver configuration is usually provided via properties in its device node in
the FDT. This allows a driver to define its own configuration values that it
will understand.  The only initial configuration required would be to specify
which device in the device tree that the driver should be configuring. When the
driver finishes initialising and registers an initialised driver interface, it
can provide configuration information to any future users via setting key-value
pair attributes. This is intended to be a way to provide any configuration
information that is 'outside' of the interface that is being registered.  This
is how multiple interfaces of the same type can be registered while still
allowing the eventual user to make an informed choice about which interface
instance to use.

In order to ensure re-usability of drivers in many different operating system
environments that may have different software execution models, we require that
all driver environments serialise any calls to driver functions essentially
treating anything that `ps_io_ops_t` has been given to as a critical section.
This means that drivers should also be written with this assumption in mind and
not perform operations that expect to block for indefinite amounts of time.
Most devices tend to be designed in a way to allow drivers to be designed as
event handlers that maintain a state machine and perform fairly short running
operations in response to new events that may be generated by either a hardware
event, or a call from a user. This is also why there are no interfaces for
creating additional threads of control.  Mechanisms to support concurrency may
be added in the future but are currently out of scope of this driver
environment.

Drivers can assume the existence of some sort of standard C library existing
that they can link against.  However, they should be careful to only use
functions that don't violate the assumptions of this driver environment.
Additional libraries are also able to be linked for implementations of data
structures a driver needs to operate correctly. In the future the available
standard library functions may be more restricted.

Access control relates to how certain operations may be restricted or allowed
depending on some sort of contextual information. Access control policies can
be implemented and enforced by any interface implementation. A driver should
generally trust any interface that it is calling but not necessarily trust
things calling it. Generally it is assumed that all software executing in the
same protection domain trusts itself. This assumption means that drivers being
called across a local interface will usually trust the caller. (This doesn't
mean that it should expect all calls to be correct). A driver should expect
that any resources that it doesn't have access to will have strong access
control mechanisms that prevent it from being able to access them, usually
implemented with hardware mechanisms, and that any access control policies that
a driver wants to implement will be able to be reinforced by hardware
mechanisms also.

Fault tolerance refers to how a system will respond due to errors that occur
while it is in operation.  Strong component isolation is expected to provide
most of the fault tolerance in a system and as such some drivers may not be
required to be implemented to a high degree of reliability if they aren't
expected to be used in any critical pathways. This is in contrast to systems
without strong component isolation where a crashing driver could result in the
entire system crashing. This doesn't mean that a driver should abort if it gets
unexpected inputs. It is encouraged that all drivers at least return an error
response and consider logging an error in situations where they encounter
unrecoverable situations.

Remote drivers are a name given to drivers that operate in a different software
component from their users.  This means that driver requests cannot be
performed via a simple function call and some sort of remote communication
mechanism is required. Remote drivers are able to more effectively implement
access control policies and distrust their users as they have an isolation
boundary protecting them from any attempted malicious actions by their users. A
remote driver may have a different interface type than if the driver was in the
same software component as it is not always possible to transparently move
software into a different execution domain. A driver that depends on other
device drivers that may be remote may need to be able to handle interracting
with the other driver using different interfaces.  Implementing remote drivers
is out of scope of this current section as providing operating system
independent interfaces for implementing communication with remote components is
out of scope of this environment at the moment.  Remote drivers can still be
accessed via interfaces that are registered with
`ps_interface_registration_ops_t`, only the interface implementations are out
of scope.

As the goal of this driver environment is to make reusing device drivers across
projects easier, mechanisms will likely be needed for handling compatibility
issues as the common interfaces are evolved. Given that this framework is
currently considered under development, there isn't expected to be many
mechanisms for ensuring compatibility. We currently expect version control
software to be used to synchronise sources, and that binary distribution
of artifacts is unsupported. Informal compatibility mechanisms such as
interface and CHANGES documentation and more formal mechanisms such as typed
interfaces should be used to provide some form of compatibility assurance.

## Implementation details

This driver environment is designed with the intention to support both dynamic
and static implementations.  A static environment refers to an environment
where the topology of software and hardware components is unchanging after
intitialisation and resource consumption behavior is known ahead of time. In
static environments, many of the interface implementations already expect to
give specific resources to certain components and also don't expect to have to
destroy instances in response to changing environment.  Dynamic environments
should also be supported and the interfaces listed above should support dynamic
implementations such as on-demand memory mapping or destruction of entire
components. Supporting static environments is currently a higher priority than
dynamic environments, so there may be areas where mechanisms for dynamic
implementations are hindered based on static assumptions. This is an area where
interfaces should evolve to be more accomodating to dynamic implementations.

This driver environment is also designed with the assumption that drivers don't
all exist in the same software protection domain. This prevents drivers from
being able to informally access other resources that aren't explicitly
available as they may be located in different protection domains. Additionally,
certain interfaces may be designed with certain trade-offs made in favour of
remote implementations.  These special cases explained just now may indicate
that a different interface is needed that is more specialised to a particular
use case.

## Potential future changes

Driver initialisation starts with a driver function call that starts the
initialisation. There are not currently a wide range of protocols in place for a
driver to make itself discoverable to an environment.  The current mechanisms
either require the driver to provide a unique symbol name that the environment
somehow knows to call, or by placing the function in a unique global variable
with some metadata that is then added to a special linker section that the
driver environment can perform searches on. As the amount of drivers that are
available grows, a wider range of driver discovery mechanisms may be provided.
This would then require additional support to be added to each driver if it
wanted to take advantage of the additional mechanisms. 

As mentioned in the usage section, concurrency is not supported and there are
no mechanisms for creating additional execution contexts. This functionality
may be added in the future once more use-cases are available to enable a
satisfactory design and these mechanisms become more prioritised.

The core set of library functions available could become more well defined in
the future. This may be by either making it possible for drivers to define
their own standard library, or by developing a smaller, libcore that is usable
in all driver environments.  Each environment likely wants to control its own
standard library and so it may be difficult to take a strong policy choice
here.
