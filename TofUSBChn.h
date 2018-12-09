#ifndef  __TOF_USB_CHN_H
#define  __TOF_USB_CHN_H

#include "TofData.h"
#include "Net.h"
#include "DPtr.h"
#include <thread>
#ifdef WIN32
#include "USBSysWin.h"
#endif

class TofUSBChn : public TofData
{
public:
    TofUSBChn();
    ~TofUSBChn();
    //virtual char * init(void *argv);
    virtual DPtr<TofOutput> init(void *argv);//返回
    virtual void uninit();

protected:
    virtual DPtr<char> setDeviceByCmd(const char* com);
#ifdef WIN32
    DPtr<USBSysWin> _usb_sys_win;
#endif
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