#include <common.h>
#include <command.h>
#include <env_callback.h>

#include "definitions.h"
#include "common_boot.h"

#include <malloc.h>

/**
 * Helper to keep logging easy.
 */
#define SK_LOG_NFS(...) printf("Siklu NFS: " __VA_ARGS__)

static int nfs_tftp_get_file(const char *path, const char *file, char *address, bool is_tftp) {
	char cmd[1024];
	
	snprintf(cmd, sizeof(cmd), 
			"%s \"%s\" \"%s:%s/%s\"", (is_tftp ? "tftp" : "nfs"),
			address, env_get(ENV_NFS_SERVERIP), path, file);

	return run_command(cmd, 0);
}

int format_rootpath_and_developer_id(const char *rootpath, const char *developer_id, 
		char *output, size_t output_size) {
	size_t ret;
	
	if (strstr(rootpath, "%s")) {
		if (! developer_id) {
			SK_LOG_NFS("please set " ENV_NFS_DEVELOPER_ID " to a valid developer name\n");
			return -EINVAL;
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
		return -EINVAL;
	}
	
	return 0;
}

static int 
load_images(const char *rootpath, bool is_tftp) {
	int ret;

	ret = nfs_tftp_get_file(rootpath, kernel_path(), kernel_load_address(), is_tftp);
	if (ret) {
		fit_update_dtb_addr();
		ret = nfs_tftp_get_file(rootpath, kernel_fit_path(), kernel_load_address(), is_tftp);
		if (ret) {
			printk(KERN_ERR "Failed to get both %s and %s from server\n", kernel_path(), kernel_fit_path());
			return CMD_RET_FAILURE;
		}
		return CMD_RET_SUCCESS;
	}
	
	ret = nfs_tftp_get_file(rootpath, dtb_path(), dtb_load_address(), is_tftp);
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

	rootpath = env_get(ENV_NFS_ROOTPATH);
	if (! rootpath) {
		rootpath = CONFIG_SIKLU_DEFAULT_NFS_ROOTPATH;
		SK_LOG_NFS("Using default rootpath: \"%s\"\n", rootpath);
	}

	ret = format_rootpath_and_developer_id(rootpath, env_get(ENV_NFS_DEVELOPER_ID),
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
	
	netmask = env_get("netmask");
	if (! netmask) {
		netmask = CONFIG_SIKLU_DEFAULT_NFS_NETMASK;
		SK_LOG_NFS("Using default netmask: %s\n", netmask);
	}

	gateway = env_get("gatewayip");
	if (! gateway)
		gateway = "";
	
	snprintf(nfsroot, sizeof(nfsroot),
			"%s:%s,tcp,nfsvers=%s", env_get(ENV_NFS_SERVERIP), rootpath, CONFIG_SIKLU_LINUX_NFS_VERSION);
	
	snprintf(ip, sizeof(ip), 
			"%s:%s:%s:%s:%s:%s:none",
			env_get("ipaddr"), env_get(ENV_NFS_SERVERIP), gateway, netmask, CONFIG_SIKLU_NFS_HOSTNAME, nfs_netdev);
	
	snprintf(bootargs, sizeof(bootargs), 
			"root=/dev/nfs rw ip=%s nfsroot=%s", ip, nfsroot);

	setup_bootargs(bootargs);
} 

static int 
setup_static_address(const char *address) {
	SK_LOG_NFS("Using static ip: %s\n", address);
	
	env_set("ipaddr", address);
	
	return 0;
}

static int 
setup_dhcp_address(void) {
	int ret;
	char *serverip = strdup(env_get("serverip"));
	char *rootpath = strdup(env_get("rootpath"));
	
	SK_LOG_NFS("Using DHCP...\n");
	ret = run_command("dhcp", 0);

	env_set("serverip", serverip);
	if (serverip)
		free(serverip);
	
	env_set("rootpath", rootpath);
	if (rootpath)
		free(rootpath);
	
	return ret;
}

static int 
setup_ip_address(void) {
	const char *ip = env_get(ENV_NFS_STATIC_IP);
	
	if (ip) {
		return setup_static_address(ip);
	} else {
		return setup_dhcp_address();
	}
}

static void 
setup_server_address(void) {
	const char *server_address = env_get(ENV_NFS_SERVERIP);
	if (! server_address) {
		server_address = CONFIG_SIKLU_DEFAULT_NFS_SERVERIP;
		SK_LOG_NFS("Using default server ip: \"%s\"\n", server_address);
		env_set(ENV_NFS_SERVERIP, server_address);
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
	bool is_tftp = false;
	
	/** Check for USB */
	if (argc == 2) {
		if (strcmp(argv[1], "usb") == 0) {
			usb = true;
		} else if (strcmp(argv[1], "usb_tftp") == 0) {
			usb = true;
			is_tftp = true;
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
		env_set("ethact", USB_ETHERNET_DRIVER);
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
	
	ret = load_images(rootpath, is_tftp);
	if (ret) {
		return CMD_RET_FAILURE;
	}

	setup_nfs_bootargs(rootpath, usb);
	
	load_kernel_image();
	
	return CMD_RET_FAILURE;
}

U_BOOT_CMD(
		siklu_nfs_boot,
		2,
		0,
		do_nfs_boot,
		"siklu_nfs_boot [usb | usb_tftp]",
		"Loads the system from NFS\n"
);
