//

#include "DPtr.h"
#include <dshow.h>
#include "qedit.h"
#include <iostream>
#include <objbase.h>
#include <atlconv.h>
#include <strmif.h>
#include <vidcap.h>         // For IKsTopologyInfo  
#include <ksproxy.h>        // For IKsControl  
#include <ks.h>
#include <ksmedia.h>
#include <Windows.h>
#include <atlbase.h>

#include "ConnectFilter.h"
using namespace ATL;


class USBSysWin
{
private:
	enum PLAYSTATE { Stopped = 0, Paused, Running, Init };
	//状态
	int  currentStatus;
	//
	IMediaControl *pMediaControl;
	IMediaEvent *pMediaEvent;

	IGraphBuilder *pGraphBuilder;
	ICaptureGraphBuilder2 *pCaptureGraphBuilder2;
	IVideoWindow *pVideoWindow;
	IMoniker *pMonikerVideo;
	IBaseFilter *pVideoCaptureFilter;
	IBaseFilter *pGrabberF;
	ISampleGrabber *pSampleGrabber;

	std::string _streamCommand;
	int _width;
	int _height;
	int _dataSize;
	/*
	* Find a node in a given KS filter by GUID
	*/
	HRESULT GetInterfaces(void);
	void    CloseInterfaces(void);
	HRESULT InitMonikers();
	int     SetFormat(int w, int h, int size);

	DPtr<char> SetCommand(IKsTopologyInfo *pKsTopologyInfo, GUID guid, const char *command, int comLen);

public:
	USBSysWin();
	~USBSysWin();

	DPtr<char> Set_uvcex(const char *com, int comLen);
	//https://docs.microsoft.com/zh-cn/windows/desktop/DirectShow/capturing-an-image-from-a-still-image-pin
	HRESULT StartCaptureVideo(std::string streamCommand, int w, int h, int size);
	void StopCaptureVideo();
};
