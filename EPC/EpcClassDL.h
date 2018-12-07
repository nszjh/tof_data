#ifndef  __EPC_CLASS_DL_H
#define  __EPC_CLASS_DL_H

#include "TofClassDL.h"
#include "APP.h"
#include "Net.h"
#include "DPtr.h"
#include "DGrabber.h"
#include <thread>

class EpcClassDL : public TofClassDL, public App
{
public:
    EpcClassDL();
    ~EpcClassDL();
    //virtual char * init(void *argv);
    virtual DPtr<TofOutput> init(void *argv);//返回

    virtual void uninit(); 
    virtual void pause(int isPause);
    virtual void getServerConf(char *Addr, int AddrLen, int &port);
    virtual void resetConf(int set);
protected:
    virtual void thread_Loop();
    void setAmpImage(const char* buffer, int bufferLen);
    void setDepthImage(const char* buffer, int bufferLen);
    void setDepthImage2Point(const char* buffer, int bufferLen);
    void setDepthImage3Point(const char* buffer, int bufferLen);
    void setServer(const char *ipAddr);    
    void pcdFliterAndShowThread();
    DPtr<cv::Mat> rawDataTo32F(const char* buffer, int bufferLen);
    DPtr<cv::Mat> rawDataTo8U(const char* buffer, int bufferLen);
	void LoadConfig(const char *filename);
    void LoadOtherConfig(const char *filename);
    void initEpcDev();
    void setIntegrationTime(int value);
    void setDeviceByCmd(char *EpcCmd);
    void setMinMaxExpos(int isMin, int value);
    void setRotate(int isRotate);


    NetRequest    _netRequest;
    int  _width;
    int  _height;
    int  _isServerSet;
    int  _isRotate;
    int _maxExpos;
    int _minExpos;
    int _w73;
    int _field_view;

    int _port;
    char _ipAddr[32];

    DPtr<DGrabber<DPtr<char>>>  _frameGrabber;
    std::thread  _pcdThread;
    int   _pcdThreadStop;
    int   _isPause;
    int   _isResetConf;
    std::string othrConfName = "Time3D.dat";
    std::string confName = "config.dat";
};

#ifdef WIN32
extern "C" __declspec(dllexport) TofClassDL* GetEpcClassDL(void);
#else
extern "C"    //must add 
DPtr<TofClassDL> GetEpcClassDL(void);
#endif

#endif