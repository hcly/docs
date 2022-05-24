# android 录像网络传输保存成mp4文件
## 说明
通过MediaRecorder加LocalSocket实现本地网络传输，再通过其它硬件接口(非网络接口)将mp4数据传送到PC端稍微处理一下   
保存成播放器能正常播放的mp4文件，如果设备与服务器有网络连接可以直接跳过本记录使用[android-libstreaming](https://github.com/Ziggeo/android-libstreaming)  
通过libstreaming的例子1设置目标地址为本机127.0.0.1测试了一两次好像不行，放弃了。  
本记录只适合录像时间比较固定，同时录像时间不能太长的应用
## 系统源码修改
源码路径frameworks/av/media/libstagefright/MPEG4Writer.cpp  
1.函数MPEG4Writer::MPEG4Writer(int fd)
```java
// Verify mFd is seekable
off64_t off = lseek64(mFd, 0, SEEK_SET);
if (off < 0) {
    ALOGE("cannot seek mFd: %s (%d)", strerror(errno), errno);
    //release();
}
```
注释掉其中的release,android5以下没有这个判断可以不用管  

2.函数int64_t MPEG4Writer::estimateMoovBoxSize(int32_t bitRate)
```java
static const int64_t MAX_MOOV_BOX_SIZE = (3000000 * 6LL / 8000) * 1800;//(180 * 3000000 * 6LL / 8000); modify 3minutes to 30 minutes
```
修改MAX_MOOV_BOX_SIZE最大为30分钟计算的大小
如果只改第1个地方，针对录像时间小于15秒左右最后保存的文件经过处理能正常播放，但大于15秒后，时间越长moovbox内容越多，最后回写的次数无法确定，不好处理。
## 应用软件
有些变量，可自行定义
```java
public final static String SOCKET_ADDR = "com.gzease.camera";
```

1.mediarecorder设置,部分代码
```java
  mMediaRecorder.setCamera(mCamera);
  CamcorderProfile mProfile = CamcorderProfile.get(CamcorderProfile.QUALITY_480P);
  mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
  mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
  mMediaRecorder.setOutputFormat(mProfile.fileFormat);
  mMediaRecorder.setVideoEncodingBitRate(mProfile.videoBitRate);
  mMediaRecorder.setVideoEncoder(mProfile.videoCodec);
  mMediaRecorder.setVideoSize(mProfile.videoFrameWidth, mProfile.videoFrameHeight);
  mMediaRecorder.setVideoFrameRate(mProfile.videoFrameRate);
  mMediaRecorder.setAudioEncodingBitRate(mProfile.audioBitRate);
  mMediaRecorder.setAudioChannels(mProfile.audioChannels);
  mMediaRecorder.setAudioSamplingRate(mProfile.audioSampleRate);
  mMediaRecorder.setAudioEncoder(mProfile.audioCodec);
  mMediaRecorder.setMaxDuration(0);
  mMediaRecorder.setMaxFileSize(300*1024*1024);
  mMediaRecorder.setOrientationHint(90);
  mMediaRecorder.setPreviewDisplay(mSurfaceHolder != null ? mSurfaceHolder.getSurface() : null);
  if(DataBase.hasCamEnCodeStream) {
      DataBase.CamFileSize = 0;
      if(usbLocalSocket)
          mMediaRecorder.setOutputFile(localServer.getLocalSocket().getFileDescriptor());
      else
          mMediaRecorder.setOutputFile(mParcelWrite.getFileDescriptor());
  } else {
      File file = new File(DataBase.PATH_VIDEOFILE);
      if (file.exists())
          file.delete();
      try {
          fos = new FileOutputStream(DataBase.PATH_VIDEOFILE);
          //mMediaRecorder.setOutputFile(DataBase.PATH_VIDEOFILE);
          mMediaRecorder.setOutputFile(fos.getFD());
      } catch (FileNotFoundException e) {
          e.printStackTrace();
      } catch (IOException e) {
          e.printStackTrace();
      }
  }

  new LogTools(TAG, "startVideoRecording setOutputFile : " + DataBase.PATH_VIDEOFILE);
  try {
      mMediaRecorder.prepare();
      mMediaRecorder.start();
  } catch (IOException e) {
      e.printStackTrace();
      new LogTools().e(TAG, "startVideoRecording exception : " + e.getMessage());
      releaseVideoRecorder();
  }
```
一定要调用setMaxFileSize设置文件最大大小，不然系统mpeg4write中moovBox是按最小值处理，会影响最终处理流程  

2.localserver
```java
package com.gzease.CameraTools;

import android.net.LocalServerSocket;
import android.net.LocalSocket;

import com.gzease.tools.DataBase;
import com.gzease.tools.LogTools;

import java.io.IOException;
import java.io.InputStream;

public class LocalServer extends Thread{
    private final String TAG = "LocalServer";
    private final int debug = DataBase.D_LEVEL_OUT;

    private LocalServerSocket server;
    private boolean isRun = true;
    private boolean isConnect = false;
    private LocalSocket localSocket;
    @Override
    public void run() {
        super.run();
        try {
            server = new LocalServerSocket(DataBase.SOCKET_ADDR);
            new LogTools(TAG," accept start",debug);
            localSocket = server.accept();
            new LogTools(TAG," accept end",debug);
            isConnect = true;
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    public LocalSocket getLocalSocket() {
        new LogTools(TAG," getLocalSocket isConnect:" + isConnect,debug);
        if(isConnect)
            return localSocket;
        else
            return null;
    }

    @Override
    public void interrupt() {
        super.interrupt();
        isRun = false;
    }
}
```

3.localclient
```java
package com.gzease.CameraTools;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;

import com.gzease.tools.DataBase;
import com.gzease.tools.LogTools;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;

public class LocalClient extends Thread{
    private static final String TAG = "LocalClient";
    private static final int debug = DataBase.D_LEVEL_OUT;
    private InputStream mInStream=null;
    private LocalSocket localSocket;
    private ReadThread readThread;
    private boolean hasWriteFile = false;
    private static String fileName = "/sdcard/test.mp4";
    private static String destFileName = "/sdcard/test1.mp4";
    @Override
    public void run() {
        super.run();
        try {
            Thread.sleep(1000);
            localSocket = new LocalSocket();
            new LogTools(TAG, "connect start",debug);
            localSocket.connect(new LocalSocketAddress(DataBase.SOCKET_ADDR));
            new LogTools(TAG, "connect end",debug);
            mInStream = localSocket.getInputStream();
            if(mInStream != null) {
                readThread = new ReadThread();
                readThread.start();
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

    }

    public LocalSocket getLocalSocket() {
        if(mInStream != null)
            return  localSocket;
        else
            return null;
    }

    @Override
    public void interrupt() {
        super.interrupt();
        if(readThread != null)
            readThread.interrupt();
    }

    class ReadThread extends Thread {
        private boolean isRun = true;
        @Override
        public void run() {
            super.run();
            try {
                while (isRun) {
                    //byte[] buffer = new byte[1024 * 1024];
                    byte[] buffer = new byte[5 * 1024];
                    int size = mInStream.read(buffer);
                    if(size > 0) {
                        if(hasWriteFile) {
                            if(DataBase.CamFileSize == 0) {
                                File file = new File(fileName);
                                if(file.exists())
                                    file.delete();
                            }
                            toFile(buffer,size);
                        } else
                            sendtoPc(buffer,size);
                        DataBase.CamFileSize += size;
                        new LogTools(TAG," DataBase.CamFileSize:" + DataBase.CamFileSize,debug);
                    } else {
                        this.interrupt();
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void interrupt() {
            super.interrupt();
            isRun = false;
        }
    }

    private void sendtoPc(byte[] buf,int size) {
    }

    private void toFile(byte[] buf,int size) {
        try {
            // 打开一个随机访问文件流，按读写方式
            RandomAccessFile randomFile = new RandomAccessFile(fileName, "rw");
            // 文件长度，字节数
            long fileLength = randomFile.length();
            // 将写文件指针移到文件尾。
            randomFile.seek(fileLength);
            randomFile.write(buf,0,size);
            randomFile.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void DealFile() {
        RandomAccessFile srcfile = null;
        RandomAccessFile destfile = null;
        long starttime,endtime;
        starttime = System.currentTimeMillis();
        new LogTools(TAG,"start time :" + starttime,debug);
        try {
            File file = new File(fileName);
            File file1 = new File(destFileName);
            if(file1.exists())
                file1.delete();
            srcfile = new RandomAccessFile(fileName, "r");
            destfile = new RandomAccessFile(destFileName, "rw");
            new LogTools(TAG,"filelen :" + file.length(),debug);
            srcfile.seek(24);//rewrite head
            destfile.writeInt(srcfile.readInt());
            srcfile.seek(4);
            byte[] dat = new byte[20];//writeFtypBox ftyp+mp42+0000+isom+mp42
            srcfile.read(dat);
            destfile.write(dat);
            long moovStart = searchEndStr(fileName,"moov");
            new LogTools(TAG,"moovStart:" + moovStart,debug);
            srcfile.seek(moovStart - 4);
            int moovSize = srcfile.readInt();
            destfile.writeInt(moovSize);
            dat = new byte[moovSize - 4];
            srcfile.read(dat);
            destfile.write(dat);
            srcfile.seek(file.length() - 8);
            int freeSize = srcfile.readInt();
            new LogTools(TAG,"freeSize:" + Integer.toHexString(freeSize),debug);
            destfile.writeInt(freeSize);//4
            destfile.write("free".getBytes());//4
            dat = new byte[freeSize - 4 - 4];
            destfile.write(dat);
            srcfile.seek(moovStart - 8);//media data size
            int mediaDataSize = srcfile.readInt();
            destfile.writeInt(mediaDataSize);//write media data size
            srcfile.seek(40);
            mediaDataSize -= 4;//start from mdat
            while (true) {
                byte[] mdat = new byte[100*1024];
                srcfile.read(mdat);
                if(mediaDataSize > mdat.length) {
                    mediaDataSize -= mdat.length;
                    destfile.write(mdat);
                } else {
                    destfile.write(mdat,0,mediaDataSize);
                    mediaDataSize = 0;
                }
                new LogTools(TAG,"media size1:" + mediaDataSize,debug);
                if(mediaDataSize <= 0)
                    break;
            }
            srcfile.close();
            destfile.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    private static long searchEndStr(String fileName,String info) {
        long index = 0;
        int searchlen = 2*1024;
        File file = new File(fileName);
        long starttime,endtime;
        starttime = System.currentTimeMillis();
        new LogTools(TAG,"searchEndStr start time :" + starttime,debug);
        try {
            RandomAccessFile srcfile = new RandomAccessFile(fileName, "r");
            for(int i = 1;;i++) {
                long start = file.length() - searchlen * i;
                if(start < 0)
                    break;
                srcfile.seek(file.length() - searchlen * i);
                byte[] dat = new byte[searchlen];
                srcfile.read(dat);
                String datStr = new String(dat,"ASCII");
                index = datStr.lastIndexOf(info);
                if(index > 0) {
                    index = start + index;
                    break;
                }
            }

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        endtime = System.currentTimeMillis();
        new LogTools(TAG,"searchEndStr end time :" + endtime + " cal:" + (endtime - starttime),debug);
        return index;
    }
}

```
录像完成后使用DealFile处理文件，得到最终能正常播放的test1.mp4,PC上的处理也是参考这个函数.
这种方式，最终的mp4文件会比一般的要大1M多左右，因为设置了最大文件大小导致free区数据比较大.但最终处理相对简单。

4.也可以通过Pipe方式
初始化
```java
try {
    mParcelFileDescriptors = ParcelFileDescriptor.createPipe();
    mParcelRead = new ParcelFileDescriptor(mParcelFileDescriptors[0]);
    mParcelWrite = new ParcelFileDescriptor(mParcelFileDescriptors[1]);
    new LogTools(TAG, "mParcelWrite:" + mParcelWrite + " mParcelRead:" + mParcelRead, debug);
    if (mParcelRead != null) {
        PipeReadThread pipeReadThread = new PipeReadThread(mParcelRead);
        pipeReadThread.start();
    }
} catch (IOException e) {
    e.printStackTrace();
}
```
PipeReadThread.java
```java
package com.gzease.CameraTools;

import android.os.ParcelFileDescriptor;

import com.gzease.tools.DataBase;
import com.gzease.tools.LogTools;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;

public class PipeReadThread extends Thread{
    private final String TAG = "PipeReadThread";
    private final int debug = DataBase.D_LEVEL_OUT;
    private boolean isRun = true;
    private InputStream is = null;
    private boolean hasWriteFile = true;
    private String fileName = "/sdcard/test.mp4";
    public PipeReadThread(ParcelFileDescriptor fileDescriptor) {
        is = new ParcelFileDescriptor.AutoCloseInputStream(fileDescriptor);
        new LogTools(TAG," is: " + is,debug);
    }

    @Override
    public void run() {
        super.run();
        if(is == null)
            return;
        while (isRun) {
            try {
                byte[] buffer = new byte[30 * 1024];
                int size = is.read(buffer);
                if(size > 0) {
                    if(hasWriteFile) {
                        if(DataBase.CamFileSize == 0) {
                            File file = new File(fileName);
                            if(file.exists())
                                file.delete();
                        }
                        toFile(buffer,size);
                    }
                    DataBase.CamFileSize += size;
                    new LogTools(TAG," size: " + size + " dat:" + DataBase.bytesToHexString(buffer,size)
                            + " DataBase.CamFileSize:" + DataBase.CamFileSize,debug);
                } else {
                    this.interrupt();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void interrupt() {
        super.interrupt();
        isRun = false;
    }

    private void toFile(byte[] buf,int size) {
        try {
            // 打开一个随机访问文件流，按读写方式
            RandomAccessFile randomFile = new RandomAccessFile(fileName, "rw");
            // 文件长度，字节数
            long fileLength = randomFile.length();
            // 将写文件指针移到文件尾。
            randomFile.seek(fileLength);
            randomFile.write(buf,0,size);
            randomFile.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

```
