// based on https://github.com/vaibhav06891/SlowMotion

#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/gpu/gpu.hpp"
#include <iostream>
#include <fstream>

#define CLAMP(x,min,max) (  ((x) < (min)) ? (min) : ( ((x) > (max)) ? (max) : (x) )  )

int main(int argc, char** argv)
{
	cv::Mat frame, prevframe;
	cv::Mat prevgray, gray;
	cv::Mat fflow, bflow;

	std::string videoName = argv[1];  // the video filename should be given as the arguement while executing the program
	std::string output = argv[2];
	cv::VideoCapture capture(videoName);

	if (!capture.isOpened()) {
		std::cout << "ERROR: Failed to open the video" << std::endl;
		return -1;
	}
	int fourcc = static_cast<int>(capture.get(CV_CAP_PROP_FOURCC));

	char FOURCC_STR[] = {
		(char)(fourcc & 0XFF)
		, (char)((fourcc & 0XFF00) >> 8)
		, (char)((fourcc & 0XFF0000) >> 16)
		, (char)((fourcc & 0XFF000000) >> 24)
		, 0
	};

	//--------------------initialize video writer object---------------------------------------
	double dWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
	double dHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video
	int fps = (int)capture.get(CV_CAP_PROP_FPS);
	std::cout << "motioncomp: Input FOURCC='" << FOURCC_STR << "' " << dWidth << "x" << dHeight << " " << fps << std::endl;

//	fourcc = CV_FOURCC('I', '4', '2', '0');

	cv::Size frameSize(static_cast<int>(dWidth), static_cast<int>(dHeight));
	cv::VideoWriter oVideoWriter (output, 0, fps, frameSize, true); //initialize the VideoWriter object

	if ( !oVideoWriter.isOpened() ) //if not initialize the VideoWriter successfully, exit the program
	{	
		std::cout << "motioncomp: ERROR Failed to write the video" << std::endl;
		return -1;
	}

	std::cout << "motioncomp: start conversion" << std::endl;

	capture >> frame;

	if (!frame.empty()) {

		cv::Mat flowf(frame.rows, frame.cols , CV_8UC3); // the forward co-ordinates for interpolation
		flowf.setTo(cv::Scalar(255, 255, 255));
		cv::Mat flowb(frame.rows, frame.cols , CV_8UC3); // the backward co-ordinates for interpolation
		flowb.setTo(cv::Scalar(255, 255, 255));
		cv::Mat final(frame.rows, frame.cols , CV_8UC3);

		int fx, fy, bx, by;

		for (;;)
		{
			capture >> frame;
			if (frame.empty())
				break;

			if (!prevframe.empty())
			{
				cv::cvtColor(prevframe, prevgray, cv::COLOR_BGR2GRAY); // Convert to gray space for optical flow calculation
				cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
				cv::calcOpticalFlowFarneback(prevgray, gray, fflow, 0.5, 3, 15, 3, 3, 1.2, 0);  // forward optical flow
				cv::calcOpticalFlowFarneback(gray, prevgray, bflow, 0.5, 3, 15, 3, 3, 1.2, 0);   //backward optical flow

				for (int t = 0; t < 4; t++) // interpolating 20 frames from two given frames at different time locations
				{
					// less than original rows and columns scanned to avoid having co-ordinates outside the frame size
					for (int y = 0; y < frame.rows; y++) //column scan
					{
						for (int x = 0; x < frame.cols; x++) //row scan
						{
							const cv::Point2f fxy = fflow.at<cv::Point2f>(y, x);
							fy = CLAMP(y + fxy.y * 0.25 * t, 0, 780);
							fx = CLAMP(x + fxy.x * 0.25 * t, 0, 1280);

							flowf.at<cv::Vec3b>(fy, fx) = prevframe.at<cv::Vec3b>(y, x);

							const cv::Point2f bxy = bflow.at<cv::Point2f>(y, x);
							by = CLAMP(y + bxy.y * (1 - 0.25 * t), 0, 780);
							bx = CLAMP(x + bxy.x * (1 - 0.25 * t), 0, 1280);
							flowb.at<cv::Vec3b>(by, bx) = frame.at<cv::Vec3b>(y, x);
						}
					}
					final = flowf * (1 - 0.25 * t) + flowb * 0.25 * t; //combination of frwd and bckward martrix
					cv::medianBlur(final, final, 3);
					std::cout << "motioncomp: write frame"<<std::endl;
					oVideoWriter.write(final);
				}
			}

			swap(frame, prevframe);

		}
	}
	capture.release();
	oVideoWriter.release();
	std::cout << "motioncomp: end conversion" << std::endl;
	exit(0);
}
