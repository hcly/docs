# s5p4418 android yuyv转nv21

```java

void CameraHardwareEase::YUYV_to_nv21(int src_w, int src_h,char *srcbuf, char *dstbuf,unsigned long int size)
{
       /* 计算循环次数，YUYV 一个像素点占2个字节*/
       int pixNUM = src_w * src_h;
       unsigned int cycleNum = size /pixNUM/2;
       printf("cycleNUM = %d\n",cycleNum);

      /*单帧图像中 NV12格式的输出图像 Y分量 和 UV 分量的起始地址，并初始化*/
      char *y = dstbuf;
      char *uv = dstbuf + pixNUM ;

      char *start = srcbuf;
      unsigned int i =0; 
      int j =0,k =0;

      /*处理Y分量*/
      for(i= 0; i<cycleNum ;i++)
      {
        int index =0;
        for(j =0; j< pixNUM*2; j=j+2) //YUYV单行中每两个字节一个Y分量
        {
            *(y+index)  =*(start + j);
            index ++;
        }
        start = srcbuf + pixNUM*2*i;
        y= y + pixNUM*3/2;
      }

      /**处理UV分量**/
      start = srcbuf;
      for(i= 0; i<cycleNum ;i++)
      {
        int uv_index = 0;
        for(j=0; j< src_h; j =j+2)  // 隔行, 我选择保留偶数行
        {
            for(k = j*src_w*2+1; k< src_w*2*(j+1); k=k+4) //YUYV单行中每四个字节含有一对UV分量
            {
                *(uv+ uv_index) = *(start +k +2);//*(start + k);//打开两个注释即为yuyv转nv12
                *(uv +uv_index+1) = *(start + k);//*(start +k +2);
                uv_index += 2;
            }
        }
        start = srcbuf + pixNUM*2*i;
        uv =uv + pixNUM*3/2;
      } 
}
```
nv12与nv21的区别是一个是u在前v在后，一个是v在前u在后





