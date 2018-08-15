# image morphing

### build
```
git clone https://github.com/Voudy/image_morphing.git
cd image_morphing
mkdir build
cd build
cmake ..
make
```
### run
```
./image_morphing img1 points1 img2 points2 frames output
```
`img*` - path to the image

`points*` - path to the corresponding points in format:
```
x1 y1
x2 y2
```
`frames` - number of frames (minimum 2)

`output` - path to output folder (it must exist)
### result
<img src="result/result.gif" width=300 height=400/>

### links
- [Face Morph Using OpenCV — C++ / Python](https://www.learnopencv.com/face-morph-using-opencv-cpp-python/)