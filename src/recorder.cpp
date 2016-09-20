// System
#include <vector>
#include <string>
#include <sys/time.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>

// OpenCV
#include <opencv2/opencv.hpp>

// OpenNI
#include "OpenNI.h"

// Boost
#include <boost/filesystem.hpp>

// Recorder
#include "ini.hpp"


// image size
cv::Size size;
// main loop
bool recording(true);
// save screen shot
bool writescreenshot(false);
// save avi
bool writeavi(false);
// save png
bool writepng(true);
// holds current data
cv::Mat frame_color;
cv::Mat frame_depth;





using namespace openni;




void onKeyStroke()
{
    // Print help
    std::cout << "s - take and save screenshot." << std::endl;
    std::cout << "v - save video. Warning: This hurts fps." << std::endl;
    std::cout << "p - save single frames as png files." << std::endl;
    std::cout << "q - quit." << std::endl;

    while(recording)
    {
        //show window
        cv::imshow("Color", frame_color);
        cv::imshow("Depth", frame_depth);
        
        
        // React on keyboard input
        char key = cv::waitKey(10);
        switch (key)
        {
             case 'v':
                 writeavi = !writeavi;
                 if(writeavi)
                     std::cout << "Video output turned on." << std::endl;
                 else
                     std::cout << "Video output turned off." << std::endl;
                 break;

             case 'p':
                 writepng = !writepng;
                 if(writepng)
                     std::cout << "png output turned on." << std::endl;
                 else
                     std::cout << "png output turned off." << std::endl;
                 break;

             case 's':
                 writescreenshot = true;
                 break;
                
             case 'q':
                 recording = false;
                 break;
        }
    }
}





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
    std::string directory = config["directory"];

    writeavi = (bool)std::stoi(config["writeavi"]);
    writepng = (bool)std::stoi(config["writepng"]);

    // create opencv size
    size = cv::Size(width, height);

    // init matrix with zero
    frame_color = cv::Mat::zeros(height, width, CV_64F);
    frame_depth = cv::Mat::zeros(height, width, CV_64F);

    

    // create directory to store data
    // struct timeval tp;
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    std::stringstream ss;
    ss << (now->tm_year + 1900) 
        << '-' 
        << std::setfill('0') << std::setw(2)
        << (now->tm_mon + 1) 
        << '-'
        << std::setfill('0') << std::setw(2)
        <<  now->tm_mday
        << '_'
        << std::setfill('0') << std::setw(2)
        << now->tm_hour
        << '-'
        << std::setfill('0') << std::setw(2)
        <<  now->tm_min
        << '-'
        << std::setfill('0') << std::setw(2)
        <<  now->tm_sec;
    std::string subdirectory = directory + "/" + ss.str();
    boost::filesystem::path dir(subdirectory);
    if (!boost::filesystem::create_directory(subdirectory))
    {
        std::cerr << "Could not create folder " << subdirectory.c_str() << std::endl;
        return -1;
    }

    // open png
    std::vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(0);        // do not use compressioon

    // open avi writer
    cv::VideoWriter *video_color_writer;
    cv::VideoWriter *video_depth_writer;
    video_color_writer = new cv::VideoWriter(subdirectory + "/" + "video_color.avi", CV_FOURCC('j','p','e','g'), 25, size, true);
    video_depth_writer = new cv::VideoWriter(subdirectory + "/" + "video_depth.avi", CV_FOURCC('j','p','e','g'), 25, size, true);
    
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
            std::cerr << "Can't apply VideoMode: " << OpenNI::getExtendedError() << std::endl;
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
                std::cerr << "Can't apply VideoMode: " << OpenNI::getExtendedError() << std::endl;
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
    
    // start a thread, which handles key strokes
    std::thread th(onKeyStroke);

    

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
        cv::cvtColor(color_frame_temp, frame_color, CV_RGB2BGR);

        const cv::Mat depth_frame_temp( size, CV_16UC1, (void*)depth_frame.getData() );
        cv::Mat frame_depth_color;
        depth_frame_temp.convertTo(frame_depth, CV_8U, 255. / max_depth);
        cv::cvtColor(frame_depth, frame_depth_color, CV_GRAY2BGR);

        // create time stamp
        time_t t = time(0);   // get time now
        struct tm * now = localtime( & t );
        struct timeval tp;
        gettimeofday(&tp, NULL);
        long int ms =tp.tv_usec;
        std::stringstream ss;
        ss << (now->tm_year + 1900) 
            << std::setfill('0') << std::setw(2)
            << (now->tm_mon + 1) 
            << std::setfill('0') << std::setw(2)
            <<  now->tm_mday
            << 'T'
            << std::setfill('0') << std::setw(2)
            << now->tm_hour
            << std::setfill('0') << std::setw(2)
            <<  now->tm_min
            << std::setfill('0') << std::setw(2)
            <<  now->tm_sec
            << '.'
            << std::setfill('0') << std::setw(6)
            << ms;
        std::string timestamp = ss.str();

        // store frame png
        if(writepng)
        {
            try
            {
                imwrite(subdirectory + "/frame_" + timestamp + "_rgb.png", frame_color, compression_params);
                imwrite(subdirectory + "/frame_" + timestamp + "_depth.png", frame_depth, compression_params);
            }
            catch (std::runtime_error& ex)
            {
                std::cerr << "Exception converting image to PNG format: " <<  ex.what() << std::endl;
                return -1;
            }
        }

        // store avi        
        if(writeavi)
        {
            try
            {
                video_color_writer->write(frame_color);
                video_depth_writer->write(frame_depth_color);
            }
            catch (std::runtime_error& ex)
            {
                std::cerr << "Exception storing video file: " <<  ex.what() << std::endl;
                return -1;
            }
        }

        // store screenshot
        if (writescreenshot)
        {
             cv::imwrite(subdirectory + "/screenshot_" + timestamp + "_rgb.png", frame_color);
             cv::imwrite(subdirectory + "/screenshot_" + timestamp + "_depth.png", frame_depth);
             writescreenshot = false;
             std::cout << "Screenshot saved." << std::endl;
        }
    }


    // kill thread
    th.join();
    
    // Destroy objects, otherwise errors will occure next time...
    depth_stream.destroy();
    color_stream.destroy();
    device.close();
    OpenNI::shutdown();
    
    return 0;
}
