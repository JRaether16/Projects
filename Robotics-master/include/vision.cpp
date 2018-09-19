#include "vision.h"

Vision::Vision(){

	//Default values for vision object. These have been optimized. If we find optimized values, put them here.

	dp_ = 1; //Refer to OpenCV docs.
	minDist_ = 80; //Minimum distance between consecutive circles.
	param1_ = 40; //Refer to OpenCV docs.
	param2_ = 30; //Refer to OpenCV docs.
	minRadius_ = 5; //Minimum radius for circle detection.
	maxRadius_ = 100; //Maximum radius for circle detection.
	radTolerance_ = 10; //Radius tolerance for radius filtering for circles.
	distTolerance_ = 40; //Distance tolerance when determining rectangle of circles.
	intTolerance_ = 50;
	capture = NULL;

	if(!this->openCapture()){ //Opens capture
		cout << "Problem with vision construction. Error with capture." << endl;
	}
	if(!this->readCapture()){ //Reads capture into video mat variable
		cout << "Problem with vision construction. Error with reading capture into video." << endl;
	}	

}

Vision::Vision(double dp, double minDist, double param1, double param2, double minRadius, double maxRadius, int radTolerance, int distTolerance, int intTolerance){

	//Values for vision object. This constructor is mainly used for testing optimal parameters. Put optimized ones in default constructor.
	//Refer to default constructor for attribute descriptions.

	dp_ = dp;
	minDist_ = minDist;
	param1_ = param1;
	param2_ = param2;
	minRadius_ = minRadius;
	maxRadius_ = maxRadius;
	radTolerance_ = radTolerance;
	distTolerance_ = distTolerance;
	intTolerance_ = intTolerance;
	capture = NULL;

	if(!this->openCapture()){ //Opens capture
		cout << "Problem with vision construction. Error with capture." << endl;
	}
	if(!this->readCapture()){ //Reads capture into video mat variable
		cout << "Problem with vision construction. Error with reading capture into video." << endl;
	}
}


Vision::~Vision(){

	//Frees capture. Nothing much to do here.

	delete(capture);

	cout << "Freed capture." << endl;

}

int Vision::openCapture(){

	capture = new VideoCapture(0); //Dynamically allocate video capture for device 0. That 0 is where the picamera resides, don't change.

	if(!capture->isOpened()){ //Check if capture opened successfully.
		cout << "Opening capture failed." << endl;
		return 0; //Unsuccessful.
	}

	cout << "Capture successfully opened." << endl;
	return 1; //Successful.

}

int Vision::readCapture(){

	//Reads capture into video mat variable. If capture is not there, read image from file.

	if(!capture->read(frame)){ //Check if capture is read into video successfully.
		cout << "Capture could not be read to video." << endl;
		return 0; //Unsuccessful
	}

	cout << "Capture read to video." << endl;
	return 1; //Successful

}

int Vision::readImage(string fileName){

	//Reads image from file. This is for test cases.
	
	frame = imread(fileName, CV_LOAD_IMAGE_COLOR); //Reads from file based on fileName.

	if(!frame.data){ //Check if capture is read into video successfully.
		cout << "Could not read image from file." << endl;
		return 0; //Unsuccessful
	}

	cout << "Read image from file." << endl;
	return 1; //Successful

}

void Vision::streamVideo(){

	//TODO: Install software and test this. The giant block comment is literally copied from stack overflow. I have done nothing with it yet.

	/* Open pipeline using gstreamer
	 *
	 * sender cmd:
	 *
	 * gst-launch-1.0 -v v4l2src \
	 * ! video/x-raw,format=YUY2,width=640,height=480 \
	 * ! jpegenc \
	 * ! rtpjpegpay \
	 * ! udpsink host=127.0.0.1 port=5000
	 */

	/*
	 * receiver cmd:
	 *
	 * gst-launch-1.0 -v udpsrc port=5000 \
	 * ! application/x-trp, media=video, clock-rate=90000, encoding-name=JPEG, payload=26 \
	 * ! rtpjpegdepay \
	 * ! jpegdec \
	 * ! xvimagesink sync=0
	 */

	/*

	VideoCapture cap("v4l2src ! video/x-raw, format=BGR, width=640, height=480, framerate=30/1 ! appsink" , CAP_GSTREAMER);

	VideoWriter out("appsrc ! videoconvert ! video/x-raw, format=YUY2, width=640, height=480, framerate=30/1 ! jpegenc ! rtpjpegpasy ! udpsink host=127.0.0.1 port=5000", CAP_GSTREAMER, 0, 30 Size(640,480), true);

	if(!cap.isOpened() || !out.isOpened()){
		cout << "VideoCapture of VideoWriter not opened." << endl;
		exit(-1);
	}

	Mat frame;

	while(true){

		cap.read(frame);

		if(frame.empty()){
			break;
		}

		out.write(frame);
	}

	*/

}

int Vision::checkBoard(string fileName){

	//Function for checkBoard on a still image, not a live feed.
	
	
	if(!readImage(fileName)){ //If there's a problem reading the image, leave function with "-1" error.
		return -1;
	}

	size_t size = findCircles(); //Grab circle vector size.

	//If less than 4 circles, then it cannot be the board. Any more circles greater than 15 makes the Pi3 run very slow during drawing.
	if(size >= 4 && size <= 20){ //The amount of circles must be a reasonable amount for radius filtering, rectangle detection, and drawing.

		filterRadius(); //Filters out unnecessary circles with radii too far off from the mode.			

		if(findRectangle()){
			return 1;
		}

		drawCircles(); //Draws the circles on the regular frame mat variable.			
	}

	imshow("GrayFrame", grayFrame);	//Show the gray image on screen.
	imshow("Frame", frame); //Show the regular image on screen.
	waitKey(100); //Wait 100 ms. Required but the time can vary. Try to keep it above 30 ms.

	return 0;

}

size_t Vision::findCircles(){

	cvtColor(frame, grayFrame, CV_BGR2GRAY);

	GaussianBlur(grayFrame, grayFrame, Size(9,9), 2, 2);
	
	HoughCircles(grayFrame, circles, CV_HOUGH_GRADIENT, dp_, minDist_, param1_, param2_, minRadius_, maxRadius_);

	return circles.size();

}

int Vision::checkBoard(){

	//IMPORTANT NOTE: THIS WHOLE FUNCTION RUNS IN A "while(true)" LOOP IN MAIN.CPP
	//
	//This function checks if we are actually at the Simon Say's board. For that to be true, there must be a few conditions:
	//1. There must be four circles that form a pseudo rectangle within a certain offset tolerance, distTolerance_.
	//2. The radius of all of the circles must be within a certain radiance tolerance, radTolerance_.

	
	if(!readCapture()){ //If there's a problem reading the capture into video, leave this function with "-1" error.
		return -1; //Big error.
	}

	size_t size = findCircles(); //Grab circle vector size.

	//If less than 4 circles, then it cannot be the board. Any more circles greater than 15 makes the Pi3 run very slow during drawing.
	if(size >= 4 && size <= 20){ //The amount of circles must be a reasonable amount for radius filtering, rectangle detection, and drawing.

		filterRadius(); //Filters out unnecessary circles with radii too far off from the mode.			

		if(findRectangle()){ //If we found the pseudo rectangle.
			return 1; //Found the board!
		}

		drawCircles(); //Draws the circles on the regular video mat variable. Used for debugging or if operator wants to see.			
	}

	imshow("GrayFrame", grayFrame);	//Show the gray video on screen.
	imshow("Frame", frame); //Show the regular video on screen.
	waitKey(100); //Wait 100 ms. Required but the time can vary. Try to keep it above 30 ms.

	return 0; //Didn't find the board.

}

int Vision::findRectangle(){

	//This function determines if there are four common circles arranged in a rectangle formation.

	if(circles.size() < 4){ //If we don't have enough circles to work with.
		return 0; //Unsuccessful
	}

	std::vector<std::pair<Vec3f, Vec3f>> xMatch;
	std::vector<std::pair<Vec3f, Vec3f>> yMatch;

	std::pair<Vec3f, Vec3f> xPair;
	std::pair<Vec3f, Vec3f> yPair;

	//This part looks for x-coordinate and y-coordinate pairs.
	for(auto outerIt : circles){
		for(auto innerIt : circles){
			if(innerIt != outerIt){
				if(abs(outerIt[0] - innerIt[0]) <= distTolerance_ && abs(outerIt[2] - innerIt[2] <= radTolerance_)){
					xPair = std::make_pair(outerIt, innerIt);
					xMatch.push_back(xPair);	
				}
				if(abs(outerIt[1] - innerIt[1]) <= distTolerance_ && abs(outerIt[2] - innerIt[2] <= radTolerance_)){
					yPair = std::make_pair(outerIt, innerIt);
					yMatch.push_back(yPair);
				}
			}
		}
	}

	std::vector<std::pair<Vec3f, Vec3f>>::iterator xIt;	
	std::vector<std::pair<Vec3f, Vec3f>>::iterator yIt;

	for(xIt = xMatch.begin(); xIt != xMatch.end(); xIt++){
		for(yIt = yMatch.begin(); yIt != yMatch.end(); yIt++){
			
			if(findPair(*xIt, *yIt)){
				circles.clear();
				circles.push_back(xIt->first);
				circles.push_back(xIt->second);
				circles.push_back(yIt->first);
				circles.push_back(yIt->second);
				return 1; //Found rectangle!
			}
		}
	}

	return 0; //No rectangle

}

int Vision::findPair(std::pair<Vec3f, Vec3f> xPair, std::pair<Vec3f, Vec3f> yPair){

	if((abs(xPair.first[0] - yPair.first[0]) <= distTolerance_ && abs(xPair.second[1] - yPair.second[1]) <= distTolerance_) || (abs(xPair.first[0] - yPair.second[0]) <= distTolerance_ && abs(xPair.second[1] - yPair.first[1]) <= distTolerance_)){
		return 1; //Found pair
	}

	return 0;
}

void Vision::drawCircles(){

	//This function draws circles on the frame mat variable.

	size_t size = circles.size(); //Get size of circles vector.

	int radius = 0; //Radius of circle.

	for(size_t i = 0; i < size; i++){ //Loop through all circles.

		Point center(cvRound(circles[i][0]), cvRound(circles[i][1])); //Get center point

		radius = cvRound(circles[i][2]); //Get radius.

		circle(frame, center, 3, Scalar(0,255,0), -1, 8, 0); //Draw center dot in green. Go to OpenCV Docs for params.

		circle(frame, center, radius, Scalar(0,0,255), 3, 8, 0); //Draw circumference in red. Go to OpenCV Docs for params.
	}
}

void Vision::filterRadius(){

	//If looking at runtime for this, which please don't, the size of "n" will never exceed "15" currently.

	//This function will filter out circles from circles vector that don't have a radius
	//within the radTolerance of the mode radius. If this confuses you, ask Jason.

	std::map<int, int> buckets; //Buckets to hold the count of a certain radius appearing in circles vector

	//NOTE: init[2] contains the radius for a particular circle in the circles vector.
	//This part initializes the buckets.
	for(auto init : circles){ //Go through the circles and fill categories for buckets.
		if(buckets.find(init[2]) == buckets.end()){ //If there is currently no category for a particular radius.
			buckets.insert(std::pair<int, int>(init[2], 0)); //Insert a bucket for that radius, start it at 0.
		}
	}	

	//This part increments the buckets.
	for(auto outerIt : circles){ //Pick a particular circle.
		for(auto innerIt : circles){ //Go through all the circles.
			if(outerIt != innerIt){ //Make sure we aren't comparing the same circle.
				if(abs(outerIt[2] - innerIt[2]) <= radTolerance_){ //If the difference of two circle radii is <= the tolerance.
					buckets.at(outerIt[2])++; //Increment the bucket size.
				}
			}
		}
	}

	std::vector<int> highestRads; //Holds the mode of the circle radii. There can be more than one.

	//This part just pushes on the first radius to start out for comparisons.
	std::map<int, int>::iterator it = buckets.begin(); //Get the first bucket.
	highestRads.push_back((*it).first); //Push back the first radius.
	int highestRadFreq = (*it).first; //Set the running highest mode frequency.

	//This part finds the mode. There can be multiple modes.
	for(auto mapIt : buckets){ //Go through all buckets
		if(mapIt.second > highestRadFreq){ //If one of the buckets beats the running highest frequency.
			highestRads.clear(); //Clear the highestRads vector.
			highestRads.push_back(mapIt.first); //Push back the new highest radius.
			highestRadFreq = mapIt.second; //Updated the running highest frequency.
		}
		else if(mapIt.second == highestRadFreq){ //If one of the buckets is the same as the running highest frequency.
			highestRads.push_back(mapIt.first); //Add it to the list of modes.
		}
	}

	std::vector<Vec3f> newCircles; //This is a temporary container to hold our filtered circles vector.

	//This part does the actual filtering and puts the appropriate circles in the newCircles container.
	for(auto radsIt : highestRads){ //Go through all the highest modes.
		std::vector<Vec3f>::iterator circlesIt; //I didn't use autos because it got complicated.
		for(circlesIt = circles.begin(); circlesIt != circles.end(); circlesIt++){ //Go through our original circles vector.
			if(abs(radsIt - (*circlesIt)[2]) <= radTolerance_){ //If it falls within the radTolerance of this particular mode.
				newCircles.push_back(*circlesIt); //It's an appropriate circle, keep it.
			}
		}
	}

	circles.clear(); //Clear our old circles vector just in case.

	circles = newCircles; //Update circles with our filtered circles we decided to keep.
}

void Vision::analyzeBoard(string fileName){

	Mat gameBoard;
	if(!readImage(fileName)){ //We might not want to recapture this, might want to stay on the original one.
		return;
	}

	cvtColor(frame, gameBoard, CV_BGR2GRAY);

	for(size_t i = 0; i < circles.size(); i++){
		
		Rect r(circles[i][0], circles[i][1], circles[i][2] * 2, circles[i][2] * 2);

		if(r.x >= 0 && r.y >= 0 && r.width + r.x < frame.cols && r.height + r.y < frame.rows){

			Mat roi(frame, r);
			Scalar avgIntensity = mean(roi);
			cout << avgIntensity.val[0] << endl;

		}

	}



}

void Vision::analyzeBoard(){

	//Must run for live video feed. Unsure if its useful for testing on images.
	//TODO: Implement same for still image.

	int size = circles.size();

	int *intMap = new int[size];

	for(int i = 0; i < size; i++){

		intMap[i] = -1;
	}

	while(true){

		Mat gameBoard;
		if(!readCapture()){ //We might not want to recapture this, might want to stay on the original one.
			return;
		}

		cvtColor(frame, gameBoard, CV_BGR2GRAY);

		for(int i = 0; i < size; i++){
		
			Rect r(circles[i][0], circles[i][1], circles[i][2] * 2, circles[i][2] * 2);

			if(r.x >= 0 && r.y >= 0 && r.width + r.x < frame.cols && r.height + r.y < frame.rows){	
			
				Mat roi(frame, r);
				Scalar avgIntensity = mean(roi);
				cout << avgIntensity.val[0] << endl;

				if(intMap[i] == -1){
					intMap[i] = avgIntensity.val[0];
				}
				else{
					int prevIntensity = intMap[i];
					if(abs(prevIntensity - avgIntensity.val[0]) <= intTolerance_){
						intMap[i] = avgIntensity.val[0];
					}
					else{
						cout << "Button lit up at: " << circles[i] << endl;
						//Do something
					}
				}
			}
		}

	}

}

void Vision::panArena(){


}


void Vision::followMovement(){


}



