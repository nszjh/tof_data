// CaptureDemo.cpp 
//

#include "stdafx.h"


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

#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"strmbase.lib")

#define DEFAULT_VIDEO_WIDTH     640
#define DEFAULT_VIDEO_HEIGHT    480

// this object is a SEMI-COM object, and can only be created statically.



EXTERN_GUID(GUID_XU,
	0xAEEFAF66, 0x8BF8, 0x4817, 0x80, 0xA4, 0x5C, 0x1C, 0x0A, 0x22, 0xAC, 0xE9);

//DEFINE_GUIDSTRUCT("AEEFAF66-8BF8-4817-80a4-5c1c0a22ace9", PROPSETID_LOGITECH_VIDEO_XU);
DEFINE_GUIDSTRUCT("66afefae-f88b-1748-80a4-5c1c0a22ace9", PROPSETID_LOGITECH_VIDEO_XU);
#define PROPSETID_LOGITECH_VIDEO_XU DEFINE_GUIDNAMED(PROPSETID_LOGITECH_VIDEO_XU)  


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


class USBSystemPrivate
{
private:
	enum PLAYSTATE { Stopped = 0, Paused, Running, Init };
	//状态

public:
	USBSystemPrivate() {}
	~USBSystemPrivate() {}

	int  currentStatus = Stopped;

	//
	IMediaControl *pMediaControl = NULL;
	IMediaEvent *pMediaEvent = NULL;

	IGraphBuilder *pGraphBuilder = NULL;
	ICaptureGraphBuilder2 *pCaptureGraphBuilder2 = NULL;
	IVideoWindow *pVideoWindow = NULL;
	IMoniker *pMonikerVideo = NULL;
	IBaseFilter *pVideoCaptureFilter = NULL;
	IBaseFilter *pGrabberF = NULL;
	ISampleGrabber *pSampleGrabber = NULL;

	HRESULT GetInterfaces(void)
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

	void CloseInterfaces(void)
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
	HRESULT
	SetCommand(IKsTopologyInfo *pKsTopologyInfo, GUID guid)
	{
		HRESULT hr = E_FAIL;
		DWORD dwNumNodes = 0;
		GUID guidNodeType;
		IKsControl *pKsControl = NULL;
		ULONG ulBytesReturned = 0;
		KSP_NODE ExtensionProp;

		if (!pKsTopologyInfo)
			return E_POINTER;

		// Retrieve the number of nodes in the filter  
		hr = pKsTopologyInfo->get_NumNodes(&dwNumNodes);
		if (!SUCCEEDED(hr))
			return hr;
		if (dwNumNodes == 0)
			return E_FAIL;

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


					char setCmd[60] = { "getDistanceAndAmplitudeSorted" };//getDistanceAndAmplitudeSorted     getDistanceSorted

					hr = pKsControl->KsProperty((PKSPROPERTY)&ExtensionProp, sizeof(ExtensionProp), setCmd, sizeof(setCmd), &ulBytesReturned);
					pKsControl->Release();
					if (SUCCEEDED(hr))
					{
						break;
					}
				}
			}
		}

		return hr;
	}


	void SetUVCXU(IBaseFilter *VCap)
	{
		CComPtr<IKsControl> m_pKsControl;
		CComPtr<IKsTopologyInfo> ks_topology_info = nullptr;
		CComPtr<IUnknown> unknown = nullptr;
		HRESULT hr = VCap->QueryInterface(__uuidof(IKsTopologyInfo), (void **)&ks_topology_info);

		if (hr == S_OK)
		{
			hr = SetCommand(ks_topology_info, PROPSETID_LOGITECH_VIDEO_XU);
			if (SUCCEEDED(hr))
			{
				printf(">>> SetCommand OK\n");
			}
		}
	}

	HRESULT InitMonikers()
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

			SetUVCXU(pVideoCaptureFilter);

			pMonikerVideo->Release();
		}

		pEnumMoniker->Release();
		pCreateDevEnum->Release();
		return hr;
	}




	//https://docs.microsoft.com/zh-cn/windows/desktop/DirectShow/capturing-an-image-from-a-still-image-pin

	HRESULT CaptureVideo()
	{
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

		IAMStreamConfig   *pSC = NULL;

		hr = pCaptureGraphBuilder2->FindInterface(&PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Video, pVideoCaptureFilter, IID_IAMStreamConfig, (void **)&pSC);
		if (FAILED(hr))
		{
			printf("Couldn't FindInterface!  hr=0x%x\n", hr);
			// Return an error.
		}

		AM_MEDIA_TYPE *pmt = NULL;


		pSC->GetFormat(&pmt);


		VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*)pmt->pbFormat;

		vih->bmiHeader.biWidth = 320;
		vih->bmiHeader.biHeight = 480;
		vih->bmiHeader.biSizeImage = 320 * 480 * 2;

		//hr = pSampleGrabber->SetMediaType(&mt);
		pSC->SetFormat(pmt);   //重新设置参数
		if (FAILED(hr))
		{
			printf("setFormat error\n");
			return -1;
		}

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

	void StopPreview()
	{
		if (currentStatus == Running)
		{
			pMediaControl->Stop();
			CloseInterfaces();
			CoUninitialize();
			currentStatus = Stopped;
		}
	}
};

int main()
{
	HRESULT hr;														
	char cmd;
	printf("p - Play Video\ns - Stop Video\nq - Quit\n\n");

	USBSystemPrivate _usbp;
	while (true)
	{
		std::cin >> cmd;
		switch(cmd)
		{
		case 'p': 
			{															
				printf("	Play Video!\n");
				hr = _usbp.CaptureVideo();
				if (FAILED(hr))	
					printf("Error!");
			}
			break;
		case 's': 
			{															
				printf("	Stop Video!\n");
				_usbp.StopPreview();
			}
			break;
		case 'q': 
			return 0;											
			break;
		default: printf("Unknown command!\n");
			break;
		}
	}
}