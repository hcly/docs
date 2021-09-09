# hi3519多sensor设置说明

## mipi接口模式与设备号
3519总共有12-lane输入,从手册上看支持12lane,8lane+4lane,4lane+4x2lane
SDK中从mipi_rx.c文件mipi_print_lane_divide_mode函数可以区分mipi模式
```java
static const char *mipi_print_lane_divide_mode(lane_divide_mode_t mode)
{
    switch (mode) {
        case LANE_DIVIDE_MODE_0:
            return "12";

        case LANE_DIVIDE_MODE_1:
            return "8+4";

        case LANE_DIVIDE_MODE_2:
            return "8+2+2";

        case LANE_DIVIDE_MODE_3:
            return "4+8";

        case LANE_DIVIDE_MODE_4:
            return "4+4+4";

        case LANE_DIVIDE_MODE_5:
            return "4+4+2+2";

        case LANE_DIVIDE_MODE_6:
            return "4+2+2+2+2";

        default:
            break;
    }

    return "N/A";
}
```
LANE_DIVIDE_MODE_4对应的是3个4lane接口
LANE_DIVIDE_MODE_6对应的是5个4lane+2lane+2lane+2lane+2lane接口
目前我只测试了LANE_DIVIDE_MODE_6模式的1个4lane sensor和LANE_DIVIDE_MODE_4模式的2个4lane sensor
由于最开始不了解，默认代码里一个sensor时是使用的LANE_DIVIDE_MODE_6模式
然后通过mipi_rx_hal.c中mipi_rx_drv_is_lane_valid函数可以确认每个通道对应的设备号
```java
int mipi_rx_drv_is_lane_valid(combo_dev_t devno, short lane_id, lane_divide_mode_t mode)
{
    int lane_valid = 0;

    switch (mode) {
        case LANE_DIVIDE_MODE_0:
            if (devno == 0) {
                if (0 <= lane_id && lane_id <= 11) {
                    lane_valid = 1;
                }
            }
            break;
        case LANE_DIVIDE_MODE_1:
            if (devno == 0) {
                if (0 <= lane_id && lane_id <= 7) {
                    lane_valid = 1;
                }
            } else if (devno == 3) {
                if (8 <= lane_id && lane_id <= 11) {
                    lane_valid = 1;
                }
            }
            break;
        case LANE_DIVIDE_MODE_2:
            if (devno == 0) {
                if (0 <= lane_id && lane_id <= 7) {
                    lane_valid = 1;
                }
            } else if (devno == 3) {
                if (lane_id == 8 || lane_id == 10) {
                    lane_valid = 1;
                }
            } else if (devno == 4) {
                if (lane_id == 9 || lane_id == 11) {
                    lane_valid = 1;
                }
            }
            break;
        case LANE_DIVIDE_MODE_3:
            if (devno == 0) {
                if (0 <= lane_id && lane_id <= 3) {
                    lane_valid = 1;
                }
            } else if (devno == 1) {
                if (4 <= lane_id && lane_id <= 11) {
                    lane_valid = 1;
                }
            }
            break;
        case LANE_DIVIDE_MODE_4:
            if (devno == 0) {
                if (0 <= lane_id && lane_id <= 3) {
                    lane_valid = 1;
                }
            } else if (devno == 1) {
                if (4 <= lane_id && lane_id <= 7) {
                    lane_valid = 1;
                }
            } else if (devno == 3) {
                if (8 <= lane_id && lane_id <= 11) {
                    lane_valid = 1;
                }
            }
            break;
        case LANE_DIVIDE_MODE_5:
            if (devno == 0) {
                if (0 <= lane_id && lane_id <= 3) {
                    lane_valid = 1;
                }
            } else if (devno == 1) {
                if (4 <= lane_id && lane_id <= 7) {
                    lane_valid = 1;
                }
            } else if (devno == 3) {
                if (lane_id == 8 || lane_id == 10) {
                    lane_valid = 1;
                }
            } else if (devno == 4) {
                if (lane_id == 9 || lane_id == 11) {
                    lane_valid = 1;
                }
            }
            break;
        case LANE_DIVIDE_MODE_6:
            if (devno == 0) {
                if (0 <= lane_id && lane_id <= 3) {
                    lane_valid = 1;
                }
            } else if (devno == 1) {
                if (lane_id == 4 || lane_id == 5) {
                    lane_valid = 1;
                }
            } else if (devno == 2) {
                if (lane_id == 6 || lane_id == 7) {
                    lane_valid = 1;
                }
            } else if (devno == 3) {
                if (lane_id == 8 || lane_id == 9) {
                    lane_valid = 1;
                }
            } else if (devno == 4) {
                if (lane_id == 10 || lane_id == 11) {
                    lane_valid = 1;
                }
            }
            break;
        default:
            break;
    }

    return lane_valid;
}
```
最终模式设置是在sample_comm_vi.c中SAMPLE_COMM_VI_StartMIPI函数
会调用SAMPLE_COMM_VI_SetMipiHsMode(LANE_DIVIDE_MODE_4)设置mipi模式
## 接口配置主要代码
sample_comm_vi.c文件SAMPLE_COMM_VI_GetComboAttrBySns函数
根据上面设置的mipi模式对各通道参数进行设置，以LANE_DIVIDE_MODE_4模式进行说明
```java
combo_dev_attr_t MIPI_4lane_CHN0 =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            HI_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1}
        }
    }
};
combo_dev_attr_t MIPI_4lane_CHN1 =
{
    .devno = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            HI_MIPI_WDR_MODE_NONE,
            {4, 5, 6, 7, -1, -1, -1, -1}
        }
    }
};
combo_dev_attr_t MIPI_4lane_CHN2 =
{
    .devno = 3,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            HI_MIPI_WDR_MODE_NONE,
            {8, 9, 10, 11, -1, -1, -1, -1}
        }
    }
};
```
分别对应硬件接口MIPI0,MIPI1,MIPI2
最后面那个devno为3具体是根据mipi_rx_hal.c中mipi_rx_drv_is_lane_valid函数LANE_DIVIDE_MODE_4模式中的代码进行确认
## 测试代码vi配置
双sensor主要配置
```java
SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

stViConfig.s32WorkingViNum                                   = 2;
stViConfig.as32WorkingViId[0]                                = 0;
stViConfig.astViInfo[0].stSnsInfo.s32BusId        = 1;
stViConfig.astViInfo[0].stSnsInfo.MipiDev         = ViDev[0];
stViConfig.astViInfo[0].stDevInfo.ViDev           = ViDev[0];
stViConfig.astViInfo[0].stDevInfo.enWDRMode       = enWDRMode;
stViConfig.astViInfo[0].stPipeInfo.enMastPipeMode = enMastPipeMode;
stViConfig.astViInfo[0].stPipeInfo.aPipe[0]       = ViPipe[0];
stViConfig.astViInfo[0].stPipeInfo.aPipe[1]       = -1;
stViConfig.astViInfo[0].stPipeInfo.aPipe[2]       = -1;
stViConfig.astViInfo[0].stPipeInfo.aPipe[3]       = -1;
stViConfig.astViInfo[0].stChnInfo.ViChn           = ViChn;
stViConfig.astViInfo[0].stChnInfo.enPixFormat     = enPixFormat;
stViConfig.astViInfo[0].stChnInfo.enDynamicRange  = enDynamicRange;
stViConfig.astViInfo[0].stChnInfo.enVideoFormat   = enVideoFormat;
stViConfig.astViInfo[0].stChnInfo.enCompressMode  = enCompressMode;

stViConfig.as32WorkingViId[1]                                = 1;
stViConfig.astViInfo[1].stSnsInfo.MipiDev         = ViDev[1];
stViConfig.astViInfo[1].stSnsInfo.s32BusId        = 5;
stViConfig.astViInfo[1].stDevInfo.ViDev           = ViDev[1];
stViConfig.astViInfo[1].stDevInfo.enWDRMode       = enWDRMode;
stViConfig.astViInfo[1].stPipeInfo.enMastPipeMode = enMastPipeMode;
stViConfig.astViInfo[1].stPipeInfo.aPipe[0]       = ViPipe[1];
stViConfig.astViInfo[1].stPipeInfo.aPipe[1]       = -1;
stViConfig.astViInfo[1].stPipeInfo.aPipe[2]       = -1;
stViConfig.astViInfo[1].stPipeInfo.aPipe[3]       = -1;
stViConfig.astViInfo[1].stChnInfo.ViChn           = ViChn;
stViConfig.astViInfo[1].stChnInfo.enPixFormat     = enPixFormat;
stViConfig.astViInfo[1].stChnInfo.enDynamicRange  = enDynamicRange;
stViConfig.astViInfo[1].stChnInfo.enVideoFormat   = enVideoFormat;
stViConfig.astViInfo[1].stChnInfo.enCompressMode  = enCompressMode;
```
其中ViDev对应上面说到的devno
s32BusId为具体硬件接口上的i2c总线
主要修改就是这些地方.
主要测试了mipi0掊一路4lane sensor
mipi0,mipi1各接一路4lane sensor
mipi0,mipi2各接一路4lane sensor
都能正常进行编码
