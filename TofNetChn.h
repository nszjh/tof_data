#ifndef  __TOF_NET_CHN_H
#define  __TOF_NET_CHN_H

#include "TofData.h"
#include "Net.h"
#include "DPtr.h"
#include <thread>

class TofNetChn : public TofData
{
public:
    TofNetChn();
    ~TofNetChn();
    //virtual char * init(void *argv);
    virtual DPtr<TofOutput> init(void *argv);//返回
    virtual void uninit();

protected:
    void setServer(const char *ipAddr);
    virtual DPtr<char> setDeviceByCmd(const char* com);

    NetRequest    _netRequest;
    int  _isServerSet;
    int  _width;
    int  _height;
};

// #ifdef WIN32
// extern "C" __declspec(dllexport) TofNetChn* GetTofNetChn(void);
// #else
// extern "C"    //must add 
// DPtr<TofClassDL> GetTofNetChn(void);
// #endif

#endif