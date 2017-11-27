#ifndef BEZIERARC_H
#define BEZIERARC_H

#include <QPoint>

class BezierArc
{
public:
    BezierArc();

    static double BezierArcLength(QPoint p1, QPoint p2, QPoint p3, QPoint p4);
private:
    static double Simpson (
            double (*f)(double),
            double a,
            double b,
            int n_limit,
            double TOLERANCE);
    static double balf(double t);
};

#endif // BEZIERARC_H
