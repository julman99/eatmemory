#include <sysmem.h>
#ifdef SYSMEM_MODE_APPLE
    #include <mach/mach.h>
    #include <mach/mach_host.h>
    size_t getTotalSystemMemory() {
        host_t host = mach_host_self();
        vm_size_t page_size;
        mach_port_t host_port = mach_host_self();
        mach_msg_type_number_t count;

        host_page_size(host_port, &page_size);
        vm_statistics_data_t vm_stat;

        count = sizeof(vm_stat) / sizeof(natural_t);
        if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &count) != KERN_SUCCESS) {
            return 0; // Failed to get memory statistics
        }

        natural_t pages = vm_stat.wire_count + vm_stat.active_count + vm_stat.inactive_count + vm_stat.free_count;
        return (size_t)pages * (size_t)page_size;
    }

    size_t getFreeSystemMemory() {
        mach_port_t host_port = mach_host_self();
        mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
        vm_size_t page_size;

        host_page_size(host_port, &page_size);

        vm_statistics_data_t vm_stat;
        if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS) {
            return 0; // Failed to get memory statistics
        }

        natural_t free_memory = vm_stat.free_count;
        return (size_t)free_memory * (size_t)page_size;
    }

#endif