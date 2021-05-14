package android.net.ethernet;

/**
 * Native calls to ethernet interface.
 *
 * {@hide}
 */
public class EthernetNative {
	public native static String getInterfaceName(int i);
	public native static int getInterfaceCnt();
	public native static int initEthernetNative();
	public native static String waitForEvent();
	public native static int isInterfaceAdded(String ifname);
}
