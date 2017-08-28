// RTPFilter.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <streams.h>
#include <windows.h>
#include <initguid.h>
#include <olectl.h>
#include "RTPFilteruid.h"
#include "RTPFilter.h"
#if (1100 > _MSC_VER)
#include <olectlid.h>
#endif
#include <iostream>

//jrtplib
//RTPSession sess;
//uint16_t portbase, destport;
//uint32_t destip;
//int status, i, num;
//unsigned int timestamp_increse = 0, ts_current = 0;
//RTPUDPv4TransmissionParams transparams;
//RTPSessionParams sessparams;
//WSADATA dat;


// setup data
//
//注册时候的信息
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_NULL,            // Major type
	&MEDIASUBTYPE_NULL          // Minor type
};
//注册时候的信息
const AMOVIESETUP_PIN psudPins[] =
{
	{
		L"Input",           // String pin name
		FALSE,              // Is it rendered
		FALSE,              // Is it an output
		FALSE,              // Allowed none
		FALSE,              // Allowed many
		&CLSID_NULL,        // Connects to filter
		L"Output",          // Connects to pin
		1,                  // Number of types
		&sudPinTypes }
};

//注册时候的信息
const AMOVIESETUP_FILTER sudFilter =
{
	&CLSID_RTPFilter,       // Filter CLSID
	L"RTP Renderer Filter",    // Filter name
	MERIT_DO_NOT_USE,        // Its merit
	1,                       // Number of pins
	psudPins                 // Pin details
};

// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
//注意g_Templates名称是固定的
CFactoryTemplate g_Templates[1] =
{
	{
		L"RTP Renderer Filter",
		&CLSID_RTPFilter,
		RTPFilter::CreateInstance,
		NULL,
		&sudFilter
	}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// ----------------------------------------------------------------------------
//            Filter implementation
// ----------------------------------------------------------------------------
RTPFilter::RTPFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CTransInPlaceFilter(tszName, punk, CLSID_RTPFilter, phr)
{
	//初始化JRTPLIB，传入端口和目标IP
	Initjrtp(8000, "172.20.111.200", 9000);
	//本机端口——接收端IP——接收端端口
}
	

RTPFilter::~RTPFilter()
{
	Cleanjrtp();
}

CUnknown * WINAPI RTPFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	RTPFilter *pNewObject = new RTPFilter(NAME("RTP Renderer Filter"), punk, phr);
	if (pNewObject == NULL)
	{
		*phr = E_OUTOFMEMORY;
	}
	return pNewObject;
}
//*****************************DoRenderSample——发送RTP*****************************
//DoRenderSample——发送RTP
//*********************************主要的实现*********************************
HRESULT RTPFilter::Transform(IMediaSample *pSample)
{
	RTPSender(pSample);//调用发包函数，具体实现在后方
	return NOERROR;
}
// Basic COM - used here to reveal our own interfaces
//暴露接口，使外部程序可以QueryInterface，该部分可以添加更改IP的接口
STDMETHODIMP RTPFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	CheckPointer(ppv, E_POINTER);
	return CTransInPlaceFilter::NonDelegatingQueryInterface(riid, ppv);
} // NonDelegatingQueryInterface

HRESULT RTPFilter::CheckInputType(const CMediaType* mtIn)
{
	// Dynamic format change will never be allowed!
	if (IsStopped() && *mtIn->Type() == MEDIATYPE_Video)
	{
		if (*mtIn->Subtype() == MEDIASUBTYPE_H264)
		{
			return NOERROR;
		}
	}
	return E_INVALIDARG;
}
HRESULT RTPFilter::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
	HRESULT hr = CTransInPlaceFilter::CompleteConnect(direction, pReceivePin);
	return hr;
}

//*********************filter运行与结束函数****************************
//可以在这里进行jrtplib的初始化与析构
//非必须实现函数
//*********************************************************************
//HRESULT RTPFilter::StartStreaming()
//{
//	BOOL pass = mOverlayController->StartTitleOverlay();
//	return pass ? S_OK : E_FAIL;
//}
//
//HRESULT RTPFilter::StopStreaming()
//{
//	mOverlayController->StopTitleOverlay();
//	return NOERROR;
//}

//******************************JRTP******************************
//JRTP发包部分
//******************************JRTP******************************
void RTPFilter::checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		exit(-1);
	}
}
//******************************初始化******************************
bool RTPFilter::Initjrtp(uint16_t m_portbase, std::string m_destip, uint16_t m_destport)
{
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // RTP_SOCKETTYPE_WINSOCK
	std::string ipstr = m_destip;
	std::cout << "Using version " << RTPLibraryVersion::GetVersion().GetVersionString() << std::endl;
	portbase = m_portbase;
	destip = inet_addr(ipstr.c_str());
	destport = m_destport;
	if (destip == INADDR_NONE)
	{
		std::cerr << "Bad IP address specified" << std::endl;
		return FALSE;
	}
	// The inet_addr function returns a value in network byte order, but
	// we need the IP address in host byte order, so we use a call to
	// ntohl
	destip = ntohl(destip);
	std::cout << portbase << " " << destport << " " << ipstr << std::endl;//控制台打印IP信息
	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// In this case, we'll be sending 10 samples each second, so we'll
	// put the timestamp unit to (1.0/10.0)
	sessparams.SetOwnTimestampUnit(1.0 / 90000.0);//发包频率
	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(portbase);
	status = sess.Create(sessparams, &transparams);
	checkerror(status);
	RTPIPv4Address addr(destip, destport);
	status = sess.AddDestination(addr);
	sess.SetDefaultPayloadType(96);//装载类型为96时，为音视频混叠数据，具体见协议
	sess.SetDefaultMark(false);//设置session的缺省配置，mark为false
	sess.SetDefaultTimestampIncrement(90000.0 / 25.0); // 时间戳增长速度（一秒25帧，即一次NALU传送后，时间戳增长长度）
	checkerror(status);
	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// In this case, we'll be sending 10 samples each second, so we'll
	// put the timestamp unit to (1.0/10.0)

}
//******************************清除释放******************************
//JRTP释放函数，同时完成NALU结构体的释放
bool RTPFilter::Cleanjrtp()
{
	sess.BYEDestroy(RTPTime(10, 0), 0, 0);
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK
	return TRUE;
}
//******************************发包程序******************************
bool RTPFilter::RTPSender(IMediaSample *pMediaSample)
{
	long size = pMediaSample->GetActualDataLength();
	BYTE *pb=NULL;
	if (FAILED(pMediaSample->GetPointer(&pb)))
	{
		return 0;
	}
	//测试用写入点
	//FILE *fp;
	//fp = fopen("G://all.txt","w");
	//fwrite(pb, size, 1, fp);
	//fclose(fp)*/;
	//
	int cout = 0;//计起始码字长
	unsigned char a;//保存NALU头
	a = *pb;
	//startcode为 0x00000001(四位)或0x000001(三位)
	//x264 codec编码器输出为四位startcode
	while (a == 0)//判断0x00，char型默认0x00为null
	{
		pb++;
		a=*pb;
		cout++;
	}
	if (a == 1)//判断0x01
	{
		cout++;
		pb++;
	}
	else
	{
		return 0;
	}
	a = *pb;//取到header 8位/1字节（TYPE——NRI——F）
	pb++;//去除了起始位4字节+头1字节,*pb指向NALU数据部分
	int startcode_len = cout;
	long len = size;//NALU大小（包括 起始位 头）
	long actual_len = size - cout-1;//去除起始位4位 size
	NALU_HEADER nh;//NALU头 结构体
	//NALU头格式—
	//高——低位
	//F-NRI-TYPE
	//1--2----5 bit
	nh.TYPE = a;//结构体截断字符a的低5位
	nh.NRI = a >> 5;//a左移5位，取第6、7位
	nh.F = a >> 2;//a左移1位，取得第8位
	//*************RTP单包********************
	//当NALU单元小于AX_RTP_PKT_LENGTH（1360字节）时，使用一个RTP发送一帧
	//本程序不进行多帧单包发送
	//每一个NALU发送完成后，在拆分的最后的RTP包中需要 置 mark位为true，接收端根据RTP包是否有mark识别是否进行组包
	//发送频率为25帧=3600ms，与session的频率设置统一
	//RTP分包无需延迟，仅在最后一包及单包进行延迟设置
	//*************RTP单包********************

	if (actual_len <= MAX_RTP_PKT_LENGTH)
	{
		memset(sendbuf, 0, 1500);
		//设置NALU HEADER,并将这个HEADER填入sendbuf[12]
		nalu_hdr = (NALU_HEADER*)&sendbuf[0]; //将sendbuf[12]的地址赋给nalu_hdr，之后对nalu_hdr的写入就将写入sendbuf中；
		nalu_hdr->F = nh.F;
		nalu_hdr->NRI = nh.NRI;//有效数据在n->nal_reference_idc的第6，7位，需要右移5位才能将其值赋给nalu_hdr->NRI。
		nalu_hdr->TYPE = nh.TYPE;
		nalu_payload = &sendbuf[1];//同理将sendbuf[13]赋给nalu_payload
		memcpy(nalu_payload, pb, actual_len);//去掉nalu头的nalu剩余内容写入sendbuf[13]开始的字符串。
		status = sess.SendPacket((void *)sendbuf, actual_len, 96, true, 3600);
		//发送RTP格式数据包并指定负载类型为96
		if (status < 0)
		{
			std::cerr << RTPGetErrorString(status) << std::endl;
			exit(-1);
		}
	}
	else if (actual_len>MAX_RTP_PKT_LENGTH)
	{
		//得到该nalu需要用多少长度为MAX_RTP_PKT_LENGTH字节的RTP包来发送
		int k = 0, l = 0;
		k = actual_len / MAX_RTP_PKT_LENGTH;//需要k个MAX_RTP_PKT_LENGTH字节的RTP包
		l = actual_len%MAX_RTP_PKT_LENGTH;//最后一个RTP包的需要装载的字节数
		int t = 0;//用于指示当前发送的是第几个分片RTP包

		//*************RTP打包循环********************
		//此部分主要实现对NALU拆分工作，单个RTP包最大大小不超过MAX_RTP_PKT_LENGTH（1360字节）
		//每一个NALU发送完成后，在拆分的最后的RTP包中需要 置 mark位为true，接收端根据RTP包是否有mark识别是否进行组包
		//发送频率为25帧=3600ms，与session的频率设置统一
		//RTP分包无需延迟，仅在最后一包及单包进行延迟设置
		//延迟设置将影响接收端的接收速率和播放
		//********************************************

		while (t <= k)
		{
			if (!t)//发送一个需要分片的NALU的第一个分片，置FU HEADER的S位
			{
				//printf("dddd1");
				memset(sendbuf, 0, 1500);
				//session.SetDefaultMark(false);
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
				fu_ind = (FU_INDICATOR*)&sendbuf[0]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nh.F;
				fu_ind->NRI = nh.NRI;
				fu_ind->TYPE = 28;
				//设置FU HEADER,并将这个HEADER填入sendbuf[13]
				fu_hdr = (FU_HEADER*)&sendbuf[1];
				fu_hdr->E = 0;
				fu_hdr->R = 0;
				fu_hdr->S = 1;
				fu_hdr->TYPE = nh.TYPE;				
				nalu_payload = &sendbuf[2];//同理将sendbuf[2]赋给nalu_payload
				memcpy(nalu_payload, pb, MAX_RTP_PKT_LENGTH);//去掉NALU头
				status = sess.SendPacket((void *)sendbuf, MAX_RTP_PKT_LENGTH + 2, 96, false, 0);
				//测试用写入点
				//FILE *fp;
				//fp = fopen("G://get.txt", "a+");
				//fwrite(sendbuf+2, MAX_RTP_PKT_LENGTH, 1, fp);
				//fwrite((void*)"\n", 1, 1, fp);
				//fclose(fp);
				//std::cout << sendbuf << std::endl;
				if (status < 0)
				{
					std::cerr << RTPGetErrorString(status) << std::endl;
					exit(-1);
				}
				t++;
			}
			//发送一个需要分片的NALU的非第一个分片，清零FU HEADER的S位，如果该分片是该NALU的最后一个分片，置FU HEADER的E位
			else if (k == t&&l!=0)//发送的是最后一个分片，注意最后一个分片的长度可能超过MAX_RTP_PKT_LENGTH字节（当l>1386时）。
			{
				//printf("dddd3\n");
				memset(sendbuf, 0, 1500);
				//session.SetDefaultMark(true);
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[0]
				fu_ind = (FU_INDICATOR*)&sendbuf[0]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nh.F;
				fu_ind->NRI = nh.NRI;
				fu_ind->TYPE = 28;
				//设置FU HEADER,并将这个HEADER填入sendbuf[1]
				fu_hdr = (FU_HEADER*)&sendbuf[1];
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->TYPE = nh.TYPE;
				fu_hdr->E = 1;
				nalu_payload = &sendbuf[2];//同理将sendbuf[2]地址赋给nalu_payload
				memcpy(nalu_payload, pb + t*MAX_RTP_PKT_LENGTH , l);//将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。
				status = sess.SendPacket((void *)sendbuf, l + 2, 96, true, 3600);
				//测试用写入点
				//FILE *fp;
				//fp = fopen("G://get.txt", "a+");
				//fwrite(sendbuf+2, MAX_RTP_PKT_LENGTH, 1, fp);
				//fwrite((void*)"\n", 1, 1, fp);
				//fclose(fp);
				if (status < 0)
				{
					std::cerr << RTPGetErrorString(status) << std::endl;
					exit(-1);
				}
				t++;
				//	Sleep(100);
			}
			else if (t<k && 0 != t)
			{
				memset(sendbuf, 0, 1500);
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[0]
				fu_ind = (FU_INDICATOR*)&sendbuf[0]; //将sendbuf[0]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nh.F;
				fu_ind->NRI = nh.NRI;
				fu_ind->TYPE = 28;
				//设置FU HEADER,并将这个HEADER填入sendbuf[1]
				fu_hdr = (FU_HEADER*)&sendbuf[1];
				//fu_hdr->E=0;
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->E = 0;
				fu_hdr->TYPE = nh.TYPE;
				nalu_payload = &sendbuf[2];//同理将sendbuf[2]的地址赋给nalu_payload
				memcpy(nalu_payload, pb + t*MAX_RTP_PKT_LENGTH , MAX_RTP_PKT_LENGTH);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。
				status = sess.SendPacket((void *)sendbuf, MAX_RTP_PKT_LENGTH + 2, 96, false, 0);
				//测试用写入点
				//FILE *fp;
				//fp = fopen("G://get.txt", "a+");
				//fwrite(sendbuf+2, MAX_RTP_PKT_LENGTH, 1, fp);
				//fwrite((void*)"\n", 1, 1, fp);
				//fclose(fp);
				if (status < 0)
				{
					std::cerr << RTPGetErrorString(status) << std::endl;
					exit(-1);
				}
				t++;
			}
			else if (k == t&&l == 0)
			{
				t++;
			}
		}
	}	
	RTPTime delay(0.0015); //发包延迟（用于调整发送速率和帧速率的差异）非必须，请根据自身情况调整
	RTPTime::Wait(delay);
}

/******************************Public Routine******************************\
* exported entry points for registration and
* unregistration (in this case they only call
* through to default implmentations).
*
*
*
* History:
*
\**************************************************************************/
//****************************DLL注册入口***********************************
//DllEntryPoint
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD dwReason,
	LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

//extern "C" _declspec(dllexport)BOOL DLLRegisterServer;

