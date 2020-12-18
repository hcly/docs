package com.gzease.hlservice;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootRecv extends BroadcastReceiver {
    private final String TAG = "gzease-BootRecv";
    private static final String ACTION ="android.intent.action.BOOT_COMPLETED";
    @Override
    public void onReceive(Context context, Intent intent) {
		//if(intent.getAction().equals(ACTION)) {
            Log.d(TAG,"onReceive getAction:" + intent.getAction());
            context.startService(new Intent(context,MainService.class));
		//}
    }
}
