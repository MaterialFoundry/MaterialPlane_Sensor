/**
 *  Library to perfom homography transforms
 *  Author: C Deenen
 */


#ifndef HOMOGRAPHY_H
#define HOMOGRAPHY_H

#include "Arduino.h"
#include "../MatrixMath/MatrixMath.h"
#include "math.h"

    class homography
    {
        public:
            homography();
            void calculateHomographyMatrix();
            void calculateCoordinates(double, double);
            double getX();
            double getY();
            void setCalibrationPoint(int,int,float);
            double getCalibrationPoint(int, int);
            void orderCalibrationArray(bool calculateHomography = true);

        private:
            uint8_t findMax(double ARRAY[]);
            mtx_type _H[3][3];
            double _x;
            double _y;
            double _bounds[2][4];
            double _calArray[2][4];
    };
#endif
