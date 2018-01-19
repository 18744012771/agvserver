#include "bezierarc.h"

#define sqr(x) (x * x)

#define _ABS(x) (x < 0 ? -x : x)

const double TOLERANCE = 0.0000001;  // Application specific tolerance

extern double sqrt(double);

double q1, q2, q3, q4, q5;

struct point2d {
        double x, y;
};

BezierArc::BezierArc()
{

}
double BezierArc::BezierArcLength(QPointF p1, QPointF p2, QPointF p3, QPointF p4)
{
    QPointF k1, k2, k3, k4;

    k1 = -p1 + 3*(p2 - p3) + p4;
    k2 = 3*(p1 + p3) - 6*p2;
    k3 = 3*(p2 - p1);
    k4 = p1;

    q1 = 9.0*(sqr(k1.x()) + sqr(k1.y()));
    q2 = 12.0*(k1.x()*k2.x() + k1.y()*k2.y());
    q3 = 3.0*(k1.x()*k3.x() + k1.y()*k3.y()) + 4.0*(sqr(k2.x()) + sqr(k2.y()));
    q4 = 4.0*(k2.x()*k3.x() + k2.y()*k3.y());
    q5 = sqr(k3.x()) + sqr(k3.y());

    double result = Simpson(balf, 0, 1, 1024, 0.001);
    return result;
}
//---------------------------------------------------------------------------
double BezierArc::balf(double t)                   // Bezier Arc Length Function
{
    double result = q5 + t*(q4 + t*(q3 + t*(q2 + t*q1)));
    if(result<0)result*=-1;
    result = sqrt(result);
    return result;
}

//---------------------------------------------------------------------------
// NOTES:       TOLERANCE is a maximum error ratio
//                      if n_limit isn't a power of 2 it will be act like the next higher
//                      power of two.
double BezierArc::Simpson (
        double (*f)(double),
        double a,
        double b,
        int n_limit,
        double TOLERANCE)
{
    int n = 1;
    double multiplier = (b - a)/6.0;
    double endsum = f(a) + f(b);
    double interval = (b - a)/2.0;
    double asum = 0;
    double bsum = f(a + interval);
    double est1 = multiplier * (endsum + 2 * asum + 4 * bsum);
    double est0 = 2 * est1;

    while(n < n_limit
          && (_ABS(est1) > 0 && _ABS((est1 - est0) / est1) > TOLERANCE)) {
        n *= 2;
        multiplier /= 2;
        interval /= 2;
        asum += bsum;
        bsum = 0;
        est0 = est1;
        double interval_div_2n = interval / (2.0 * n);

        for (int i = 1; i < 2 * n; i += 2) {
            double t = a + i * interval_div_2n;
            bsum += f(t);
        }

        est1 = multiplier*(endsum + 2*asum + 4*bsum);
    }

    return est1;
}
