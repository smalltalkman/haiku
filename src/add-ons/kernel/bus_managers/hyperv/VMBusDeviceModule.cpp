/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "VMBusDevicePrivate.h"


static status_t
vmbus_device_init(device_node* node, void** _driverCookie)
{
	CALLED();

	VMBusDevice* device = new(std::nothrow) VMBusDevice(node);
	if (device == NULL) {
		ERROR("Unable to allocate VMBus device object\n");
		return B_NO_MEMORY;
	}

	status_t status = device->InitCheck();
	if (status != B_OK) {
		ERROR("Failed to set up VMBus device object\n");
		delete device;
		return status;
	}
	TRACE("VMBus device object created\n");

	*_driverCookie = device;
	return B_OK;
}


static void
vmbus_device_uninit(void* driverCookie)
{
	CALLED();
	VMBusDevice* device = reinterpret_cast<VMBusDevice*>(driverCookie);
	delete device;
}


static void
vmbus_device_removed(void* _device)
{
	CALLED();
}


static uint32
vmbus_device_get_bus_version(hyperv_device cookie)
{
	CALLED();
	VMBusDevice* device = reinterpret_cast<VMBusDevice*>(cookie);
	return device->GetBusVersion();
}


static status_t
vmbus_device_open(hyperv_device cookie, uint32 txLength, uint32 rxLength,
	hyperv_device_callback callback, void* callbackData)
{
	CALLED();
	VMBusDevice* device = reinterpret_cast<VMBusDevice*>(cookie);
	return device->Open(txLength, rxLength, callback, callbackData);
}


static void
vmbus_device_close(hyperv_device cookie)
{
	CALLED();
	VMBusDevice* device = reinterpret_cast<VMBusDevice*>(cookie);
	device->Close();
}


static status_t
vmbus_device_write_packet(hyperv_device cookie, uint16 type, void* buffer, uint32 length,
	bool responseRequired, uint64 transactionID)
{
	VMBusDevice* device = reinterpret_cast<VMBusDevice*>(cookie);
	return device->WritePacket(type, buffer, length, responseRequired, transactionID);
}


static status_t
vmbus_device_read_packet(hyperv_device cookie, vmbus_pkt_header* _header, uint32* _headerLength,
	void* _buffer, uint32* _length)
{
	VMBusDevice* device = reinterpret_cast<VMBusDevice*>(cookie);
	return device->ReadPacket(_header, _headerLength, _buffer, _length);
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			break;
	}

	return B_ERROR;
}


hyperv_device_interface gVMBusDeviceModule = {
	{
		{
			HYPERV_DEVICE_MODULE_NAME,
			0,
			std_ops
		},
		NULL,	// supported devices
		NULL,	// register node
		vmbus_device_init,
		vmbus_device_uninit,
		NULL,	// register child devices
		NULL,	// rescan
		vmbus_device_removed
	},

	vmbus_device_get_bus_version,
	vmbus_device_open,
	vmbus_device_close,
	vmbus_device_write_packet,
	vmbus_device_read_packet
};
