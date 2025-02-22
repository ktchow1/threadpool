#include<iostream> 
#include<fstream> 
#include<cstdint> 
#include<string>         // for strncmp
#include<unistd.h>       // for close(), read()
#include<fcntl.h>        // for open() options
#include<sys/epoll.h>    // for epoll_create1(), epoll_ctl(), struct epoll_event

#define MAX_EVENTS 5
#define READ_SIZE 10

// ***************************************************************************** //
// Epoll is used for listening blocking type IO events : socket, pipe, std::cin.
// Why epoll is better than select : 17355702 
// ***************************************************************************** //
void test_epoll()
{
    // ***************************** //
    // *** Step 1 : Create Epoll *** //
    // ***************************** //
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        std::cout << "\nFailed to create epoll file descriptor";
        return;
    }
    
    // ****************************** //
    // *** Step 2 : Registeration *** //
    // ****************************** //
/*  auto fd = open("./temp", O_CREAT | O_TRUNC | O_RDWR); 
    // fopen() returns FILE*, while open() returns FD
    //  open() does not compile in release mode, it needs 3 args, why?
    
    if (fd == -1)
    {
        std::cout << "\nFailed to open file";
    } */

    epoll_event acquire_event0;
    epoll_event acquire_event1;
    acquire_event0.events  = EPOLLIN; // READY-TO-READ to event
    acquire_event0.data.fd = 0;
//  acquire_event1.events  = EPOLLIN; // READY-TO-READ to event
//  acquire_event1.data.fd = fd;
    
    // We can ADD/MOD/DEL
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &acquire_event0))
    {
        std::cout << "\nFailed to add file descriptor 0";
        close(epoll_fd);
        return;
    }
/*  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &acquire_event1)) // Epoll does not work for file.
    {
        std::cout << "\nFailed to add file descriptor 0";
        close(epoll_fd);
        return;
    } */
    
    // ******************************* //
    // *** Step 3 : Wait for event *** //
    // ******************************* //
    epoll_event received_events[MAX_EVENTS];

    bool running = true; 
    while(running)
    {
		// When events are detected, they are copied ...
        // from kernel-event-table (in kernel space)
        // to received_events (in user space)

        std::cout << "\nPolling for event, please enter text (quit) ... " << std::flush;
        auto event_count = epoll_wait(epoll_fd, received_events, MAX_EVENTS, 30000); // 30000 ms waiting 

        std::cout << "\n" << event_count << " events received" << std::flush;
        for(std::uint32_t n=0; n!=event_count; ++n)
        {
            char buffer[READ_SIZE + 1];
            size_t bytes = read(received_events[n].data.fd, buffer, READ_SIZE); // no null char in buffer

            std::string str(buffer, bytes);
            std::cout << "\n" << bytes << " bytes from fd " << received_events[n].data.fd << " : " << str;

            if (str == "quit\n") running = false;
        }
    }
    
    // *** Clear *** //
    close(epoll_fd);
}
