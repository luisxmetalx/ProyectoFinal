
#include <stdio.h>
#include <libudev.h>

#define SYSPATH "/sys/class/net"

int main(int argc, char *argv[])
{
	struct udev *udev;
	struct udev_device *dev, *dev_parent;
	char device[128]; 

	/* verify that we have an argument, like eth0, otherwise fail */
	if (!argv[1]) {
		fprintf(stderr, "Missing network interface name.\nexample: %s eth0\n", argv[0]);
		return 1;
	}

	/* build device path out of SYSPATH macro and argv[1] */
	snprintf(device, sizeof(device), "%s/%s", SYSPATH, argv[1]);
	
	/* create udev object */
	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "Cannot create udev context.\n");
		return 1;
	}

	/* get device based on path */
	dev = udev_device_new_from_syspath(udev, device);
	if (!dev) {
		fprintf(stderr, "Failed to get device.\n");
		return 1;
	}
	
	printf("I: DEVNAME=%s\n", udev_device_get_sysname(dev));
	printf("I: DEVPATH=%s\n", udev_device_get_devpath(dev));
	printf("I: MACADDR=%s\n", udev_device_get_sysattr_value(dev, "address"));

	dev_parent = udev_device_get_parent(dev);
	if (dev_parent)
		printf("I: DRIVER=%s\n", udev_device_get_driver(dev_parent));

	/* free dev */
	udev_device_unref(dev);

	/* free udev */
	udev_unref(udev);

	return 0;
}