#include "destripe.hpp"

#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <vector>
#include <stdexcept>

#include "destripe.hpp"

using namespace std;

typedef uint8_t uint8;
int ROUND(double x) {return x >= 0 ? int(x+0.5) : int (x-0.5);}

class MeanStd {  // for computing mean and standard deviation
public:
    MeanStd(){sum = sum2 = 0.0; n=0;}
    void Reset(){sum = sum2 = 0.0; n=0;}
    void Element(double a){sum += a; sum2 += a*a; n++;}
    void Stats(double &avg, double &std){ avg = sum/n; std = sum2/n-avg*avg < 0 ? 0.0 : sqrt(sum2/n - avg*avg);}  // could be off by rounding
    double Mean(){return sum/n;}
    double Sum(){return sum;}
    double Sum2(){return sum2;}
    double Std() {double avg = sum/n; return sum2/n-avg*avg < 0 ? 0.0 : sqrt(sum2/n - avg*avg);}  // could be off by rounding
    double Var() {double avg = sum/n; return sum2/n-avg*avg < 0 ? 0.0 : sum2/n - avg*avg;}        // could be off by rounding
    long int HowMany(){return n;}
private:
    double sum, sum2;
    long int n;  // could average more the 2B elements, so need long
};

struct Correction {
public:
    double left, right;
    Correction(){ left = 0.0; right = 0.0;}
};


vector<uint8> destripe(uint8 * image, size_t w, size_t h, size_t YC, vector<int> const & seam, bool writeplot)
{

    printf("%d measurement points in Y\n", int(YC));
    vector<int> ys(YC+2);
    ys[0] = -1;
    for(size_t i=1; i<=YC; i++)
        ys[i] = int((double(i)-0.5)/YC * h);
    ys[YC+1] = int(h);
    for(size_t i=0; i<ys.size(); i++) {
        printf("ys[%2d] = %5d\n", int(i), ys[i]);
    }
    // here are the X values of the seams.  Also two at ends.
    //int seam[] = {-1, 2066,4684,7574,10385,13222,15937,18531,21198,23826,26506,29175,31772, int(w)};
    //size_t NS = sizeof(seam)/sizeof(int);

    size_t NS = seam.size();
    printf("%d seams\n", int(NS));

    if (seam[0] != -1 or seam[NS-1] != int(w)) {
        throw std::runtime_error("seam definitions must start with -1 and end with the image width!");
    }

    // look at normalizing through one section.   First make plot
    vector<double> means_by_col(w);

    if (writeplot) {
        FILE *fp = fopen("pl", "w");
        if (fp == nullptr) {
            throw std::runtime_error("Could not open 'pl'\n");
        }
        for(size_t x=0; x<w; x++) {
            MeanStd m;
            for(size_t y=0; y<h; y++)
                m.Element(image[x+y*w]);
            fprintf(fp, "%d %.2f %.2f\n", int(x), m.Mean(), m.Std() );
            means_by_col[x] = m.Mean();
        }
        fclose(fp);
        printf("Wrote file 'fp'\n");
    }

    for(size_t i=0; i<NS-1; i++) {
        size_t xmin = static_cast<size_t>(max(seam[i]+1, 0));
        size_t xmax = static_cast<size_t>(min(seam[i+1]-1, int(w)-1));
        MeanStd overall;
        for(size_t x=xmin; x <= xmax; x++)
            overall.Element(means_by_col[x]);
        printf("slab mean is %.2f\n", overall.Mean());
        for(size_t x=xmin; x <= xmax; x++) {
            int delta = ROUND(overall.Mean() - means_by_col[x]);
            for(size_t y=0; y<h; y++) {
                int pix = image[x+y*w] + delta;
                pix = min(pix, 255);
                pix = max(pix,   0);
                image[x+y*w] = static_cast<uint8>(pix);
            }
        }
    }

    // Create an array of corrections.  It's unevenly spaced in X and Y, but with a constant number of points in each row/column.
    // Two extra points in each direction; one at 0 and one at the far edge.  All measured points are interior.

    size_t XSIZE = NS;
    size_t YSIZE = YC+2;
    vector<vector<Correction>  >corr(XSIZE, vector<Correction>(YSIZE));
    printf("Correction vector has %lu entries, %lu by %lu\n", corr.size(), XSIZE, YSIZE);

    for(size_t i=1; i<NS-1; i++) {
        int xmid = seam[i];
        const int DX = 10;
        const int SX = 100;
        for(size_t iy=1; iy < YSIZE-1; iy++) {
            int y0 = ys[iy];
            MeanStd left, right;
            for(int y=y0-500; y<y0+500 && y < int(h); y++) {
                for(int x=DX; x<=SX; x++) {
                    left .Element(image[xmid-x + y*int(w)]);
                    right.Element(image[xmid+x + y*int(w)]);
                }
            }
            printf("At y=%5d and xmid = %d, left %.2f %.3f, right %.2f %.3f\n", y0, xmid, left.Mean(), left.Std(), right.Mean(), right.Std() );
            // If both values look plausible, set corrections at this location.
            // Plausible means values between 100-200, std between 15 and 60.
            bool PlausLeft   = left.Mean() > 100 &&  left.Mean() < 220 &&  left.Std() > 15 &&  left.Std() < 60;
            bool PlausRight = right.Mean() > 100 && right.Mean() < 220 && right.Std() > 15 && right.Std() < 60;
            if (PlausLeft && PlausRight) {
                double mid = (left.Mean() + right.Mean())/2.0;
                corr[i][iy].left = mid - left.Mean();  // amount to add
                corr[i][iy].right= mid - right.Mean();
            }
        }
    }
    for(size_t y=0; y<YSIZE; y++) {
        for(size_t x=0; x<XSIZE; x++)
            printf("%5.1f:%5.1f ", corr[x][y].left, corr[x][y].right);
        printf("\n");
    }

    // Find the corrections.  Put the X loop on the outside, even though that's bad for the cache, since the X computation is more expensive.
    vector<uint8> fake(w*h, 127);
    double yslot = double(h)/YC;
    for(int x=0; x<int(w); x++) {
        // find the x bin.  May not exist since exact seams don't count
        size_t ix;
        for(ix = 0; ix < XSIZE-1; ix++)
            if (x > seam[ix] && x < seam[ix+1])
                break;

        if (ix >= XSIZE-1) {// found nothing.  Just copy this column.  Fix the case where it's black, since Shinya prefers white.
            printf("No X at %d\n", int(x));
            for(size_t y=0; y<h; y++) {
                uint8 pix = image[x+int(y*w)];
                if (pix == 0 && (image[size_t(x)+y*w-1] != 0 || image[size_t(x)+y*w+1] != 0)) // if not in a completely black area
                    pix = 225;  // Set to a very white value
                fake[size_t(x)+y*w] = pix;
            }
            continue;
        }
        double alpha = (double(x) - seam[ix])/(seam[ix+1] - seam[ix]);
        //printf("alpha %f\n", alpha);
        for(size_t y=0; y<h; y++) {
            size_t slot = static_cast<size_t>(double(y)/yslot + 0.5);
            // check
            if (int(y) < ys[slot] || int(y) > ys[slot+1])
                printf("Oops.  yslot %f, slot %d, ys %d %d\n", yslot, int(slot), ys[slot], ys[slot+1]);

            // OK, interpolate
            double beta = (double(y) - ys[slot])/(ys[slot+1] - ys[slot]);

            double incr = (1-alpha)*(1-beta)*corr[ix  ][slot  ].right +
                          (alpha  )*(1-beta)*corr[ix+1][slot  ].left  +
                          (1-alpha)*(beta  )*corr[ix  ][slot+1].right +
                          (alpha  )*(beta  )*corr[ix+1][slot+1].left  ;

            int delta = incr >= 0 ? int(incr + 0.5) : int(incr - 0.5);  // round to integer

            int pix = int(image[size_t(x)+y*w]) + delta;
            //pix = 127 + delta;  // just testing
            pix = min(pix, 255);
            pix = max(pix,   0);
            fake[size_t(x)+y*w] = static_cast<uint8>(pix);
        }
    }

    return fake;
}
