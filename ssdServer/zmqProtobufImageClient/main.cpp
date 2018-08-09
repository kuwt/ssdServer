// zmq + protobuf
// client using zmq for protobuf message
//

#include "ssdService.h"

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
	return 0;
}

int main()
{
	system("pause");
	// init
	SSDservice service;
	// read image

	while (1)
	{
		cv::Mat Imgdata;
		{
			std::string imagePath;
			std::cout << "1|2|3|4|5" << "\n";
			int imageId;
			std::cin >> imageId;
			imagePath = std::string("0") + std::to_string(imageId) + ".bmp";
			Imgdata = cv::imread(imagePath, CV_LOAD_IMAGE_COLOR);
		}

		// ssd
		std::vector<ObjectInfo> vObjectInfos;
		service.submitService(Imgdata, 2, vObjectInfos);

		// show result
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
		// timer
		{

		}
	}
	system("pause");
	return 0;
}