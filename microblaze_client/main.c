#include "glob.h"

/*
  Initialise board, start lwip server and then call main_main()
 */
int main(void)
{
	struct ip_addr ipaddr;

	mac_ethernet_address[0] = 0x00;
	mac_ethernet_address[1] = 0x0a;
	mac_ethernet_address[2] = 0x35;
	mac_ethernet_address[3] = 0x00;
	mac_ethernet_address[4] = 0x01;
	mac_ethernet_address[5] = 0x02;

	netif = &server_netif;

	init_platform();

	/* initliaze IP addresses to be used */
	IP4_ADDR(&ipaddr,  SERV_0,SERV_1,SERV_2,SERV_3);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   0,  1);

	lwip_init();

	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(netif, &ipaddr, &netmask, &gw, mac_ethernet_address, PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	netif_set_default(netif);

	/* now enable interrupts */
	platform_enable_interrupts();

	/* specify that the network if is up */
	netif_set_up(netif);

	/* set global vars */
	init = 0;
	ret = 0;
	packet = NULL;

	/* start the application (web server, rxtest, txtest, etc..) */
	if(start_connection() != 0) {
		return -1;
	}

	/*
	  The IP and Port for the server to use to connect to
	 */
	xil_printf("Waiting for connection @ %d.%d.%d.%d:%d\n", SERV_0, SERV_1, SERV_2, SERV_3, PORT);

	/* wait for initialisation from host */
	while (1) {
		xemacif_input(netif);
		if(init) {
			break;
		}
	}

	xil_printf("Connected, running program\n");
	/* run main_main() */
	main_main();
	xil_printf("program finished\n");

	xil_printf("sending finish\n");
	/* close connection to host */
	int end = (ENDIAN_FLIP(DEINIT));
	send_data((void *)(char *)&end, 4, 1);
	xil_printf("waiting for ack\n");
	while (1) {
		xemacif_input(netif);
		if(ret) {
			break;
		}
	}
	pbuf_free(packet);
	xil_printf("all done\n");

	cleanup_platform();

	return 0;
}
