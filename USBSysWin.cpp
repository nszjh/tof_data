// CaptureDemo.cpp 
//

#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>

#include "USBSysWin.h"

#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"strmbase.lib")

#define DEFAULT_VIDEO_WIDTH     640
#define DEFAULT_VIDEO_HEIGHT    480

// this object is a SEMI-COM object, and can only be created statically.



EXTERN_GUID(GUID_XU,
	0xAEEFAF66, 0x8BF8, 0x4817, 0x80, 0xA4, 0x5C, 0x1C, 0x0A, 0x22, 0xAC, 0xE9);

//DEFINE_GUIDSTRUCT("AEEFAF66-8BF8-4817-80a4-5c1c0a22ace9", EXTENSION_CONFIG_GUID);
DEFINE_GUIDSTRUCT("66afefae-f88b-1748-80a4-5c1c0a22ace9", EXTENSION_CONFIG_GUID);
#define EXTENSION_CONFIG_GUID DEFINE_GUIDNAMED(EXTENSION_CONFIG_GUID)  


//GUID GUID_XU{ 0xAE, 0xEF, 0xAF, 0x66, 0x8B, 0xF8, 0x48, 0x17, 0x80, 0xA4, 0x5C, 0x1C, 0x0A, 0x22, 0xAC, 0xE9 };


// 
class CSampleGrabberCB : public ISampleGrabberCB 
{
public:

	long Width;
	long Height;

	HANDLE BufferEvent;
	LONGLONG prev, step;
	// Fake out any COM ref counting
	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release() { return 1; }

	CSampleGrabberCB()
	{
	}
	// Fake out any COM QI'ing
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
	{
		//CheckPointer(ppv,E_POINTER);

		if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) 
		{
			*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
			return NOERROR;
		}    

		return E_NOINTERFACE;
	}

	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample )
	{
		return 0;
	}

	STDMETHODIMP BufferCB( double SampleTime, BYTE * pBuffer, long BufferSize )
	{

		printf(">>> SampleTime=%f, bufsize=%d\n", SampleTime, BufferSize);
		return 0;
	}
};

//void SetupVideoWindow(void)
//{
//	pVideoWindow->put_Left(0); 
//	pVideoWindow->put_Width(DEFAULT_VIDEO_WIDTH); 
//	pVideoWindow->put_Top(0); 
//	pVideoWindow->put_Height(DEFAULT_VIDEO_HEIGHT); 
//	pVideoWindow->put_Caption(L"Video Window");
//}



USBSysWin::USBSysWin()
{
	pMediaControl = NULL;
	pMediaEvent = NULL;
	pGraphBuilder = NULL;
	pCaptureGraphBuilder2 = NULL;
	pVideoWindow = NULL;
	pMonikerVideo = NULL;
	pVideoCaptureFilter = NULL;
	pGrabberF = NULL;
	pSampleGrabber = NULL;
	currentStatus = Stopped;
	
	_streamCommand = "";
	_width = 320;
	_height = 240;
	_dataSize = 320 * 240 * 2;
}

USBSysWin::~USBSysWin()
{

}

HRESULT USBSysWin::GetInterfaces(void)
{
	HRESULT hr;

	hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, (void **) &pGraphBuilder);
	if (FAILED(hr))
		return hr;
	hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
		IID_ICaptureGraphBuilder2, (void **) &pCaptureGraphBuilder2);
	if (FAILED(hr))
		return hr;

	hr = pGraphBuilder->QueryInterface(IID_IMediaControl,(LPVOID *) &pMediaControl);
	if (FAILED(hr))
		return hr;

	hr = pGraphBuilder->QueryInterface(IID_IVideoWindow, (LPVOID *) &pVideoWindow);
	if (FAILED(hr))
		return hr;

	hr = pGraphBuilder->QueryInterface(IID_IMediaEvent,(LPVOID *) &pMediaEvent);
	if (FAILED(hr))
		return hr;

	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, (void**)&pGrabberF);
	if (FAILED(hr))
		return hr;

	hr = pGrabberF->QueryInterface(IID_ISampleGrabber, (void**)&pSampleGrabber);
	if (FAILED(hr)) 
		return hr;

	return hr;
}

void USBSysWin::CloseInterfaces(void)
{
	if (pMediaControl)
		pMediaControl->StopWhenReady();

	currentStatus = Stopped;

	if(pVideoWindow) 
		pVideoWindow->put_Visible(OAFALSE);

	pMediaControl->Release();
	pGraphBuilder->Release();
	pVideoWindow->Release();
	pCaptureGraphBuilder2->Release();
	pMediaEvent->Release();
	pVideoCaptureFilter->Release();
	pVideoCaptureFilter->Release();
	pSampleGrabber->Release();
}

/*
* Find a node in a given KS filter by GUID
*/
DPtr<char>
USBSysWin::SetCommand(IKsTopologyInfo *pKsTopologyInfo, GUID guid, const char *command, int comLen)
{
	HRESULT hr = E_FAIL;
	DWORD dwNumNodes = 0;
	GUID guidNodeType;
	IKsControl *pKsControl = NULL;
	ULONG ulBytesReturned = 0;
	KSP_NODE ExtensionProp;
	DPtr<char> pResult = nullptr;

	if (!pKsTopologyInfo)
	{
		printf ("pKsTopologyInfo null\n");
		return pResult;
	}
	// Retrieve the number of nodes in the filter  
	hr = pKsTopologyInfo->get_NumNodes(&dwNumNodes);
	if (!SUCCEEDED(hr))
	{
		printf ("get_NumNodes error, hr=0x%x\n", hr);	
		return pResult;
	}
	if (dwNumNodes == 0)
	{
		printf ("get_NumNodes zero\n");	
		return pResult;
	}
	// Find the extension unit node that corresponds to the given GUID  
	for (unsigned int i = 0; i < dwNumNodes; i++)
	{
		hr = E_FAIL;
		pKsTopologyInfo->get_NodeType(i, &guidNodeType);
		if (IsEqualGUID(guidNodeType, KSNODETYPE_DEV_SPECIFIC))
		{
			hr = pKsTopologyInfo->CreateNodeInstance(i, IID_IKsControl, (void **)&pKsControl);

			if (SUCCEEDED(hr))
			{
				ExtensionProp.Property.Set = guid;
				ExtensionProp.Property.Id = 1;
				ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
				ExtensionProp.NodeId = i;
				ExtensionProp.Reserved = 0;

				char setCmd[60] = { 0 };//getDistanceAndAmplitudeSorted     getDistanceSorted
				strncpy(setCmd, command, comLen);
				hr = pKsControl->KsProperty((PKSPROPERTY)&ExtensionProp, sizeof(ExtensionProp), setCmd, comLen, &ulBytesReturned);
				if (SUCCEEDED(hr) && comLen)
				{//success
					pResult = DPtr<char>(new char[128]);
					ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
					hr = pKsControl->KsProperty((PKSPROPERTY)&ExtensionProp, sizeof(ExtensionProp), pResult.get(), 128, &ulBytesReturned);
					pKsControl->Release();
					if (SUCCEEDED(hr)) {
						
						break;
					}
					else {
						printf ("get data error, hr=0x%x\n", hr);
					}
				}
				pKsControl->Release();

			}
		}
	}

	return pResult;
}


int USBSysWin::SetFormat(int w, int h, int size)
{
	IAMStreamConfig   *pSC = NULL;
	HRESULT hr = E_FAIL;

	hr = pCaptureGraphBuilder2->FindInterface(&PIN_CATEGORY_CAPTURE,
		&MEDIATYPE_Video, pVideoCaptureFilter, IID_IAMStreamConfig, (void **)&pSC);
	if (FAILED(hr))
	{
		printf("Couldn't FindInterface!  hr=0x%x\n", hr);
		return -1;
		// Return an error.
	}

	AM_MEDIA_TYPE *pmt = NULL;
	pSC->GetFormat(&pmt);
	VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*)pmt->pbFormat;

	vih->bmiHeader.biWidth = w;
	vih->bmiHeader.biHeight = h;
	vih->bmiHeader.biSizeImage = size;

	//hr = pSampleGrabber->SetMediaType(&mt);
	pSC->SetFormat(pmt);   //重新设置参数
	if (FAILED(hr))
	{
		printf("setFormat error, hr=0x%x\n", hr);
		return -1;
	}
	return 0;
}


DPtr<char> USBSysWin::Set_uvcex(const char *com, int comLen)
{
	DPtr<char> rs = nullptr;
	CComPtr<IKsTopologyInfo> ks_topology_info = nullptr;
	if (pVideoCaptureFilter) {
		HRESULT hr = pVideoCaptureFilter->QueryInterface(__uuidof(IKsTopologyInfo), (void **)&ks_topology_info);

		if (hr == S_OK)
		{
			rs = SetCommand(ks_topology_info, EXTENSION_CONFIG_GUID, com, comLen);
			if (rs)
			{
				printf(">>> SetCommand OK\n");
			}
		}
		ks_topology_info.Release();
	}
	return rs;
}


HRESULT USBSysWin::InitMonikers()
{
	USES_CONVERSION;
	HRESULT hr;
	ULONG cFetched;

	//https://docs.microsoft.com/zh-cn/windows/desktop/DirectShow/selecting-a-capture-device

	ICreateDevEnum *pCreateDevEnum;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if (FAILED(hr))
	{
		printf("Failed to enumerate all video and audio capture devices!  hr=0x%x\n", hr);
		return hr;
	}

	IEnumMoniker *pEnumMoniker;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
	if (FAILED(hr) || !pEnumMoniker)
	{
		printf("Failed to create ClassEnumerator!  hr=0x%x\n", hr);
		return -1;
	}

	while (hr = pEnumMoniker->Next(1, &pMonikerVideo, &cFetched), hr == S_OK)
	{
		IPropertyBag *pPropBag = NULL;
		HRESULT hr = pMonikerVideo->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMonikerVideo->Release();
			continue;
		}

		VARIANT var;
		VariantInit(&var);

		hr = pPropBag->Read(L"DevicePath", &var, 0);
		if (FAILED(hr))
		{
			VariantClear(&var);
			pMonikerVideo->Release();
			continue;
		}
		pPropBag->Release();

		std::string devpath = std::string(W2A(var.bstrVal));
		//if (devpath.find("vid_046d&pid_0825") == -1)
		if (devpath.find("vid_1d6b&pid_0102") == -1)
		{
			VariantClear(&var);
			pMonikerVideo->Release();
			continue;
		}
		VariantClear(&var);

		hr = pMonikerVideo->BindToObject(0, 0, IID_IBaseFilter, (void**)&pVideoCaptureFilter);
		if (FAILED(hr))
		{
			printf("Couldn't bind moniker to filter object!  hr=0x%x\n", hr);
			pMonikerVideo->Release();
			return hr;
		}

		char setCmd[60] = { "getDistanceSorted" };//getDistanceAndAmplitudeSorted     getDistanceSorted
		Set_uvcex(_streamCommand.c_str(), _streamCommand.size());

		pMonikerVideo->Release();
	}

	pEnumMoniker->Release();
	pCreateDevEnum->Release();
	return hr;
}


//https://docs.microsoft.com/zh-cn/windows/desktop/DirectShow/capturing-an-image-from-a-still-image-pin

HRESULT USBSysWin::StartCaptureVideo(std::string streamCommand, int w, int h, int size)
{
	_streamCommand = streamCommand;
	_width = w;
	_height = h;
	_dataSize = size;

	HRESULT hr = CoInitialize(NULL);

	//https://docs.microsoft.com/zh-cn/windows/desktop/DirectShow/about-the-capture-graph-builder
	hr = GetInterfaces();
	if (FAILED(hr))
	{
		printf("Failed to get video interfaces!  hr=0x%x\n", hr);
		return hr;
	}

	hr = pCaptureGraphBuilder2->SetFiltergraph(pGraphBuilder);
	if (FAILED(hr))
	{
		printf("Failed to attach the filter graph to the capture graph!  hr=0x%x\n", hr);
		return hr;
	}

	hr = InitMonikers();
	if(FAILED(hr))
	{
		printf("Failed to InitMonikers!  hr=0x%x\n", hr);
		return hr;
	}


	hr = pGraphBuilder->AddFilter(pVideoCaptureFilter, L"Video Capture");
	if (FAILED(hr))
	{
		printf("Couldn't add video capture filter to graph!  hr=0x%x\n", hr);
		pVideoCaptureFilter->Release();
		return hr;
	}

	////SampleGrabber Filter
	hr = pGraphBuilder->AddFilter(pGrabberF, L"Sample Grabber");
	if (FAILED(hr))
	{
		printf("Couldn't add sample grabber to graph!  hr=0x%x\n", hr);
		// Return an error.
	}


	//hr = pCaptureGraphBuilder2->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVideoCaptureFilter, pGrabberF, 0 );
	//if (FAILED(hr))
	//{
	//	printf("Couldn't render video capture stream. The device may already be in use.  hr=0x%x\n", hr);
	//	pVideoCaptureFilter->Release();
	//	return hr;
	//}


	hr = ConnectFilters(pGraphBuilder, pVideoCaptureFilter, pGrabberF);

	SetFormat(_width, _height, _dataSize);

	hr = pSampleGrabber->SetOneShot(FALSE);
	hr = pSampleGrabber->SetBufferSamples(TRUE);


	CSampleGrabberCB* CB = new CSampleGrabberCB();
	hr = pSampleGrabber->SetCallback(CB, 1);
	if (FAILED(hr))
	{
		printf("set still trigger call back failed, hr=0x%x\n\n", hr);
		return hr;
	}

	//SetupVideoWindow();

	hr = pMediaControl->Run();
	if (FAILED(hr))
	{
		printf("Couldn't run the graph!  hr=0x%x\n", hr);
		return hr;
	}
	else 
		currentStatus = Running;

	return hr;
}

void USBSysWin::StopCaptureVideo()
{
	if (currentStatus == Running)
	{
		pMediaControl->Stop();
		CloseInterfaces();
		CoUninitialize();
		currentStatus = Stopped;
	}
}


// int main()
// {
// 	HRESULT hr;														
// 	char cmd;
// 	printf("p - Play Video\ns - Stop Video\nq - Quit\n\n");

// 	USBSysWin _usbp;
// 	while (true)
// 	{
// 		std::cin >> cmd;
// 		switch(cmd)
// 		{
// 		case 'p': 
// 			{															
// 				printf("	Play Video!\n");
// 				hr = _usbp.StartCaptureVideo();
// 				if (FAILED(hr))	
// 					printf("Error!");
// 			}
// 			break;
// 		case 's': 
// 			{															
// 				printf("	Stop Video!\n");
// 				_usbp.StopCaptureVideo();
// 			}
// 			break;
// 		case 'q': 
// 			return 0;											
// 			break;
// 		default: printf("Unknown command!\n");
// 			break;
// 		}
// 	}
// }