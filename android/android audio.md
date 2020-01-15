# android audio相关问题记录
## android播放音频后会等待几秒才会关闭输出的问题
修改frameworks/av/services/audioflinger/AudioFlinger.h中
```
static const nsecs_t kDefaultStandbyTimeInNsecs = seconds(0);//seconds(3); modify by leijie
```
kDefaultStandbyTimeInNsecs为0秒，默认为3秒
