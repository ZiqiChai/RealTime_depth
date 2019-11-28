#include <k4a/k4a.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <array>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


#include "Pixel.h"
#include "DepthPixelColorizer.h"
#include "StaticImageProperties.h"

using namespace std;
using namespace cv;
using namespace sen;

Rect select;
bool mousedown_flag = false; //��갴�µı�ʶ��
bool select_flag = false;    //ѡ������ı�ʶ��
Point origin;
Mat frame;


void onMouse(int event, int x, int y, int, void*)
{
	//ע��onMouse��void���͵ģ�û�з���ֵ��
	//Ϊ�˰���Щ������ֵ��������������Щ������������Ϊȫ�ֱ���	
	if (mousedown_flag)
	{
		select.x = MIN(origin.x, x);     //��һ��Ҫ����굯��ż�����ο򣬶�Ӧ������갴�¿�ʼ���������ʱ��ʵʱ������ѡ���ο�
		select.y = MIN(origin.y, y);
		select.width = abs(x - origin.x);                  //����ο�Ⱥ͸߶�
		select.height = abs(y - origin.y);
		//select &= Rect(0, 0, frame.cols, frame.rows);       //��֤��ѡ���ο�����Ƶ��ʾ����֮��
		cout << "rect: " << select.x << " " << select.y << " " << select.width << " " << select.height << endl;
	}
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		//cout << "initialize" << endl;
		mousedown_flag = true;
		select_flag = false;
		origin = Point(x, y);
		//cout << "initialize_x&y: " << x << " " << y << endl;
		select = Rect(x, y, 0, 0);           //����һ��Ҫ��ʼ������͸�Ϊ(0,0)����Ϊ��opencv��Rect���ο����ڵĵ��ǰ������Ͻ��Ǹ���ģ����ǲ������½��Ǹ���
	}
	else if (event == CV_EVENT_LBUTTONUP)
	{
		//cout << "BUTTON_UP" << endl;
		mousedown_flag = false;
		select_flag = true;
	}
}

void ave_depth(unsigned long sum_depth, int width, int height) {
	long points_num = (width + 1) * (height + 1);
	long ave_depth = sum_depth / points_num;
	cout << ave_depth << endl;
}

void sum_depth(Rect &select, const k4a::image& depthImage)
{
	const int width = depthImage.get_width_pixels();
	const int height = depthImage.get_height_pixels();
	int bon_w = select.x + select.width;
	int bon_h = select.y + select.height;
	const uint16_t* depthData = reinterpret_cast<const uint16_t*>(depthImage.get_buffer());
	unsigned long sum_depth = 0;
	if (select.y != bon_h && select.x != bon_w) {
		for (int h = select.y; h <= bon_h; ++h)
		{
			for (int w = select.x; w <= bon_w; ++w)
			{
				const size_t currentPixel = static_cast<size_t>(h * width + w);
				sum_depth += (unsigned long)depthData[currentPixel];
				if (h == bon_h && w == bon_w) {
					ave_depth(sum_depth, select.width, select.height);
				}
			}
		}
	}
	
}


int main(int argc, char **argv)
{
	const uint32_t deviceCount = k4a::device::get_installed_count();
	if (deviceCount == 0)
	{
		cout << "no azure kinect devices detected!" << endl;
	}

	k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
	config.camera_fps = K4A_FRAMES_PER_SECOND_15;
	config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
	config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
	config.color_resolution = K4A_COLOR_RESOLUTION_720P;
	config.synchronized_images_only = true;

	cout << "Started opening K4A device..." << endl;
	k4a::device device = k4a::device::open(K4A_DEVICE_DEFAULT);
	device.start_cameras(&config);
	cout << "Finished opening K4A device." << endl;

	std::vector<Pixel> depthTextureBuffer;
	//std::vector<Pixel> irTextureBuffer;
	uint8_t *colorTextureBuffer;

	k4a::capture capture;

	k4a::image depthImage;
	k4a::image colorImage;
	//k4a::image irImage;

	cv::Mat depthFrame;
	cv::Mat colorFrame;
	//cv::Mat irFrame;

	namedWindow("kinect depth map master", 1);
	setMouseCallback("kinect depth map master", onMouse, 0);

	while (1)
	{
		if (device.get_capture(&capture, std::chrono::milliseconds(0)))
		{
			{
				depthImage = capture.get_depth_image();
				colorImage = capture.get_color_image();
				//irImage = capture.get_ir_image();

				ColorizeDepthImage2(depthImage, DepthPixelColorizer::ColorizeBlueToRed, GetDepthModeRange(config.depth_mode), &depthTextureBuffer);
				//ColorizeDepthImage(irImage, DepthPixelColorizer::ColorizeGreyscale, GetIrLevels(K4A_DEPTH_MODE_PASSIVE_IR), &irTextureBuffer);
				colorTextureBuffer = colorImage.get_buffer();

				depthFrame = cv::Mat(depthImage.get_height_pixels(), depthImage.get_width_pixels(), CV_8UC4, depthTextureBuffer.data());
				colorFrame = cv::Mat(colorImage.get_height_pixels(), colorImage.get_width_pixels(), CV_8UC4, colorTextureBuffer);
				//irFrame = cv::Mat(irImage.get_height_pixels(), irImage.get_width_pixels(), CV_8UC4, irTextureBuffer.data());


				//cal_Depth(select, depthImage);
				//�������ο�
				rectangle(depthFrame, select, Scalar(0, 0, 255), 1, 8, 0);//�ܹ�ʵʱ��ʾ�ڻ����δ���ʱ�ĺۼ�
				sum_depth(select,depthImage);

				cv::imshow("kinect depth map master", depthFrame);
				//cv::imshow("kinect color frame master", colorFrame);
				//cv::imshow("kinect ir frame master", irFrame);

				
			}

		}
		if (waitKey(30) == 27 || waitKey(30) == 'q')
		{
			device.close();
			break;
		}
	}
	return 0;
}