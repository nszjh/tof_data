#ifndef  __TOF_DATA_H
#define  __TOF_DATA_H
#include "DPtr.h"

struct TofOutput
{
    char className[64];
    int dataWidth;
    int dataHeight;
};

struct DataPacket
{
    int rows;
    int cols;
    int channel;
    DPtr<char> data;
};


// tof data floor
class TofData
{
public:
    virtual DPtr<TofOutput> init(void *argv) = 0;//返回
    virtual void uninit() = 0;
    DPtr<DataPacket> setDevice(const char *com);

    // typedef Function<void (DPtr<DataPacket> frame)> DataCallback;
    // void registerDepth_DataCb(DataCallback f) { DepthCb = f; };//回调函数：幅度图
    // void registerAmplitude_DataCb(DataCallback f) { AmpCb = f; };//回调函数：深度图
    
protected:    
	virtual DPtr<char> setDeviceByCmd(const char* com) { return nullptr;  }
    // DataCallback DepthCb;
    // DataCallback AmpCb;
    int _isInit;
    int _rows;
    int _cols;
    int _channel;
    int _dataSize;
    int _isGetStream;
};


#endif