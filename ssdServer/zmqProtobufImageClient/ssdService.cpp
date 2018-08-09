
#include "ssdService.h"
#include <string.h>
#include <stdio.h>
#include "objectInfos.pb.h"
#include <chrono>  // for high_resolution_clock
#include <math.h>


/**************************
Util Service
************************/

static int GetReductionFactorFromPyramidLevel(const int  pyramid_level)
{
	int ReductionFactor = 1;
	switch (pyramid_level)
	{
	case 0:
		ReductionFactor = 1;
		break;
	case 1:
		ReductionFactor = 2;
		break;
	case 2:
		ReductionFactor = 4;
		break;
	case 3:
		ReductionFactor = 8;
		break;
	case 4:
		ReductionFactor = 16;
		break;
	case 5:
		ReductionFactor = 32;
		break;
	case 6:
		ReductionFactor = 64;
		break;
	case 7:
		ReductionFactor = 128;
		break;
	default:
		ReductionFactor = 1;
	}
	return ReductionFactor;
}

void FormImgPyramid_With_Smoothing(
	const cv::Mat &img,
	int HighestLvL,
	std::vector<cv::Mat> &img_query_pyramids)
{
	for (int i = 0; i <= HighestLvL; ++i)
	{
		cv::Mat img_query_py;
		int reduction = GetReductionFactorFromPyramidLevel(i);
		if (reduction != 1)
		{
			cv::pyrDown(img_query_pyramids.at(i - 1), img_query_py, cv::Size(img_query_pyramids.at(i - 1).cols / 2, img_query_pyramids.at(i - 1).rows / 2));
		}
		else
		{
			cv::blur(img, img_query_py, cv::Size(3, 3));
		}

		img_query_pyramids.push_back(img_query_py);
	}
}

SSDservice::SSDservice(const std::string &addr_)
{
	startService(addr_);
}

SSDservice::~SSDservice()
{
	closeService();
	delete m_pSock;
}

int SSDservice::startService(const std::string &addr_)
{
	printf("Connecting to server \n");
	m_context = zmq::context_t(1);
	m_pSock = new zmq::socket_t(m_context, ZMQ_REQ);
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	m_pSock->connect(addr_);
	return 0;
}
int SSDservice::submitService(const cv::Mat &inputImage, const int reductionFactor, std::vector<ObjectInfo> &infos)
{
	/***********************
	//  Serialize
	**********************/
	std::vector<uchar> data_encode;
	int zoomOutLevel = reductionFactor;
	{
		std::vector<cv::Mat> imagePyList;
		FormImgPyramid_With_Smoothing(inputImage, zoomOutLevel, imagePyList);
		cv::imencode(".bmp", imagePyList.at(zoomOutLevel), data_encode);
	}

	/***********************
	//  Send
	**********************/
	{
		zmq::message_t message(data_encode.size());
		memcpy(message.data(), data_encode.data(), data_encode.size());
		m_pSock->send(message);
	}
	/***********************
	// get reply
	**********************/
	zmq::message_t reply;
	m_pSock->recv(&reply, 0);
	std::string  msgStr = std::string((char*)reply.data(), reply.size());
	//unserialize
	proto::objectInfos MsgObjectInfos;
	MsgObjectInfos.ParseFromString(msgStr);
	//scale up
	std::vector<ObjectInfo> vObjectInfos;
	for (int i = 0; i < MsgObjectInfos.infos_size(); ++i)
	{
		ObjectInfo info;
		info.objclass = MsgObjectInfos.infos(i).objclass();
		info.score = MsgObjectInfos.infos(i).score();
		info.tlx = MsgObjectInfos.infos(i).tlx()  * std::pow(2, zoomOutLevel);
		info.tly = MsgObjectInfos.infos(i).tly() * std::pow(2, zoomOutLevel);
		info.brx = MsgObjectInfos.infos(i).brx() * std::pow(2, zoomOutLevel);
		info.bry = MsgObjectInfos.infos(i).bry() * std::pow(2, zoomOutLevel);
		vObjectInfos.push_back(info);
	}

	/***********************
	// assign result
	**********************/
	infos = vObjectInfos;
	return 0;
}
int SSDservice::closeService()
{
	m_pSock->close();
	return 0;
}