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
            void calculateInvertedCoordinates(double x, double y);
            double getXinv();
            double getYinv();
            void setCalibrationPoint(int,int,float);
            double getCalibrationPoint(int, int);
            void orderCalibrationArray(bool calculateHomography = true);
            void setBounds(double xMin = 0, double xMax = 4096, double yMin = 0, double yMax = 4096);
            void setBounds2(double xMax1, double yMax1, double xMax2, double yMin1, double xMin1, double yMin2, double xMin2, double yMax2);

        private:
            uint8_t findMax(double ARRAY[]);
            mtx_type _H[3][3];
            double _x;
            double _y;
            double _xInv;
            double _yInv;
            double _bounds[2][4];
            double _calArray[2][4];
    };
#endif
