# **Project: Unscented Kalman Filter**
[![Udacity - Self-Driving Car NanoDegree](https://s3.amazonaws.com/udacity-sdc/github/shield-carnd.svg)](http://www.udacity.com/drive)

This is my work for project of Self-Driving Car Nanodegree Program of Udacity.

Build instruction and dependency of this project is identical to [upstream repository](https://github.com/udacity/CarND-Unscented-Kalman-Filter-Project) which is the starter code repository of udacity.

## Result


[//]: # (Image References)

[result]: ./result.png "final result"
[nis]: ./NIS_RADAR.png "NIS RADAR"

This is screen capture image of my result.

![alt text][result]

### RMSE
Here is my RMSE result and it satisfies accuracy criteria of [project rubric](https://review.udacity.com/#!/rubrics/783/view). `RMSE <= [.09, .10, 0.40, 0.30]`

|       | px | py | vx | vy |
|:------|-------:|-------:|-------:|-------:|
| RMSE  | 0.0703 | 0.0819 | 0.3299 | 0.2466 |

### NIS(Normalized Innovation Squared)

Final tuned values of the process noise model parameters as followed.

 - longitudinal acceleration in m/s^2 : `std_a_ = 2;`
 - yaw acceleration in rad/s^2 : `std_yawdd_ = 0.785; // 0.25 * pi`

Here is plot of NIS Radar. It shows consistent result.

![alt text][nis]