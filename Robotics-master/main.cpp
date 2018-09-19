#include <iostream>
#include "include/vision.h"

using namespace std;

//These are so far the best parameters for find circles as well as tolerances for certain
//algorithim decisions for radius filtering and rectangle circle pattern detection.
//Don't change these unless you ask Jason first.

static double dp = 1;
static double minDist = 80;
static double param1 = 40;
static double param2 = 30;
static double minRadius = 5;
static double maxRadius = 100;
static int radTolerance = 10;
static int distTolerance = 40;
static int intTolerance = 50;

int main(){

	Vision *v = new Vision(dp, minDist, param1, param2, minRadius, maxRadius, radTolerance, distTolerance, intTolerance);

	while(true){
		if(v->checkBoard()){
			cout << "Found board!" << endl;
			v->analyzeBoard();
		}
	}

	delete v;

	return 0;
}
