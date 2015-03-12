#include <iostream>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/xfeatures2d/nonfree.hpp"

#include "beta_shape.cc"

#define MATCH_CUTOFF 0.05
#define HIST_CUTOFF 0.02
#define HIST_BINS 12

using namespace std;
using namespace cv;
//using namespace cv::xfeatures2d;

struct Features{
    unsigned char n_colors;
    unsigned short colors, points;
    float white, sharpness, aspect;
};

unsigned char bit_count16(unsigned short n){
    unsigned short x = n;
    x = x - ((x >> 1) & 0x5555);
    x = (x & 0x3333) + ((x >> 2) & 0x3333);
    x = (x + (x >> 4)) & 0x0F0F;
    x = x + (x >> 8);
    return x & 0x1F;
}

/// Extraction
void get_histogram(Mat &im, Features *f){
    Mat hls, hist;
    cvtColor(im, hls, COLOR_BGR2HLS);
    int h_bins = HIST_BINS - 2; int l_bins = 4;
    int histSize[] = {h_bins, l_bins};
    float h_ranges[] = {0, 180};
    float l_ranges[] = {0, 256};

    float histogram[HIST_BINS];
    const float* ranges[] = {h_ranges, l_ranges};
    int channels[] = {0, 1};
    calcHist(&hls, 1, channels, Mat(), hist, 2, histSize, ranges);
    normalize(hist, hist);
    for(int i = 0; i < HIST_BINS; i++)
        histogram[i] = 0.0;
    for(int i = 0; i < h_bins; i++){
        histogram[0] += hist.at<float>(i, 3);
        histogram[1] += hist.at<float>(i, 0);
        histogram[i+2] += hist.at<float>(i, 1) + hist.at<float>(i, 2);
    }
    f->colors = 0;
    f->white = histogram[0];
    for(int i = 1; i < HIST_BINS; i++) {
        if (histogram[i] > HIST_CUTOFF)
            f->colors |= 1 << (i-1);
    }
    f->n_colors = bit_count16(f->colors);
}

// void get_descriptors(Mat &im, Mat &desc){
//     Ptr<SURF> surf = SURF::create(400.0, 3, 3, false);
//     std::vector<KeyPoint> kp;
//     surf->detectAndCompute(im, Mat(), kp, desc);
// }

void get_shapes(Mat &im, Contour &shape){ // Remove im
    Mat gray, edges;
    cvtColor(im, gray, COLOR_BGR2GRAY);
    // bilateralFilter(gray.clone(), gray, 11, 17, 17);
    Canny(gray, edges, 30, 150);
    Contours contours, outer, inner;
    Hierarchy hierarchy;
    findContours(edges, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);

    for(size_t i = 0; i < hierarchy.size(); i++){
        if (hierarchy[i][3] == -1){
            outer.push_back(contours[i]);
        }
    }
    Contour beta;
    get_beta_shape(outer, beta);

    // DRAWING
    // Contours b;
    // b.push_back(beta);
    // cout << beta.size() << endl;
    // drawContours(im, b, -1, Scalar(225,0,255),
    //              2, LINE_AA);


    /// Get inner shapes that don't match outer ones
    /// reduce points of all shapes approxPolyDP
    /// Find points to perimeter ratio: arcLength
    /// Total points?
    /// Sort inner points by area, pick ones that make up majority
    /// Find b-shape of inner points

}

int get_features(string file, Features *f, Contour &shape){
    Mat im;
    im = imread(file, CV_LOAD_IMAGE_COLOR);
    if (im.empty()) return 0;
    get_shapes(im, shape);
    get_histogram(im, f); // TODO trim image before getting histogram
    // namedWindow(file, 1);
    // imshow(file, im);
    /// BoundingRect(shape) -> topleft, w, h
    //f->aspect = 1.0 * w, h;
    return 1;
}


/// Distance

// float descriptor_distance(Mat desc1, Mat desc2){
//     //FlannBasedMatcher matcher;
//     BFMatcher matcher(NORM_L1);
//     vector<DMatch> matches;
//     matcher.match(desc1, desc2, matches);
//     float r = 0.0;
//     vector<DMatch>::iterator it;
//     for(it = matches.begin(); it != matches.end(); ++it){
//         float d = (*it).distance;
//         r += (d*d);
//     }
//     r /= matches.size();
//     return r;
// }

float match_c = 1.0;
float color_count_c = 0.5;
float histogram_distance(Features *f1, Features *f2){
    // TODO white
    int color_match = f1->colors ^ f2->colors;
    int count_diff = abs(f1->n_colors - f2->n_colors);
    color_match = bit_count16(color_match) - count_diff;
    return color_match * match_c + count_diff * color_count_c;
}

const float histogram_c = 1;
const float aspect_c = 1;

float feature_distance(Features *f1, Features *f2){
    float hist_dist = histogram_distance(f1, f2);
    //float a = aspect1-aspect2;
    //float aspect_dist = (a*a)/(aspect1 + aspect2);
    return histogram_c * hist_dist
        //+ aspect_c * aspect_dist
        ;
}
