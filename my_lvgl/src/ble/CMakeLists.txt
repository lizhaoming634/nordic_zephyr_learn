target_sources(app PRIVATE 
    ble_comm.c
    ble_proto_time.c
    ble_transport.c
    ble_proto.c
    time_bus.c
)

target_include_directories(app PRIVATE ble)