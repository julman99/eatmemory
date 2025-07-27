#include "eatmemory.h"
#ifdef SYSMEM_MODE_APPLE
    #include <mach/mach_host.h>
    #include <mach/task.h>
    #include <mach/mach_init.h>

    void get_system_memory_stats(struct system_memory_stats* stats) {
        vm_size_t page_size;
        mach_port_t host_port = mach_host_self();
        mach_msg_type_number_t count;

        host_page_size(host_port, &page_size);
        vm_statistics_data_t vm_stat;

        count = sizeof(vm_stat) / sizeof(natural_t);
        if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &count) != KERN_SUCCESS) {
            stats->supported = false;
            return;
        }
        stats->supported = true;

        natural_t pages = vm_stat.wire_count + vm_stat.active_count + vm_stat.inactive_count + vm_stat.free_count +
                          vm_stat.speculative_count + vm_stat.purgeable_count;
        
        stats->total = (size_t)pages * (size_t)page_size;

        natural_t free_memory = vm_stat.free_count + vm_stat.inactive_count;
        stats->free = (size_t)free_memory * (size_t)page_size;
    }

    void get_process_memory_stats(struct process_memory_stats* stats) {
        mach_task_basic_info_data_t task_basic_info;
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        
        kern_return_t kr = task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                                   (task_info_t)&task_basic_info, &count);
        
        if (kr != KERN_SUCCESS) {
            stats->supported = false;
            return;
        }
        
        stats->supported = true;
        stats->rss = (size_t)task_basic_info.resident_size;
    }
#endif