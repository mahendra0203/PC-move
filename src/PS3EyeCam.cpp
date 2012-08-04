#include "PS3EyeCam.h"
#include <tchar.h>

/*Global variable to handle feature reports*/

unsigned char report_op[256] =  {0};
DWORD WINAPI hidFunction(LPVOID lpParam);
void create_threads();
bool Move_found = false;
/*My functions*/
void CLEyeCameraCapture::leftClick()
{
  INPUT Input={0};
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  ::SendInput(1,&Input,sizeof(INPUT));
  ::ZeroMemory(&Input,sizeof(INPUT));
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  ::SendInput(1,&Input,sizeof(INPUT));
  ::ZeroMemory(&Input,sizeof(INPUT));
}
void CLEyeCameraCapture::rightArrowDown()
{
	cout<<"rightArrowDown"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_RIGHT;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void CLEyeCameraCapture::rightArrowUp()
{
	cout<<"rightArrowUp"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_RIGHT;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void CLEyeCameraCapture::leftArrowDown()
{
	cout<<"leftArrowDown"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_LEFT;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void CLEyeCameraCapture::leftArrowUp()
{
	cout<<"leftArrowUp"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_LEFT;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void CLEyeCameraCapture::upArrowDown()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_UP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void CLEyeCameraCapture::upArrowUp()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_UP;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void CLEyeCameraCapture::downArrowDown()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_DOWN;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void CLEyeCameraCapture::downArrowUp()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_DOWN;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}


void CLEyeCameraCapture::setupWindows()
{
	is_ball_detected = Move_found = false;

	param1 = 0;//h min
	param2 = 0;//s min
	param3 = 160;//v min
	param4 = 256;//h max
	param5 = 256;//s max
	param6 = 256;//v max

	param7 = 0;//erode
	param8 = 0;//dilate

	gain = 5;
	exposure = 511;
	mode = 0;
	
	orgx=0;
	orgy=0;

	w=0; h=0;

}

bool CLEyeCameraCapture::StartCapture()
{
	//cvNamedWindow(_windowName, CV_WINDOW_AUTOSIZE);

	////////////////////////////////////////////////
	setupWindows();
	////////////////////////////////////////////////

	_running = true;

	// Start CLEye image capture thread
	_hThread = CreateThread(NULL, 0, &CLEyeCameraCapture::CaptureThread, this, 0, 0);
	if(_hThread == NULL)
	{
		MessageBox(NULL,"Could not create capture thread","CLEyeMulticamTest", MB_ICONEXCLAMATION);
		return false;
	}
	HANDLE hidThread = CreateThread(NULL, 0, hidFunction, this, 0, 0);
	if(hidThread == NULL)
	{
		MessageBox(NULL,"Could not create hid thread","CLEyeMulticamTest", MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

void CLEyeCameraCapture::StopCapture()
{
	if(!_running)	return;
	_running = false;
	WaitForSingleObject(_hThread, 500);
}

void CLEyeCameraCapture::setupParams()
{
	threshold_image = cvCreateImage(cvGetSize(pCapImage),IPL_DEPTH_8U,1);
	imgHSV = cvCreateImage(cvGetSize(pCapImage), IPL_DEPTH_8U, 3);
	image_roi = Mat(w,h,CV_8UC1,Scalar(0,0,0));
	circles.resize(1); //sino peta en debug
	radius=9999;
	oldTime=0; newTime=0;
}

void CLEyeCameraCapture::colorFiltering()
{
	cvCvtColor( pCapImage, imgHSV, CV_BGR2HSV);
	min_color = Scalar(param1,param2,param3,0);
	max_color = Scalar(param4,param5,param6,0);
	cvInRangeS(imgHSV, min_color, max_color, threshold_image);
}

void CLEyeCameraCapture::opening()
{
	cvErode(threshold_image,threshold_image,0,param7);
	cvDilate(threshold_image,threshold_image,0,param8);
}

void CLEyeCameraCapture::find_ball()
{
	is_ball_detected = false;

	using_image_roi = false;
	threshold_image_mat = threshold_image;

	if(radius==9999) Hough_mat= threshold_image_mat; 
	else
	{
		using_image_roi=true;
		Hough_mat= image_roi;
	}

	GaussianBlur(Hough_mat, Hough_mat, Size(7,7), 1.5, 1.5);
	HoughCircles(Hough_mat, circles, CV_HOUGH_GRADIENT, 2, Hough_mat.cols, 200,10); 
	int i;
	for( i = 0; i < circles.size(); i++ )
	{
		if(using_image_roi)
		{
			center.x = cvRound(circles[i].val[0]) + pos_rectangle_roi.x;
			center.y = cvRound(circles[i].val[1]) + pos_rectangle_roi.y;
		}
		else
		{
			center.x = (circles[i].val[0]);
			center.y = (circles[i].val[1]);
		}
		radius = cvRound(circles[i].val[2]);

		rowStep = threshold_image->widthStep;
		pixel = rowStep*center.y+center.x;
		real_radiusR = real_radiusL = real_radiusT = real_radiusB = 0;
		while(threshold_image->imageData[pixel+real_radiusR])
		{
			real_radiusR++;
		}
		while(threshold_image->imageData[pixel-real_radiusL])
		{				
			real_radiusL++;
		}
		while(threshold_image->imageData[pixel-rowStep*real_radiusT])
		{
			real_radiusT++;
			if((pixel-rowStep*real_radiusT)<=0) break;
		}
		while(threshold_image->imageData[pixel+rowStep*real_radiusB])
		{
			real_radiusB++;
			if((pixel+rowStep*real_radiusB)>=w*h) break;
		}
		temp = ((real_radiusR+real_radiusL)*(real_radiusR+real_radiusL) + (real_radiusB+real_radiusT)*(real_radiusB+real_radiusT));
		newradius = ((float)sqrt(temp))/3.0;
		diffradius = abs(newradius - radius);

		newCX = (real_radiusR+real_radiusL)*0.5;
		newCY = (real_radiusB+real_radiusT)*0.5;
		if(real_radiusL < newCX) newCX = center.x + (newCX - real_radiusL);
		else newCX = center.x - (real_radiusL - newCX);
		if(real_radiusT < newCY) newCY = center.y + (newCY - real_radiusT);
		else newCY = center.y - (real_radiusT - newCY);
		diffcenter = (int)(sqrt(abs(center.x-newCX)*abs(center.x-newCX)+abs(center.y-newCY)*abs(center.y-newCY)));
		//////printf("diffCENTER: %d\n",diffcenter);


		if(diffradius>10 || diffcenter>10 || mode == 2)
		{
			ball[0] = center.x;
			ball[1]= center.y;
			ball[2]= radius;
		}
		else if( mode != 2)
		{
			ball[0] = newCX;
			ball[1] = newCY;
			ball[2] = newradius;
		}

		balltoPaint[0] = (ball[0] + last_ball1[0] + last_ball2[0] + last_ball3[0])*0.25;
		balltoPaint[1] = (ball[1] + last_ball1[1] + last_ball2[1] + last_ball3[1])*0.25;
		balltoPaint[2] = (ball[2] + last_ball1[2] + last_ball2[2] + last_ball3[2])*0.25;
		is_ball_detected = true;

		offset = (w == 640)? 150:75;
		xx = cvRound(center.x-offset);
		if(xx<0) xx = 0;
		yy = cvRound(center.y-offset);
		if(yy<0) yy = 0;
		ww = cvRound(center.x+offset);
		if(ww>w) ww = w;
		hh = cvRound(center.y+offset);
		if(hh>h) hh = h;
		roi = Rect(Point(xx,yy),Point(ww,hh));
		image_roi = threshold_image_mat(roi);
		pos_rectangle_roi.x = xx;
		pos_rectangle_roi.y = yy;
		cvRectangle(pCapImage, Point(xx,yy),Point(ww,hh),Scalar(0,255,0),3);

		last_ball3 = last_ball2;
		last_ball2 = last_ball1;
		last_ball1 = ball;


		if(mode==0) 
		{
			cvRectangle(pCapImage, Point((ball[0]-ball[2]),(ball[1]-ball[2])),Point((ball[0]+ball[2]),(ball[1]+ball[2])),Scalar(0,255,0),2);
			cvLine(pCapImage,Point(pCapImage->width,ball[1]),Point(0,ball[1]),Scalar(0,255,0));
			cvLine(pCapImage,Point(ball[0],pCapImage->height),Point(ball[0],0),Scalar(0,255,0));
		}
		else if(mode==1)
		{
			cvRectangle(pCapImage, Point((balltoPaint[0]-balltoPaint[2]),(balltoPaint[1]-balltoPaint[2])),Point((balltoPaint[0]+balltoPaint[2]),(balltoPaint[1]+balltoPaint[2])),Scalar(0,255,0),2);
			cvLine(pCapImage,Point(pCapImage->width,balltoPaint[1]),Point(0,balltoPaint[1]),Scalar(0,255,0));
			cvLine(pCapImage,Point(balltoPaint[0],pCapImage->height),Point(balltoPaint[0],0),Scalar(0,255,0));
		}
		else if(mode==2)
		{
			cvRectangle(pCapImage, Point((center.x-radius),(center.y-radius)),Point((center.x+radius),(center.y+radius)),Scalar(0,0,255),2);
			cvLine(pCapImage,Point(pCapImage->width,center.y),Point(0,center.y),Scalar(0,0,255));
			cvLine(pCapImage,Point(center.x,pCapImage->height),Point(center.x,0),Scalar(0,0,255));
		}
		else if(mode==3)
		{
			cvRectangle(pCapImage, Point((newCX-newradius),(newCY-newradius)),Point((newCX+newradius),(newCY+newradius)),Scalar(255,0,0),2);
			cvLine(pCapImage,Point(pCapImage->width,newCY),Point(0,newCY),Scalar(255,0,0));
			cvLine(pCapImage,Point(newCX,pCapImage->height),Point(newCX,0),Scalar(255,0,0));
		}

			
			int screenx1 = GetSystemMetrics(SM_XVIRTUALSCREEN);  
			int screeny1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
			int screenx2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			int screeny2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);
			int movex = 640+(balltoPaint[0]*-1); 
			int movey = balltoPaint[1];
			
			int mousex = screenx1+((movex-50)*screenx2/540);  
			int mousey = screeny1+((movey-50)*screeny2/380);  
			if(mmode == 2){ 
				POINT p;
				GetCursorPos(&p);
				int mousex = p.x;
				int mousey = p.y;
				if(abs(movex-320)>10){
					if(movex-320>100){ movex = 420;} //Sensitivity
					if(movex-320<-100){ movex = 220;} //Sensitivity
					mousex = mousex + (movex-320);
				}
				if (abs(movey-240)>10){
					if(movex-240>100){ movex = 340;} //Sensitivity
					if(movex-240<-100){ movex = 140;} //Sensitivity
					mousey = mousey + (movey-240);
				}
				////printf("MoveY: %f\n",balltoPaint[1]);
				SetCursorPos(mousex,mousey);
			}else if(mmode == 1){
				POINT p;
				GetCursorPos(&p);
				if(orgx !=0){
					int relx = mousex-orgx;
					int rely = mousey-orgy;

					SetCursorPos(p.x+relx,p.y+rely);
				}
				orgx = mousex;
				orgy = mousey;
			}else if(mmode == 0){
				SetCursorPos(mousex,mousey);
			}
	}
	if(i==0)
	{
		
		radius = 9999; //no se han encontrado circulos
	}
	Move_found = is_ball_detected;
	
}

void CLEyeCameraCapture::destroyMyImages()
{
	cvReleaseImage(&threshold_image);
	cvReleaseImage(&imgHSV);
}

void CLEyeCameraCapture::showFPS()
{
	newTime = timeGetTime();
	float dt = newTime - oldTime;
	float fps = 1.0f / dt * 1000;
	oldTime = newTime;
	
}

void CLEyeCameraCapture::Run()
{
	pCapBuffer = NULL;

	// Create camera instance
	_cam = CLEyeCreateCamera(_cameraGUID, _mode, _resolution, _fps);
	if(_cam == NULL)		return;

	// Get camera frame dimensions
	CLEyeCameraGetFrameDimensions(_cam, w, h);

	// Depending on color mode chosen, create the appropriate OpenCV image
	if(_mode == CLEYE_COLOR_PROCESSED || _mode == CLEYE_COLOR_RAW)
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
	else
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);

	// Set some camera parameters
	CLEyeSetCameraParameter(_cam, CLEYE_GAIN, gain);
	CLEyeSetCameraParameter(_cam, CLEYE_EXPOSURE, exposure);

	// Start capturing
	CLEyeCameraStart(_cam);
	cvGetImageRawData(pCapImage, &pCapBuffer);

	//////////////////////////////////////
	setupParams();
	//////////////////////////////////////

	// image capturing loop
	while(_running)
	{
		CLEyeCameraGetFrame(_cam, pCapBuffer);

		////////////
		//showFPS();
		////////////

		//////////////////////////////////////
		//cvShowImage(_windowName, pCapImage);
		//cvWaitKey(10);	
		colorFiltering();
		opening();
		find_ball();
		//////////////////////////////////////
	}

	// Stop camera capture
	CLEyeCameraStop(_cam);

	// Destroy camera object
	CLEyeDestroyCamera(_cam);

	// Destroy the allocated OpenCV image
	cvReleaseImage(&pCapImage);

	//////////////////////////////////
	destroyMyImages();
	//////////////////////////////////

	_cam = NULL;
}
// Main program entry point
int main(HINSTANCE hInstance, HINSTANCE hPredInstance, LPSTR lpCmdLine, int nShowCmd)
{
	TCHAR cmdline[4096] ;
    TCHAR* argv[4096] ;
    int argc = 0 ;
	cout<<"Entered main";

	CLEyeCameraCapture *cam = NULL;
	
	mmode = 1;
	
	if(CLEyeGetCameraCount() == 0){//printf("No PS3Eye cameras detected\n");	
		return -1;}

	// Create camera capture object
	cam = new CLEyeCameraCapture("Camera Window", CLEyeGetCameraUUID(0), CLEYE_COLOR_RAW, CLEYE_VGA, 60);
	//cam = new CLEyeCameraCapture("Camera Window", CLEyeGetCameraUUID(0), CLEYE_COLOR_RAW, CLEYE_QVGA, 125);
	
	cam->StartCapture();
	
	while(1==1){}

	//printf("Stopping capture on camera\n");
	cam->StopCapture();
	delete cam;

	return 0;
}

DWORD WINAPI hidFunction(LPVOID lpParam)
{
	int res;
	unsigned char buf[256];
	unsigned char report_ip[] ={
								0x00 ,0x00 ,0x00 ,0x02 ,0x00, 0xA5, 0x3F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	hid_device *handle;
	int i;

/*#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif*/

	struct hid_device_info *devs, *cur_dev;
	
	devs = hid_enumerate(0x0, 0x0);
	cur_dev = devs;	
	while (cur_dev) {
		printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		printf("\n");
		printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		printf("  Product:      %ls\n", cur_dev->product_string);
		printf("  Release:      %hx\n", cur_dev->release_number);
		printf("  Interface:    %d\n",  cur_dev->interface_number);
		printf("\n");
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);

	// Set up the command buffer.
	memset(buf,0x00,sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;
	

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	////handle = hid_open(0x4d8, 0x3f, L"12345");
	handle = hid_open(0x8888, 0x0308, NULL);
	if (!handle) {
		printf("unable to open device\n");
 		return 1;
	}

	// Read the Manufacturer String
	wstr[0] = 0x0000;
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read manufacturer string\n");
	printf("Manufacturer String: %ls\n", wstr);

	// Read the Product String
	wstr[0] = 0x0000;
	res = hid_get_product_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read product string\n");
	printf("Product String: %ls\n", wstr);

	// Read the Serial Number String
	wstr[0] = 0x0000;
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read serial number string\n");
	printf("Serial Number String: (%d) %ls", wstr[0], wstr);
	printf("\n");

	// Read Indexed String 1
	wstr[0] = 0x0000;
	res = hid_get_indexed_string(handle, 1, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read indexed string 1\n");
	printf("Indexed String 1: %ls\n", wstr);

	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1);
	
	// Try to read from the device. There shoud be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 17);

	// Send a Feature Report to the device
	/*buf[0] = 0x2;
	buf[1] = 0xa0;
	buf[2] = 0x0a;
	buf[3] = 0x00;
	buf[4] = 0x00;*/
	create_threads();
	while(1){
		#ifdef WIN32
		Sleep(50);
		#else
		usleep(500*1000);
		#endif
		res = hid_send_feature_report(handle, report_ip, 52);
		if (res < 0) {
			printf("Unable to send a feature report.\n");
		}

		memset(report_op,0,sizeof(report_op));

		// Read a Feature Report from the device
		buf[0] = 0x00;
		res = hid_get_feature_report(handle, report_op, sizeof(report_op));
		if (res < 0) {
			printf("Unable to get a feature report.\n");
			printf("%ls", hid_error(handle));
		}
		else {
			////Print out the returned buffer.
			//printf("Feature Report\n   ");
			//for (i = 0; i < res; i++)
			//	printf("%02hhx ", report_op[i]);
			//printf("\n");
		}
	}
	memset(buf,0,sizeof(buf));

	// Toggle LED (cmd 0x80). The first byte is the report number (0x1).
	buf[0] = 0x0;
	buf[1] = 0x80;
	res = hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write()\n");
		printf("Error: %ls\n", hid_error(handle));
	}
	

	// Request state (cmd 0x81). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x81;
	hid_write(handle, buf, 17);
	if (res < 0)
		printf("Unable to write() (2)\n");

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	while (res == 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res == 0)
			printf("waiting...\n");
		if (res < 0)
			printf("Unable to read()\n");
		#ifdef WIN32
		Sleep(500);
		#else
		usleep(500*1000);
		#endif
	}

	printf("Data read:\n   ");
	// Print out the returned buffer.
	for (i = 0; i < res; i++)
		printf("%02hhx ", buf[i]);
	printf("\n");

	hid_close(handle);

	/* Free static HIDAPI objects. */
	hid_exit();

#ifdef WIN32
	system("pause");
#endif

	return 0;
}


void leftMouseDown();
void leftMouseUp();
void enterDown();
void enterUp();
void spacebarDown();
void spacebarUp();
void rightArrowDown();
void rightArrowUp();
void leftArrowDown();
void leftArrowUp();
void downArrowDown();
void downArrowUp();
void upArrowDown();
void upArrowUp();

//PS move buttons
bool MB_start=false;
bool MB_sel=false;
bool MB_triangle=false;
bool MB_square = false;
bool MB_circle = false;
bool MB_xbutton = false;
bool MB_ps=false;
bool MB_move=false;
bool MB_tbutton=false;

DWORD WINAPI triggerMonitor(LPVOID lpParam){
	//printf("\n TriggerMonitor Thread");
	int i;
	bool mouseDown=false;
	while(1){
		#ifdef WIN32
		Sleep(50);
		#else
		usleep(500*1000); 
		#endif
		if(report_op){
			if(mouseDown){
				if(report_op[06] == 0 && report_op[07] == 0){
					//leftMouseUp();
					mouseDown=false;
				}
			}else if(report_op[06] == 255 && report_op[07] == 255){
				//leftMouseDown();
				mouseDown=true;
			}
			/*printf("received report:");
			for(i=0;i<54;i++){
				printf("%02hx ", report_op[i]);
			}*/
			//printf("received report = %s",report_op[0]);
		}
	}
}

DWORD WINAPI startselMonitor(LPVOID lpParam){
	while(1){
		#ifdef WIN32
		Sleep(100);
		#else
		usleep(500*1000);
		#endif
		if(report_op){
			if(!MB_start && !MB_sel){
				if(report_op[2] == 1){
					printf("\n Select pressed");
				}else if(report_op[2] == 8){
					printf("\n Start selected");
					enterDown();
					MB_start=true;
				}
			}else{
				if(MB_start && (report_op[2] != 8)){
					enterUp();
					MB_start=false;
				}else if(MB_start && report_op[2] == 8){
					enterDown();
				}
			}
		}
	}
}

DWORD WINAPI xsctMonitor(LPVOID lpParam){
	while(1){
		#ifdef WIN32
		Sleep(100);
		#else
		usleep(500*1000);
		#endif
		if(report_op){
			if(!MB_triangle && !MB_square && !MB_circle && !MB_xbutton){
				if(report_op[3] == 16){
					printf("\n Triangle pressed");
					upArrowDown();
					MB_triangle=true;
				}else if(report_op[3] == 32){
					printf("\n Circle pressed");
					downArrowDown();
					MB_circle=true;
				}else if(report_op[3] == 128){
					printf("\n square pressed");
					rightArrowDown();
					MB_square=true;
				}else if(report_op[3] == 64){
					printf("\n X pressed");
					leftArrowDown();
					MB_xbutton=true;
				}
			}else{
				/*Check for triangle*/
				if(MB_triangle && report_op[3] != 16){
					upArrowUp();
					MB_triangle = false;
				}else if(MB_triangle && report_op[3] == 16){
					upArrowDown();
				}
				/*Check for circle*/
				if(MB_circle && report_op[3] != 32){
					downArrowUp();
					MB_circle=false;
				}else if(MB_circle && report_op[3] == 32){
					downArrowDown();
				}
				/*Check for square*/
				if(MB_square && report_op[3] != 128){
					rightArrowUp();
					MB_square = false;
				}else if(MB_square && report_op[3] ==128){
					rightArrowDown();
				}

				/*Check for xbutton*/
				if(MB_xbutton && report_op[3] != 64){
					leftArrowUp();
					MB_xbutton=false;
				}else if(MB_xbutton && report_op[3] == 64){
					leftArrowDown();
				}
			}
		}
	}
}
DWORD WINAPI pmtMonitor(LPVOID lpParam){
	while(1){
		#ifdef WIN32
		Sleep(100);
		#else
		usleep(500*1000);
		#endif
		if(report_op){
			if(!MB_ps && !MB_move && !MB_tbutton){
				if((report_op[4]==1) &&(report_op[5]/ 16 == 0)){
					//0x010 for PS button
					printf("\n PS button is pressed");
				}else if((report_op[4] == 8) && ((int)report_op[5]/16 == 4)){
					//0x084
					printf("\n Move button is pressed");
					//spacebarDown();
					leftMouseDown();
					MB_move=true;
				}else if((report_op[4] == 16) &&((int)report_op[5]/16 == 8)){
					//0x108
					printf("\n T button is pressed");
					MB_tbutton=true;
				}
			}else{
				if(MB_move && report_op[4] != 8 && (int)report_op[5]/16 != 8){
					//spacebarUp();
					leftMouseUp();
					MB_move=false;
				}else if(MB_move  && report_op[4] == 8 && (int)report_op[5]/16 == 4){
					//spacebarDown();
					leftMouseDown();
				}else if(MB_tbutton && report_op[4] != 16 && (int)report_op[5]/16 != 8){
					MB_tbutton=false;
				}
			}
		}
	}
}

DWORD WINAPI accelMonitor(LPVOID lpParam){
	int oldval=0;
	int newval=0;
	while(1){
		#ifdef WIN32
		Sleep(500);
		#else	
		usleep(500*1000);
		#endif
		newval = ((report_op[14] +  report_op[20])+((report_op[15] +  report_op[21])<<8))/2 - 0x8000;
		printf("\n xval:%d",newval);
		if((abs(oldval - newval) > 800) && ((report_op[4] == 8) && ((int)report_op[5]/16 == 4))){
			if((oldval - newval) < 0)
				printf("\n left click ");
			else if((oldval -newval) > 0)
				printf("\n right click ");
		}
		oldval = newval;
			
	}
}

DWORD WINAPI imageMonitor(LPVOID lpParam){
	bool old_t=false;
	bool m_found=false;
	char filename[256]={0};
	while(1){
		Sleep(50);
		printf("\n Move_found:%d, MB_tbutton:%d",Move_found,MB_tbutton);
		if(!Move_found && MB_tbutton){
			printf("\n move not found.but T pressed");
			FILE *cOp= _popen("\"C:\\Python25\\python.exe \"C:\\Users\\P.B naik\\Desktop\\zedashaw\\moveUtility.py\"\"","r");
			if(NULL == cOp){
				printf("_popen for image create failed");
				exit(0);
			}
			old_t = true;
			while(!feof(cOp)){
				if(fgets(filename,256,cOp) == NULL) printf("\n Null image on console");
				if(NULL != strstr(filename,".jpg")){
				std::cout<<filename;
				break;
				}
			}

			while(!Move_found){
				if(!MB_tbutton)
					break;
			}
			if(Move_found){
				//Show image
				char show_paint_command[256] = {0};
				strcpy(show_paint_command,"mspaint ");
				strcat(show_paint_command,filename);
				FILE *shiOp = _popen(show_paint_command,"r");
				if(NULL == shiOp){
					printf("\n _popen for show image failed");
					exit(0);
				}else{
					//clear the filename buffer
					memset(filename,0,256);
				}			
			}
		}
	}
}
void create_threads(void)
{
	/*Thread to monitor the mouse click*/
	HANDLE trigger_thread = CreateThread(NULL,0,triggerMonitor,0,0,0);
	if(NULL == trigger_thread){
		printf("\n Thread creation failed");
		exit(0);
	}
	/*Thread for start or select*/
	HANDLE startsel_thread = CreateThread(NULL,0,startselMonitor,0,0,0);
	if(NULL == startsel_thread){
		printf("\n Thread creation failed");
		exit(0);
	}
	/*Thread for X,Circle,Square,Triangle*/
	HANDLE xsct_thread = CreateThread(NULL,0,xsctMonitor,0,0,0);
	if(NULL == xsct_thread){
		printf("\n Thread creation failed");
		exit(0);
	}

	/*Thread for PS,Mode,T*/
	HANDLE pmt_thread = CreateThread(NULL,0,pmtMonitor,0,0,0);
	if(NULL == pmt_thread){
		printf("\n Thread creation failed");
		exit(0);
	}

	//Image thread
	HANDLE im_thread = CreateThread(NULL,0,imageMonitor,0,0,0);
	if(NULL == im_thread){
		printf("\n Image monitor thread failed");
		exit(0);
	}

}


void leftMouseDown(){
	printf("\n mouse clicked");
	INPUT Input={0};
	Input.type      = INPUT_MOUSE;
	Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}
void leftMouseUp(){
	printf("\n mouse released");
	INPUT Input={0};
	Input.type      = INPUT_MOUSE;
	Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

/*Map the keyboard keys*/
/*Setting for KmPlayer*/
//Space Bar
void spacebarDown(){
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_SPACE;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void spacebarUp(){
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_SPACE;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

//Arrow keys
void rightArrowDown()
{
	//cout<<"rightArrowDown"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_RIGHT;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void rightArrowUp()
{
	//cout<<"rightArrowUp"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_RIGHT;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void leftArrowDown()
{
	//cout<<"leftArrowDown"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_LEFT;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void leftArrowUp()
{
	//cout<<"leftArrowUp"<<endl;
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_LEFT;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void upArrowDown()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_UP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void upArrowUp()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_UP;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void downArrowDown()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_DOWN;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void downArrowUp()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_DOWN;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void enterDown()
{
	printf("\n enter pressed");
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_RETURN;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}

void enterUp()
{
	INPUT Input ={0};
	Input.type=INPUT_KEYBOARD;
	Input.ki.wVk=VK_RETURN;
	Input.ki.dwFlags=KEYEVENTF_KEYUP;
	::SendInput(1,&Input,sizeof(INPUT));
	::ZeroMemory(&Input,sizeof(INPUT));
}