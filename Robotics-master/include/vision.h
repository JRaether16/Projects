#ifndef VISION_H
#define VISION_H

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <math.h>
#include <queue>
#include <map>
#include <string>
#include <unordered_map>

using std::string;
using std::cout;
using std::endl;
using namespace cv;

class Vision{

	public:
		
		/*
		 * Default Constructor, initializes private variables. Default values are visible in .cpp files, they aren't very good.
		 */
		Vision();

		Vision(double dp, double minDist, double param1, double param2, double minRadius, double maxRadius, int radTolerance, int distTolerance, int intTolerance);

		/*
		 * Destructor, deletes private variables that need to be deleted
		 */
		~Vision();


	
		/*
		 * Streams video to window on display,
		 * later instead of streaming on the pi,
		 * this will be sent to our receiving display,
		 * whether it be VR or a laptop
		 *
		 * Needs to spawn another process so this can keep happening
		 * while we do other stuff to the video. This should
		 * also be sent over with gstreamer to our receiving display
		 */
		void streamVideo();


		/*
		 * Checks on screen if we are at the simon says board
		 *
		 * Returns 1 if we are at the board, 0 otherwise
		 */
		int checkBoard();


		/*
		 * Analyzes the video during the simon says game
		 * Push 1 if top left, 2 if top right,
		 * 3 if bottom left, 4 if bottom right.
		 * Do nothing or push 0 if it cannot determine which one it is
		 */

		void analyzeBoard();

		void analyzeBoard(string fileName);

		void drawCircles();

		int readImage(string fileName);


		/*
		 * Rotates around in a circular direction
		 */
		void panArena();

		int checkBoard(string fileName);

		/*
		 * Follows moving objects with camera
		 */
		void followMovement();


	private:
	
		void filterRadius();

		int findRectangle();

		int findPair(std::pair<Vec3f, Vec3f> xPair, std::pair<Vec3f, Vec3f> yPair);

		size_t findCircles();

		std::queue<int> presses; //Queue to hold our presses

		std::vector<Vec3f> circles;

		Mat frame; //Holds the frame Mat

		Mat grayFrame;

		VideoCapture *capture; //Holds the capture

	/*
		 * Opens video stream for object, running in Mat video
		 * 
		 * Returns 1 if successful, 0 otherwise
		 */
		int openCapture();


		/*
		 * Reads capture into Mat video
		 */
		int readCapture();


		double dp_;
		double minDist_;
		double param1_;
		double param2_;
		double minRadius_;
		double maxRadius_;
		
		int radTolerance_;
		int distTolerance_;
		int intTolerance_;
};

#endif
