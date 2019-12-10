

#include "precomp.h"
DEVICE_LIST	g_Devices;




void attach_tag_to_device(aiwITEM_ptr tag, aiwDeviceInfo_ptr d)
{

}
void detach_tag_from_device(aiwITEM_cptr tag)
{

}
void attach_device_tags(aiwDeviceInfo_ptr  d)
{

}
void detach_device_tags(aiwDeviceInfo_ptr d)
{

}



IOSS_API aiwDeviceInfo_ptr AIW_CALL io_create_device(
	aiwDriverInfo_ptr driverObj,
	aiwDeviceKey_cptr key
)
{
	aiwDeviceInfo_ptr dev = aiwNULL;
	dev = (aiwDeviceInfo_ptr)new char[sizeof(aiwDeviceInfo)];
	if (IsNull(dev)) {
		return aiwNULL;
	}
	std::memset(dev, 0, sizeof(aiwDeviceInfo));

	//�����豸����
	dev->k = *key;

	// �����豸������Ϣ
	dev->d = driverObj;

	if (driverObj) {
		driverObj->device_count++;
	}
	//��ʼ���豸��Ϣ������
	RtkInitializeListHead(&dev->tags);

	//�����豸�б�
	g_Devices.insert(DEVICE_LIST::value_type(*key, dev));

	return dev;
}

IOSS_API aiwBool AIW_CALL io_delete_device(aiwDeviceInfo_ptr dev)
{
	aiwDriverInfo_ptr driverObj;
	DEVICE_LIST::iterator it;

	//����豸����ر�ǩ�������
	detach_device_tags(dev);

	//�ͷ�g_Devices�и��豸��Ϣ
	it = g_Devices.find(dev->k);
	//���û�д��豸��Ҳ����ɾ���ɹ�
	if (it == g_Devices.end())
	{
		return aiwTRUE;
	}
	//�ͷ�ǰ���ҵ����豸��Ϣ������������Ϣ�ĵ�ַ
	driverObj = dev->d;

	g_Devices.erase(it);
	delete dev;

	//������Ϣ�Ĵ���
	if (driverObj) {
		//������Ϣ�еĹ����豸������1
		driverObj->device_count--;
		assert(driverObj->device_count >= 0);
		//������������Ϣ�еĹ����豸����Ϊ0����д������DLL
		if (!driverObj->device_count) {
			io_unload_driver(driverObj);
		}
	}
	return aiwTRUE;
}


IOSS_API aiwUInt32 AIW_CALL ioss_get_devices(
	aiwDeviceInfo_ptr  buffer,
	aiwUInt32 *maxcount
)
{
	aiwUInt32 count, i;
	count = std::min((aiwUInt32)g_Devices.size(), *maxcount);
	DEVICE_LIST::iterator p;
	p = g_Devices.begin();
	for (i = 0; i < count; i++, p++) {
		buffer[i] = *(p->second);
	}
	if (*maxcount < g_Devices.size()) {
		*maxcount = g_Devices.size();
	}
	return count;
}

IOSS_API aiwDeviceInfo_ptr AIW_CALL io_open_device(aiwDeviceKey_cptr key)
{
	DEVICE_LIST::iterator it;
	it = g_Devices.find(*key);
	if (it == g_Devices.end()) {
		return aiwNULL;
	}
	return it->second;
}

IOSS_API aiwBool AIW_CALL io_start_device(aiwDeviceInfo_ptr dev)
{

	aiwBool r = aiwFALSE;
	IOSS_STATUS st = IOSS_FAILED;

	assert(dev->d);

	if (!(dev->d->flags & DRIVER_FLAG_LOADED)) {
		dev->error = IOSS_DRIVER_NOT_LOADED;
		r = aiwFALSE;
	}
	else {
		if (IsNull(dev->d->start_device)) {
			r = aiwTRUE;
		}
		else {
			st = dev->d->start_device(dev);
			if (st == IOSS_FAILED) {
				r = aiwFALSE;
				dev->error = IOSS_DEVICE_NOT_STARTED;
			}
			else
			{
				r = aiwTRUE;
			}
		}
	}
	//����ʧ��
	if (!r) {
		dev->flags &= ~DF_Active;
	}
	//�����ɹ�
	else {
		dev->flags |= DF_Active;
		dev->error = 0;
	}

	//�����־
	if (r) {
		AIW_LOG_TRACE("Device %s started.",dev->k.Data);
	}
	else {
		AIW_LOG_TRACE("Cannot start device %s.", dev->k.Data);
	}

	return r;	
}

IOSS_API aiwBool AIW_CALL io_stop_device(aiwDeviceInfo_ptr dev)
{
	if (!(dev->flags & DF_Active)) {
		//�豸�Ѿ�ֹͣ
		return aiwTRUE;
	}
	AIW_LOG_TRACE("Stopping device %s.", dev->k.Data);

	if (IsNotNull(dev->d)) {
		if (IsNotNull(dev->d->stop_device)) {

			if (dev->d->stop_device(dev)== IOSS_FAILED) {
				dev->flags &= ~DF_Active;
				AIW_LOG_TRACE("Device %s not responding to STOP command.", dev->k.Data);
				return aiwFALSE;
			}
		}
	}
	dev->flags &= ~DF_Active;
	AIW_LOG_TRACE("Device %s stopped.", dev->k.Data);
	return aiwTRUE;
}

