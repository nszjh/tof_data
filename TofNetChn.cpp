#include "TofNetChn.h"
#include <iostream>
// #ifndef WIN32
// #include "sys/time.h"
// #endif

#define  CLASS_NAME  "TofNetChn"


DPtr<TofOutput> TofNetChn::init(void *argv)
{
    char* ipAddr = (char *) (argv);

	std::cout << "TofNetChn: init\n";
    if(ipAddr)
    {
        setServer(ipAddr);
    }

    DPtr<TofOutput> _o(new TofOutput);
    memset(_o->className, 0, sizeof(_o->className));
    strncpy(_o->className, CLASS_NAME, sizeof(_o->className));
    _o->dataHeight = _height;
    _o->dataWidth = _width;

    _isInit = 1;
    return _o;
}


void TofNetChn::uninit()
{
}


TofNetChn::TofNetChn()
{ 
    _isServerSet = 0;
    _width = 320;
    _height = 240;
}


TofNetChn::~TofNetChn()
{
    uninit();
}


void TofNetChn::setServer(const char *ipAddr)
{
    _netRequest.SetServer(ipAddr);
    _isServerSet = 1;
}



DPtr<char> TofNetChn::setDeviceByCmd(const char *EpcCmd)
{
    DPtr<char> pData = nullptr;
    if(_isServerSet)
    {
        if (_dataSize)
            pData = DPtr<char>(new  char[_dataSize]);

        _netRequest.Request(pData.get(), _dataSize, EpcCmd);
    }
    return pData;
}
