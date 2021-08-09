#include <common.h>
#include <command.h>
#include <env_callback.h>

#include "definitions.h"
#include "common_boot.h"

#include <malloc.h>

#include <linux/err.h>
#include "common_fdt.h"
#include "common_boot.h"

/**
 * Helper to keep logging easy.
 */
#define SK_LOG_NFS(...) printf("Siklu NFS: " __VA_ARGS__)
#define USB_RESET_GPIO_PIN 	55

static int nfs_get_file(const char *path, const char *file, char *address) {
	char cmd[1024];
	
	snprintf(cmd, sizeof(cmd), 
			"nfs \"%s\" \"%s:%s/%s\"", address, getenv(ENV_NFS_SERVERIP), path, file);

	return run_command(cmd, 0);
}

int format_rootpath_and_developer_id(const char *rootpath, const char *developer_id, 
		char *output, size_t output_size) {
	size_t ret;
	
	if (strstr(rootpath, "%s")) {
		if (! developer_id) {
			SK_LOG_NFS("please set " ENV_NFS_DEVELOPER_ID " to a valid developer name\n");
			return -ERR_INVAL;
		}
		
		ret = snprintf(output, output_size, 
				rootpath, developer_id);
	} else {
		if (developer_id)
			SK_LOG_NFS("rootpath does not contain %%s, using rootpath without a developer\n");
		
		ret = snprintf(output, output_size, "%s", rootpath);
	}
	
	if (ret == output_size) {
		printf("Siklu NFS: rootpath or developer_id are too long\n");
		return -ERR_INVAL;
	}
	
	return 0;
}

static int 
load_images(const char *rootpath) {
	int ret;
	
	ret = nfs_get_file(rootpath, kernel_path(), kernel_load_address());
	if (ret) {
		SK_LOG_NFS("Failed to get %s from the server\n", kernel_path());
		return CMD_RET_FAILURE;
	}

	/* No separate DTB on IPQ60xx */
	if (IS_ENABLED(CONFIG_ARCH_IPQ6018))
		return 0;


	ret = nfs_get_file(rootpath, dtb_path(), dtb_load_address());
	if (ret) {
		SK_LOG_NFS("Failed to get %s from the server\n", dtb_path());
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int
setup_rootpath(char *output, size_t output_size) {
	const char *rootpath;
	int ret;

	rootpath = getenv(ENV_NFS_ROOTPATH);
	if (! rootpath) {
		rootpath = CONFIG_SIKLU_DEFAULT_NFS_ROOTPATH;
		SK_LOG_NFS("Using default rootpath: \"%s\"\n", rootpath);
	}

	ret = format_rootpath_and_developer_id(rootpath, getenv(ENV_NFS_DEVELOPER_ID),
										   output, output_size);
	if (ret) {
		return CMD_RET_FAILURE;
	}
	
	return CMD_RET_SUCCESS;
}

static void 
setup_nfs_bootargs(const char *rootpath, bool usb) {
	char bootargs[1024];
	char nfsroot[512];
	char ip[512];
	const char *netmask;
	const char *nfs_netdev = CONFIG_SIKLU_NFS_NETDEV;
	const char *gateway;
	
	if (usb) {
		nfs_netdev = CONFIG_SIKLU_NFS_USB_NETDEV;
	}
	
	netmask = getenv("netmask");
	if (! netmask) {
		netmask = CONFIG_SIKLU_DEFAULT_NFS_NETMASK;
		SK_LOG_NFS("Using default netmask: %s\n", netmask);
	}

	gateway = getenv("gatewayip");
	if (! gateway)
		gateway = "";
	
	snprintf(nfsroot, sizeof(nfsroot),
			"%s:%s,tcp,nfsvers=%s", getenv(ENV_NFS_SERVERIP), rootpath, CONFIG_SIKLU_LINUX_NFS_VERSION);
	
	snprintf(ip, sizeof(ip), 
			"%s:%s:%s:%s:%s:%s:none",
			getenv("ipaddr"), getenv(ENV_NFS_SERVERIP), gateway, netmask, CONFIG_SIKLU_NFS_HOSTNAME, nfs_netdev);
	
	snprintf(bootargs, sizeof(bootargs), 
			"root=/dev/nfs rw ip=%s nfsroot=%s", ip, nfsroot);

	setup_bootargs(bootargs);
} 

static int 
setup_static_address(const char *address) {
	SK_LOG_NFS("Using static ip: %s\n", address);
	
	setenv("ipaddr", address);
	
	return 0;
}


static int 
setup_dhcp_address(void) {
	int ret;
	char *serverip = strdup(getenv("serverip"));
	char *rootpath = strdup(getenv("rootpath"));
	
	SK_LOG_NFS("Using DHCP...\n");
	ret = run_command("dhcp", 0);

	setenv("serverip", serverip);
	if (serverip)
		free(serverip);
	
	setenv("rootpath", rootpath);
	if (rootpath)
		free(rootpath);
	
	return ret;
}

static int 
setup_ip_address(void) {
	const char *ip = getenv(ENV_NFS_STATIC_IP);
	
	if (ip) {
		return setup_static_address(ip);
	} else {
		return setup_dhcp_address();
	}
}


static void 
setup_server_address(void) {
	const char *server_address = getenv(ENV_NFS_SERVERIP);
	if (! server_address) {
		server_address = CONFIG_SIKLU_DEFAULT_NFS_SERVERIP;
		SK_LOG_NFS("Using default server ip: \"%s\"\n", server_address);
		setenv(ENV_NFS_SERVERIP, server_address);
	} else {
		SK_LOG_NFS("Using predefined server ip: \"%s\"\n", server_address);
	}
}

static int 
setup_usb_eth(void) {
	return run_command("usb start", 0);
}


static int
do_nfs_boot(cmd_tbl_t *cmdtp, int flag, int argc,
			char *const argv[])
{

	int ret;
	char rootpath[1024];
	bool usb = false;

	/** Check for USB */
	if (argc == 2) {
		if (strcmp(argv[1], "usb") == 0) {
			usb = true;
		} else {
			return CMD_RET_USAGE;
		}
	}
	
	if (usb) {
		ret = setup_usb_eth();
		if (ret < 0) {
			SK_LOG_NFS("Failed to load USB devices\n");
			return CMD_RET_FAILURE;
		}
		setenv("ethact", USB_ETHERNET_DRIVER);
	}
	
	setup_server_address();
	
	ret = setup_ip_address();
	if (ret) {
		SK_LOG_NFS("Failed to setup IP\n");
		return CMD_RET_FAILURE;
	}
	
	ret = setup_rootpath(rootpath, sizeof(rootpath));
	if (ret) {
		return CMD_RET_FAILURE;
	}
	
	ret = load_images(rootpath);
	if (ret) {
		return CMD_RET_FAILURE;
	}

	setup_nfs_bootargs(rootpath, usb);

	// usb reset gpio pin must to be set off after usb start so that 
	// linux kernel usb reset will succeed
	gpio_set_value(USB_RESET_GPIO_PIN, 0);
	
	load_kernel_image();

	return CMD_RET_FAILURE;
}

U_BOOT_CMD(
		siklu_nfs_boot,
		2,
		0,
		do_nfs_boot,
		"siklu_nfs_boot [usb]",
		"Loads the system from NFS\n"
);
