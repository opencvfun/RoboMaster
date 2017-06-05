#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "Structure.h"
using namespace cv;
using namespace std;
int main() {
    Mat frame;
    struct shared_package * shared_package = get_shared_package();
//    pthread_mutex_unlock(&shared_package->image_lock);

    while (1){

        if(getImageFromMemory(frame)!=0)continue;
        try{
            imshow("",frame);

        }catch (cv::Exception e){
            cout<<"error"<<endl;
        }
        waitKey(0xff);
    }
    std::cout << "Hello, World!" << std::endl;
    return 0;
}