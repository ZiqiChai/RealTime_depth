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
		cout << "rect: " << select.x << " " << select.y << " " << select.width << " " << select.height << endl;
	}
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		mousedown_flag = true;
		select_flag = false;
		origin = Point(x, y);
		select = Rect(x, y, 0, 0);           //����һ��Ҫ��ʼ������͸�Ϊ(0,0)����Ϊ��opencv��Rect���ο����ڵĵ��ǰ������Ͻ��Ǹ���ģ����ǲ������½��Ǹ���
	}
	else if (event == CV_EVENT_LBUTTONUP)
	{
		mousedown_flag = false;
		select_flag = true;
	}
}


void ave_depth(Rect& select, const k4a_image_t& depthImage)
{
	const int width = k4a_image_get_width_pixels(depthImage);
	const int height = k4a_image_get_height_pixels(depthImage);
	int bon_w = select.x + select.width;
	int bon_h = select.y + select.height;
	const uint16_t* depthData = reinterpret_cast<const uint16_t*>(k4a_image_get_buffer(depthImage));
	unsigned long sum_depth = 0;
	if (select.y != bon_h && select.x != bon_w) {
		for (int h = select.y; h <= bon_h; ++h)
		{
			for (int w = select.x; w <= bon_w; ++w)
			{
				const size_t currentPixel = static_cast<size_t>(h * width + w);
				sum_depth += (unsigned long)depthData[currentPixel];
				if (h == bon_h && w == bon_w) {
					long points_num = (select.width + 1) * (select.height + 1);
					long ave_depth = sum_depth / points_num;
					cout << ave_depth << endl;
				}
			}
		}
	}

}


int main(int argc, char** argv)
{
	const uint32_t deviceCount = k4a_device_get_installed_count();
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
	k4a_device_t device = nullptr;
	k4a_device_open(0, &device);
	k4a_device_start_cameras(device, &config);
	cout << "Finished opening K4A device." << endl;

	std::vector<Pixel> depthTextureBuffer;

	k4a_capture_t capture;

	k4a_image_t depthImage;

	cv::Mat depthFrame;

	namedWindow("kinect depth map master", 1);
	setMouseCallback("kinect depth map master", onMouse, 0);

	while (1)
	{
		if (k4a_device_get_capture(device, &capture, K4A_WAIT_INFINITE) == K4A_WAIT_RESULT_SUCCEEDED)
		{
			{
				depthImage = k4a_capture_get_depth_image(capture);

				ColorizeDepthImage(depthImage, DepthPixelColorizer::ColorizeBlueToRed, GetDepthModeRange(config.depth_mode), &depthTextureBuffer);

				depthFrame = cv::Mat(k4a_image_get_height_pixels(depthImage), k4a_image_get_width_pixels(depthImage), CV_8UC4, depthTextureBuffer.data());
			
				//�������ο�
				rectangle(depthFrame, select, Scalar(0, 0, 255), 1, 8, 0);//�ܹ�ʵʱ��ʾ�ڻ����δ���ʱ�ĺۼ�
				ave_depth(select, depthImage);//�����ѡ�����ƽ�����

				cv::imshow("kinect depth map master", depthFrame);

				k4a_image_release(depthImage);
			}

		}
		if (waitKey(30) == 27 || waitKey(30) == 'q')
		{
			k4a_capture_release(capture);
			k4a_device_close(device);
			break;
		}
	}
	return 0;
}