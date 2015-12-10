#include "ini.hpp"
#include <opencv2/opencv.hpp>
#include "OpenNI.h"



cv::Size size;

std::string screenshot_color_dir;
std::string screenshot_depth_dir;
std::string video_color_dir;
std::string video_depth_dir;

cv::VideoWriter *video_color_writer;
cv::VideoWriter *video_depth_writer;


using namespace openni;

int main(int argc, char *argv[])
{
    // Open INI config file
    std::ifstream file("../config.ini");
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    // INI parser
    INI::Parser p(buffer);
    std::stringstream out;
    p.dump(out);
    auto config = p.top();
    
    int width = std::stoi(config["width"]);
    int height = std::stoi(config["height"]);
    size = cv::Size(width, height);
    

    screenshot_color_dir = config["screenshot_color_dir"];
    screenshot_depth_dir = config["screenshot_depth_dir"];
    video_color_dir = config["video_color_dir"];
    video_depth_dir = config["video_depth_dir"];
	
	
	video_color_writer = new cv::VideoWriter(video_color_dir, CV_FOURCC('j','p','e','g'), 30, size, true);
	video_depth_writer = new cv::VideoWriter(video_depth_dir, CV_FOURCC('j','p','e','g'), 30, size, false);
	
    
    
	// Initialize OpenNI
    if (OpenNI::initialize() != STATUS_OK)
	{
        std::cerr << "OpenNI Initial Error: "  << OpenNI::getExtendedError() << std::endl;
    	return -1;
	}
  
	// Open Device
    Device device;
    if (device.open(ANY_DEVICE) != STATUS_OK)
	{
        std::cerr << "Can't Open Device: "  << OpenNI::getExtendedError() << std::endl;
    	return -1;
	}
  
	// Create depth stream
	VideoStream depth_stream;
    if (depth_stream.create(device, SENSOR_DEPTH) == STATUS_OK)
    {
	    // 3a. set video mode
    	VideoMode mode;
    	mode.setResolution(size.width, size.height);
    	mode.setFps(30);
    	mode.setPixelFormat(PIXEL_FORMAT_DEPTH_1_MM);

    	if (STATUS_OK != depth_stream.setVideoMode(mode))
		{
    		std::cout << "Can't apply VideoMode: " << OpenNI::getExtendedError() << std::endl;
		}
    }
    else
    {
    	std::cerr << "Can't create depth stream on device: " << OpenNI::getExtendedError() << std::endl;
    	return -1;
    }
  
	// Create color stream
	VideoStream color_stream;
	if (device.hasSensor(SENSOR_COLOR))
	{
    	if (STATUS_OK == color_stream.create(device, SENSOR_COLOR))
    	{
    		// 4a. set video mode
		    VideoMode mode;
    		mode.setResolution(size.width, size.height);
    		mode.setFps(30);
    		mode.setPixelFormat(PIXEL_FORMAT_RGB888);
            
    		if (STATUS_OK != color_stream.setVideoMode(mode))
			{
        		std::cout << "Can't apply VideoMode: " << OpenNI::getExtendedError() << std::endl;
			}
                
    		// 4b. image registration
    		if (device.isImageRegistrationModeSupported(IMAGE_REGISTRATION_DEPTH_TO_COLOR))
			{
        		device.setImageRegistrationMode(IMAGE_REGISTRATION_DEPTH_TO_COLOR);
			}
    	}
    	else
    	{
    		std::cerr << "Can't create color stream on device: " << OpenNI::getExtendedError() << std::endl;
    		return -1;
    	}
	}
	
	
    // Print help
	std::cout << "s - take and save screenshot \n"
        << "q - quit" << std::endl;
	

    // Init color and depth frames, start streames
	VideoFrameRef color_frame;
	VideoFrameRef depth_frame;
	depth_stream.start();
	color_stream.start();
	int max_depth = depth_stream.getMaxPixelValue();	
    
    // Read first frame
    if (color_stream.readFrame(&color_frame) != STATUS_OK)
    {
        std::cerr << "Can't read color frame." << std::endl;
        return -1;
    }
    if (depth_stream.readFrame(&depth_frame) != STATUS_OK)
    {
        std::cerr << "Can't read depth frame." << std::endl;
        return -1;
    }
    
    
    // Prepare while loop
	bool recording = true;
	while (recording)
	{
    	// Check possible errors
    	if (!color_stream.isValid())
    	{
    		std::cerr << "Color stream not valid" << std::endl;
    		break;
    	}
    	
    	if (STATUS_OK != color_stream.readFrame( &color_frame ))
    	{
    		std::cerr << "Can't read color frame'" << std::endl;
    		break;
    	}
    	
    	if (STATUS_OK != depth_stream.readFrame( &depth_frame ))
    	{
    		std::cerr << "Can't read depth frame'" << std::endl;
    		break;
    	}
    	
        
    	// Grab frames, result is frame and frame_depth
    	const cv::Mat color_frame_temp( size, CV_8UC3, (void*)color_frame.getData() );
        cv::Mat frame_color;
        cv::cvtColor(color_frame_temp, frame_color, CV_RGB2BGR);
        
    	const cv::Mat depth_frame_temp( size, CV_16UC1, (void*)depth_frame.getData() );
    	cv::Mat frame_depth;
    	depth_frame_temp.convertTo(frame_depth, CV_8U, 255. / max_depth);
		cv::Mat frame_depth_color;
		cv::cvtColor(frame_depth, frame_depth_color, CV_GRAY2BGR);
		
		video_color_writer->write(frame_color);
		video_depth_writer->write(frame_depth_color);
		
		
		cv::imshow("Color", frame_color);
		cv::imshow("Depth", frame_depth);
		
        
        // React on keyboard input
    	char key = cv::waitKey(10);
        switch (key)
        {
            case 's':
				cv::imwrite(screenshot_color_dir, frame_color);
				cv::imwrite(screenshot_depth_dir, frame_depth);
                std::cout << "Screenshot saved." << std::endl;
                break;
                
            case 'q':
                recording = false;
                break;
        }
	}
    
    // Destroy objects, otherwise errors will occure next time...
	depth_stream.destroy();
	color_stream.destroy();
	device.close();
	OpenNI::shutdown();
    
    return 0;
}
