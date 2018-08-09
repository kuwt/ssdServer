// zmq + protobuf
// client using zmq for protobuf message
//

#include <zmq.hpp>
#include <string.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "objectInfos.pb.h"
#include <chrono>  // for high_resolution_clock
#include <math.h>
struct ObjectInfo
{
	int objclass = 0;
	float score = 0;
	int tlx = 0;
	int tly = 0;
	int brx = 0;
	int bry = 0;
};
int showImg(std::string windowName, cv::Mat img, int maxHeightShown, int maxWidthShown)
{
	if (img.empty())
	{
		std::cout << "no image" << "\n";
		return -1;
	}

	cv::Mat showImg = img;
	if (showImg.size().height > maxHeightShown)
	{
		float newHeight = maxHeightShown;
		float fy = newHeight / (float)showImg.size().height;
		float fx = fy;
		cv::resize(showImg, showImg, cv::Size(), fx, fy, cv::INTER_CUBIC);
	}
	if (showImg.size().width > maxWidthShown)
	{
		float newwidth = maxWidthShown;
		float fx = newwidth / (float)showImg.size().height;
		float fy = fx;
		cv::resize(showImg, showImg, cv::Size(), fx, fy, cv::INTER_CUBIC);
	}

	cv::imshow(windowName, showImg);
	cv::waitKey(1);
}

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
int main(void)
{
	zmq::context_t context(1);

	/***********************
	//  Socket to talk to server
	**********************/
	auto startInit = std::chrono::high_resolution_clock::now();
	printf("Connecting to server \n");
	zmq::socket_t sock(context, ZMQ_REQ);
	sock.connect("tcp://localhost:5555");
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	auto finishInit = std::chrono::high_resolution_clock::now();
	auto startWholeProcess = std::chrono::high_resolution_clock::now();
	/***********************
	//  Serialize
	**********************/
	auto startEncode = std::chrono::high_resolution_clock::now();
	std::vector<uchar> data_encode;
	cv::Mat Imgdata;
	int zoomOutLevel = 2;
	{
		std::string imagePath;
		std::cout << "1|2|3|4|5" << "\n";
		int imageId;
		std::cin >> imageId;
		imagePath = std::string("0") + std::to_string(imageId) + ".bmp";
		Imgdata = cv::imread(imagePath, CV_LOAD_IMAGE_COLOR);

		std::vector<cv::Mat> imagePyList;
		FormImgPyramid_With_Smoothing(Imgdata, zoomOutLevel, imagePyList);
		cv::imencode(".bmp", imagePyList.at(zoomOutLevel), data_encode);
		//cv::imencode(".bmp", Imgdata, data_encode);
	}
	auto finishEncode = std::chrono::high_resolution_clock::now();
	
	/***********************
	// send
	**********************/
	auto startSend = std::chrono::high_resolution_clock::now();
	{
		zmq::message_t message(data_encode.size());
		memcpy(message.data(), data_encode.data(), data_encode.size());
		sock.send(message);
	}
	auto finishSend = std::chrono::high_resolution_clock::now();

	/***********************
	// get reply
	**********************/
	zmq::message_t reply;
	sock.recv(&reply, 0);
	std::string  msgStr = std::string((char*)reply.data(), reply.size());
	auto endWholeProcess = std::chrono::high_resolution_clock::now();
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
	
	{
		for (int i = 0; i < vObjectInfos.size(); ++i)
		{
			std::cout << vObjectInfos.at(i).objclass << " "
				<< vObjectInfos.at(i).score << " "
				<< vObjectInfos.at(i).tlx << " "
				<< vObjectInfos.at(i).tly << " "
				<< vObjectInfos.at(i).brx << " "
				<< vObjectInfos.at(i).bry << " "
				<< "\n";
		}

		cv::Mat shownImage;
		Imgdata.copyTo(shownImage);

		for (int i = 0; i < vObjectInfos.size(); ++i)
		{
			cv::Rect r = cv::Rect(
				vObjectInfos.at(i).tlx,
				vObjectInfos.at(i).tly,
				vObjectInfos.at(i).brx - vObjectInfos.at(i).tlx,
				vObjectInfos.at(i).bry - vObjectInfos.at(i).tly);
			cv::rectangle(shownImage, r, cv::Scalar(255, 0, 0), 5, 8, 0);
		}

		showImg("result", shownImage, 800, 800);
	}
	std::chrono::duration<double> elapsedSerialize = finishEncode - startEncode;
	std::cout << "serialize process time elapsed = " << elapsedSerialize.count() << "\n";
	std::chrono::duration<double> elapsedSend = finishSend - startSend;
	std::cout << "send time elapsed = " << elapsedSend.count() << "\n";
	std::chrono::duration<double> elapsedWholeProcess = endWholeProcess - startWholeProcess;
	std::cout << "whole process time elapsed = " << elapsedWholeProcess.count() << "\n";
	std::chrono::duration<double> elapsedInit = finishInit - startInit;
	std::cout << "Init time elapsed = " << elapsedInit.count() << "\n";
	sock.close();
	system("pause");
	return 0;
}