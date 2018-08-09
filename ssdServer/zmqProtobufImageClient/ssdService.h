#pragma once
#include <opencv2/opencv.hpp>
#include <zmq.hpp>

struct ObjectInfo
{
	int objclass = 0;
	float score = 0;
	int tlx = 0;
	int tly = 0;
	int brx = 0;
	int bry = 0;
};

class SSDservice
{
private:
	zmq::context_t m_context;
	zmq::socket_t *m_pSock;
	
	int closeService();
public:
	SSDservice(const std::string &addr_ = "tcp://localhost:5555");
	~SSDservice();
	int startService(const std::string &addr_ = "tcp://localhost:5555");
	int submitService(const cv::Mat &inputImage, const int reductionFactor, std::vector<ObjectInfo> &infos);
	
};