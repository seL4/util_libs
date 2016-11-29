# Targets
TARGETS := libpicotcp.a

# Source files required to build the target
CFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/stack/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_tcp.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_socket_tcp.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_ipv4.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_arp.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_udp.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_socket_udp.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_icmp4.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_dev_mock.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_dev_dhcp_common.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_dev_dhcp_client.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_igmp.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_dns_client.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/modules/pico_dns_common.c))

CORE_OBJ= stack/pico_stack.o \
          stack/pico_frame.o \
          stack/pico_device.o \
          stack/pico_protocol.o \
          stack/pico_socket.o \
          stack/pico_socket_multicast.o \
          stack/pico_tree.o \
          stack/pico_md5.o



# Header files/directories this library provides
HDRFILES := $(wildcard $(SOURCE_DIR)/include/*)
HDRFILES += $(wildcard $(SOURCE_DIR)/include/arch/*)
HDRFILES += $(wildcard $(SOURCE_DIR)/modules/*.h)

include $(SEL4_COMMON)/common.mk
