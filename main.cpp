#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
//

using namespace std;
using namespace cv;

static void draw_point(Mat & img, const Point2f &fp, const Scalar &color) {
    circle(img, fp, 2, color, CV_FILLED, CV_AA, 0);
}

static void draw_triangulation(Mat & img, Subdiv2D & subdiv, const Scalar &delaunay_color) {
    vector<Vec6f> triangleList;
    subdiv.getTriangleList(triangleList);
    vector<Point> pt(3);
    Size size = img.size();
    Rect rect(0, 0, size.width, size.height);
    for (auto t : triangleList) {
        pt[0] = Point(cvRound(t[0]), cvRound(t[1]));
        pt[1] = Point(cvRound(t[2]), cvRound(t[3]));
        pt[2] = Point(cvRound(t[4]), cvRound(t[5]));
        // Draw rectangles completely inside the image.
        if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2])) {
            line(img, pt[0], pt[1], delaunay_color, 1, CV_AA, 0);
            line(img, pt[1], pt[2], delaunay_color, 1, CV_AA, 0);
            line(img, pt[2], pt[0], delaunay_color, 1, CV_AA, 0);
        }
    }
}

void read_image(char * path_image, char * path_points, Mat& image, vector<Point2f>& key_points) {
    image = imread(path_image);
    ifstream ifs(path_points);
    vector<int> points;
    int x, y;
    while (ifs >> x >> y) {
        key_points.emplace_back(x, y);
    }
}

Subdiv2D get_triangulation(Mat & img, vector<Point2f> & points) {
    Size size = img.size();
    Rect rect(0, 0, size.width, size.height);
    Subdiv2D subdiv(rect);
    for (const auto &point: points) {
        subdiv.insert(point);
    }
    return subdiv;
}

// Apply affine transform calculated using srcTri and dstTri to src
void applyAffineTransform(Mat & warpImage, Mat & src, vector<Point2f> & srcTri, vector<Point2f> & dstTri) {
    // Given a pair of triangles, find the affine transform.
    Mat warpMat = getAffineTransform( srcTri, dstTri);
    // Apply the Affine Transform just found to the src image
    warpAffine( src, warpImage, warpMat, warpImage.size(), INTER_LINEAR, BORDER_REFLECT_101);
}

// Warps and alpha blends triangular regions from img1 and img2 to img
void morphTriangle(Mat & img1, Mat & img2, Mat & img, vector<Point2f> & t1,
        vector<Point2f> & t2, vector<Point2f> &t, float alpha) {
    // Find bounding rectangle for each triangle
    Rect r = boundingRect(t);
    Rect r1 = boundingRect(t1);
    Rect r2 = boundingRect(t2);
    // Offset points by left top corner of the respective rectangles
    vector<Point2f> t1Rect, t2Rect, tRect;
    vector<Point> tRectInt;
    for(int i = 0; i < 3; i++) {
        tRect.emplace_back(t[i].x - r.x, t[i].y -  r.y);
        tRectInt.emplace_back(t[i].x - r.x, t[i].y - r.y); // for fillConvexPoly
        t1Rect.emplace_back(t1[i].x - r1.x, t1[i].y -  r1.y);
        t2Rect.emplace_back(t2[i].x - r2.x, t2[i].y - r2.y);
    }
    // Get mask by filling triangle
    Mat mask = Mat::zeros(r.height, r.width, CV_32FC3);
    fillConvexPoly(mask, tRectInt, Scalar(1.0, 1.0, 1.0), 16, 0);
    // Apply warpImage to small rectangular patches
    Mat img1Rect, img2Rect;
    img1(r1).copyTo(img1Rect);
    img2(r2).copyTo(img2Rect);

    Mat warpImage1 = Mat::zeros(r.height, r.width, img1Rect.type());
    Mat warpImage2 = Mat::zeros(r.height, r.width, img2Rect.type());

    applyAffineTransform(warpImage1, img1Rect, t1Rect, tRect);
    applyAffineTransform(warpImage2, img2Rect, t2Rect, tRect);
    // Alpha blend rectangular patches
    Mat imgRect = (1.0 - alpha) * warpImage1 + alpha * warpImage2;

    // Copy triangular region of the rectangular patch to the output image
    multiply(imgRect,mask, imgRect);
    multiply(img(r), Scalar(1.0,1.0,1.0) - mask, img(r));
    img(r) = img(r) + imgRect;
}

int main(int argc, char * argv[]) {
    if (argc != 7) {
        cout << "use following args:" << endl;
        cout << "arg #1: path to image 1" << endl;
        cout << "arg #2: path to points for image 1" << endl;
        cout << "arg #3: path to image 2" << endl;
        cout << "arg #4: path to points for image 2" << endl;
        cout << "arg #5: number of frames (min 2)" << endl;
        cout << "arg #5: path to output results" << endl;
        return 0;
    }

    int frames = stoi(argv[5]);
    if (frames < 2) {
        cout << "minimum 2 frames" << endl;
        return 0;
    }

    string res_path = argv[6];
    if (res_path.back() != '/') {
        res_path += "/";
    }

    Mat img1, img2;
    vector<Point2f> key_points1, key_points2;
    read_image(argv[1], argv[2], img1, key_points1);
    read_image(argv[3], argv[4], img2, key_points2);
    auto subdiv1 = get_triangulation(img1, key_points1);
    auto subdiv2 = get_triangulation(img2, key_points2);

    img1.convertTo(img1, CV_32F);
    img2.convertTo(img2, CV_32F);

    // Draw triangulation
    Mat img1_copy, img2_copy;
    img1.copyTo(img1_copy);
    img2.copyTo(img2_copy);
    Scalar color(255, 255, 255);
    draw_triangulation(img1_copy, subdiv1, color);
    draw_triangulation(img2_copy, subdiv2, color);
    imwrite(res_path + "tri1.jpg", img1_copy);
    imwrite(res_path + "tri2.jpg", img2_copy);

    vector<Vec6f> triangles1, triangles2;
    subdiv1.getTriangleList(triangles1);
    auto len = triangles1.size();

    vector<vector<int>> index;
    Size size = img1.size();
    for (int i = 0; i < len; ++i) {
        vector<pair<Point2f, int>> cur;
        for (int j = 0; j <= 4; j += 2) {
            cur.emplace_back(Point2f(triangles1[i][j], triangles1[i][j + 1]), 0);
            for (auto k = 0; k < key_points1.size(); ++k) {
                if (key_points1[k] == cur.back().first) {
                    cur[cur.size() - 1].second = k;
                    break;
                }
            }
        }
        Rect rect(0, 0, size.width, size.height);
        if (rect.contains(cur[0].first) && rect.contains(cur[1].first) && rect.contains(cur[2].first)) {
            index.emplace_back(vector<int>({cur[0].second, cur[1].second, cur[2].second}));
        }
    }

    for (int id = 0; id < frames; ++id) {
        auto alpha = static_cast<float>(1. * id / (frames - 1));
        Mat img_res = Mat::zeros(img1.size(), CV_32FC3);
        vector<Point2f> key_points_res;
        for (auto i = 0; i < key_points1.size(); ++i) {
            auto x = (1. - alpha) * key_points1[i].x + alpha * key_points2[i].x;
            auto y = (1. - alpha) * key_points1[i].y + alpha * key_points2[i].y;
            key_points_res.emplace_back(x, y);
        }

        for (const auto &i: index) {
            vector<Point2f> t1, t2, t_res;
            for (auto j = 0; j < 3; ++j) {
                t1.push_back(key_points1[i[j]]);
                t2.push_back(key_points2[i[j]]);
                t_res.push_back(key_points_res[i[j]]);
            }
            morphTriangle(img1, img2, img_res, t1, t2, t_res, alpha);
        }

        imwrite(res_path + to_string(id) + ".jpg", img_res);
    }

    return 0;
}