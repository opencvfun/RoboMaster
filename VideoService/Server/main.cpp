#include <iostream>
#include <cstring>
#include <libv4l2.h>
#include <linux/videodev2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "v4l2uvc.h"
#include "Structure.h"
using namespace std;
static pthread_t thread_fps;
const char devicefile[] = "/dev/video0";	//摄像头地址
const int imageWidth = 640;
const int imageHeight = 480;
const int imageFps=30;
unsigned int * image = 0;		//图片数据指针
unsigned int framelen = 0;	//图片文件大小
#define CLEAR(x) memset(&(x), 0, sizeof(x))
struct buffer {
    void   *start;
    size_t length;
};

static void xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1) {
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}
unsigned int fps_now = 0;	//当前发送的帧数
unsigned int fps_last = 0;	//一秒前发送的帧数
unsigned int fps_last_camera = 0;	//一秒前摄像头帧数
volatile unsigned int framenum = 0;		//实时帧数

void *fps1(void * pvoid){	//每秒统计一次帧数差
    sleep(1);
    while(1){
        int delta_fps = fps_now - fps_last;
        int delta_camera = framenum - fps_last_camera;
        printlog("\rfps: %d  \tcamera: %d  \tfilesize: %dkb  ", delta_fps, delta_camera, framelen/1024);
        printf("\rfps: %d  \tcamera: %d  \tfilesize: %dkb  ", delta_fps, delta_camera, framelen/1024);

        fflush(stdout);
        fps_last_camera = framenum;
        fps_last = fps_now;
        sleep(1);
    }
}

int main() {
    flog = fopen("./image_server.log", "w+");
    if(pthread_create(&thread_fps, NULL, fps1, NULL) < 0){
        printlog("create fps thread error/n");
        return 1;
    }
    /*
    //打开摄像头设备
    int fd = -1;
    //fd = open (videodevice, O_RDWR | O_NONBLOCK, 0);
    fd = open (videodevice, O_RDWR, 0);
    if(fd<0){
        printlog("failed to open\n");
        exit(EXIT_FAILURE);
    }
    printlog("打开摄像头设备成功，%s\n", videodevice);

    //查询摄像头信息，VIDIOC_QUERYCAP
    printlog("------------\n");
    struct v4l2_capability cap;
    xioctl (fd, VIDIOC_QUERYCAP, &cap);
    printlog("Driver Name:%s\nCard Name:%s\nBus info:%s\nDriver Version:%u.%u.%u\n\n", cap.driver, cap.card, cap.bus_info, (cap.version>>16)&0XFF, (cap.version>>8)&0XFF, cap.version&0XFF);

    //查询摄像头视频格式，VIDIOC_ENUM_FMT
    printlog("------------\n");
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printlog("Supportformat:\n");
    while(ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)!=-1)
    {
        printlog("\t%d.%c%c%c%c\t%s\n", fmtdesc.index+1, fmtdesc.pixelformat & 0xFF, (fmtdesc.pixelformat >> 8) & 0xFF, (fmtdesc.pixelformat >> 16) & 0xFF, (fmtdesc.pixelformat >> 24) & 0xFF, fmtdesc.description);
        fmtdesc.index++;
    }

    struct v4l2_format			fmt;
    struct v4l2_buffer			buf;
    struct v4l2_requestbuffers	req;
    struct v4l2_streamparm      fps;

    CLEAR(fmt);
    CLEAR(buf);
    CLEAR(req);
    CLEAR(fps);
    fps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fps.parm.capture.timeperframe.numerator=1;
    fps.parm.capture.timeperframe.denominator=imageFps;
    xioctl(fd, VIDIOC_S_PARM, &fps);
    xioctl(fd, VIDIOC_G_PARM, &fps);





    printlog("------------\n");
    printlog("设置摄像头视频数据格式\n");
    //设置摄像头视频数据格式，VIDIOC_S_FMT
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = imageWidth;
    fmt.fmt.pix.height = imageHeight;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    xioctl(fd, VIDIOC_S_FMT, &fmt);

    if ((fmt.fmt.pix.width != imageWidth) || (fmt.fmt.pix.height != imageHeight))
        printlog("Warning: driver is sending image at %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    else {
        printlog("fmt.type is %d\n", fmt.type);
        printlog("Width = %d\n", fmt.fmt.pix.width);
        printlog("Height= %d\n", fmt.fmt.pix.height);
        printlog("Sizeimage = %d\n", fmt.fmt.pix.sizeimage);
    }

    printlog("------------\n");
    printlog("请求分配视频缓冲区\n");
    //请求分配视频缓冲区，VIDIOC_REQBUFS
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    printlog("------------\n");
    printlog("查询缓冲信息，将内核空间映射到用户空间\n");
    //查询缓冲信息，将内核空间映射到用户空间，VIDIOC_QUERYBUF
    struct buffer *buffers;
    unsigned int i, n_buffers;

    buffers = (buffer*)calloc(req.count, sizeof(*buffers));
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;
        //把VIDIOC_REQBUFS中分配的数据缓存转换成物理地址
        xioctl(fd, VIDIOC_QUERYBUF, &buf);
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
            printlog("mmap\n");
            exit(EXIT_FAILURE);
        }
    }

    //投放一个空的视频缓冲区到视频缓冲区输入队列中，VIDIOC_QBUF
    printlog("------------\n");
    printlog("投放一个空的视频缓冲区到视频缓冲区输入队列中\n");
    for (i = 0; i < n_buffers; ++i) {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(fd, VIDIOC_QBUF, &buf);
    }

    //启动视频采集命令，VIDIOC_STREAMON
    printlog("------------\n");
    printlog("启动视频采集\n");
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &type);

    fd_set fds;
    int r;
    struct timeval tv;

    do {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno = EINTR)));
    if (r == -1) {
        printlog("select\n");
        return errno;
    }

*/

    struct shared_package * shared_package = get_shared_package();
    shared_package->count=0;
//    pthread_mutex_unlock(&shared_package->image_lock);

    //std::cout << "Hello, World!" << std::endl;
/*

    while(1)
    {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        //从视频缓冲区的输出队列中取得一个已经保存有一帧视频数据的视频缓冲区，VIDIOC_DQBUF
        xioctl(fd, VIDIOC_DQBUF, &buf);
        //pthread_mutex_lock(&frame_mutex);
        image = (unsigned int *)buffers[buf.index].start;
        framelen = buf.bytesused;
        //pthread_mutex_unlock(&frame_mutex);
        //printlog("------------\n");
        framenum ++;
        fps_now++;
        //printlog("取得一帧，framenum=%d\t%fkb\n", framenum, buf.bytesused/1024.0);
        //写入共享内存
        shared_package->image_size = framelen;
       // pthread_rwlock_wrlock(&shared_package->image_lock);

        //pthread_mutex_lock(&shared_package->image_lock);
        memcpy(shared_package->image_data, image, framelen);
        shared_package->count++;
      //  pthread_rwlock_unlock(&shared_package->image_lock);

        //pthread_mutex_unlock(&shared_package->image_lock);
        //printlog("\n");

        xioctl(fd, VIDIOC_QBUF, &buf);
    }





*/



    int width = 640 ;
    int height = 480 ;
    //int fps   = 30 ;
//    filename = "/dev/video0";
//    avifilename = "test.avi";

    int frame_width=width;
    int frame_height=height;
    int format = V4L2_PIX_FMT_MJPEG;
    int ret;
    int grabmethod = 1;

    FILE * file = fopen(devicefile, "wb");
    if(file == NULL)
    {
        printf("Unable to open file for raw frame capturing\n ");
        exit(1);
    }

    //v4l2 init
    struct vdIn *vd = (struct vdIn *) calloc(1, sizeof(struct vdIn));
    if(init_videoIn(vd, (char *) devicefile, width, height,imageFps,format,grabmethod,NULL) < 0)
    {
        exit(1);
    }

    if (video_enable(vd))
    {
        exit(1);
    }

    while(1){
        int ret;

        memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
        vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vd->buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(vd->fd, VIDIOC_DQBUF, &vd->buf);
        if (ret < 0)
        {
            printf("Unable to dequeue buffer");
            exit(1);
        }
        memcpy(shared_package->image_data, vd->mem[vd->buf.index],vd->buf.bytesused);
        shared_package->image_size=vd->buf.bytesused;
        shared_package->count++;fps_now++;
        ret = ioctl(vd->fd, VIDIOC_QBUF, &vd->buf);
        if (ret < 0)
        {
            printf("Unable to requeue buffer");
            exit(1);
        }
    }

















    return 0;
}