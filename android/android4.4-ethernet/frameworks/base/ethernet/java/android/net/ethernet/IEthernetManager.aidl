package android.net.ethernet;
import android.net.ethernet.EthernetDevInfo;
import android.net.DhcpInfo;

interface IEthernetManager
{
	String[] getDeviceNameList();
	void setEthState(int state);
	int getEthState( );
	void UpdateEthDevInfo(in EthernetDevInfo info);
	boolean isEthConfigured();
	EthernetDevInfo getSavedEthConfig();
	int getTotalInterface();
	void setEthMode(String mode);
	boolean isEthDeviceUp();
	boolean isEthDeviceAdded();
	DhcpInfo getDhcpInfo();
}
