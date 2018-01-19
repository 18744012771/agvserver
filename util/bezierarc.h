#ifndef BEZIERARC_H
#define BEZIERARC_H

#include <QPoint>

class BezierArc
{
public:
    BezierArc();

    static double BezierArcLength(QPointF p1, QPointF p2, QPointF p3, QPointF p4);
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
